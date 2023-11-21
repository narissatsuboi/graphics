// SmoothMesh: smooth, Phong-shaded .obj mesh

#include <glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "IO.h"
#include "Misc.h"
#include "VecMat.h"
#include "Widgets.h"


// files
const char* bodyObjectFilename = "Airplane-Body.obj";
const char* propObjectFilename = "Airplane-Propeller.obj";

// OpenGL IDs
GLuint		 program = 0;

// window, camera
int          winWidth = 800, winHeight = 800;
Camera		 camera(0, 0, winWidth, winHeight, vec3(0, 0, 0), vec3(0, 0, -4.5f), 30, 0.001f, 500);
vec3		 grn(.1f, .6f, .1f), yel(1, 1, 0), blu(0, 0, 1);

// meshes
struct Mesh {
	vector<vec3> points, normals;  // from .obj file
	vector<vec2> uvs;              // from .obj file
	vector<int3> triangles;        // from .obj file
	mat4 toWorld;                  // object-to-world transformation
	GLuint vBuffer = 0;            // GPU vertex buffer

	int textureUnit = 0;

	void Read(const char* objFileName) {
		if (!ReadAsciiObj(objFileName, points, triangles, &normals, &uvs))
			printf("can't read %s\n", bodyObjectFilename);
		glGenBuffers(1, &vBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
		int sPoints = points.size() * sizeof(vec3), sNormals = normals.size() * sizeof(vec3);
		glBufferData(GL_ARRAY_BUFFER, sPoints + sNormals, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points.data());
		glBufferSubData(GL_ARRAY_BUFFER, sPoints, sNormals, normals.data());
	}

	void Render(const vec3 color) {
		glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
		VertexAttribPointer(program, "point", 3, 0, (void*)0);
		VertexAttribPointer(program, "normal", 3, 0, (void*)(points.size() * sizeof(vec3)));
		SetUniform(program, "modelview", camera.modelview * toWorld);
		SetUniform(program, "persp", camera.persp);
		SetUniform(program, "color", color);
		glDrawElements(GL_TRIANGLES, 3 * triangles.size(), GL_UNSIGNED_INT, triangles.data());
	}
};
Mesh body, prop;  

// lighting

vec3		lights[] = { {.5, 0, 1}, {1, 1, 0} };
const int	nLights = sizeof(lights)/sizeof(vec3);

// interaction

void	   *picked = NULL;
Mover		mover;


// shaders

const char *vertexShader = R"(
	#version 130
	in vec3 point, normal;
	out vec3 vPoint, vNormal;
	uniform mat4 modelview, persp;
	void main() {
		vPoint = (modelview*vec4(point, 1)).xyz;
		vNormal = (modelview*vec4(normal, 0)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
	}
)";

const char *pixelShader = R"(
	#version 130
	in vec3 vPoint, vNormal;
	uniform int nLights = 0;
	uniform vec3 lights[20];
	uniform float amb = .1, dif = .7, spc =.7;		// ambient, diffuse, specular
	uniform vec3 color;
	uniform bool highlights = true;
	out vec4 pColor;
	void main() {
		float d = 0, s = 0;							// diffuse, specular terms
		vec3 N = normalize(vNormal);				// surface normal
		vec3 E = normalize(vPoint);					// eye vector
		for (int i = 0; i < nLights; i++) {
			vec3 L = normalize(lights[i]-vPoint);	// light vector
			vec3 R = reflect(L, N);					// highlight vector
			d += max(0, dot(N, L));					// one-sided diffuse
			if (highlights) {
				float h = max(0, dot(R, E));		// highlight term
				s += pow(h, 100);					// specular term
			}
		}
		float ads = clamp(amb+dif*d+spc*s, 0, 1);
		pColor = vec4(ads*color, 1);
	}
)";

void Display(GLFWwindow *w) {
	// clear screen, enable blend, z-buffer
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// enable shader program and GPU buffer, update matrices
	glUseProgram(program);
	// send transformed lights
	vec3 xLights[nLights];
	for (int i = 0; i < nLights; i++) {
		vec4 xLight = camera.modelview*vec4(lights[i]);
		xLights[i] = vec3(xLight.x, xLight.y, xLight.z);
	}
	SetUniform(program, "nLights", nLights);
	SetUniform3v(program, "lights", nLights, (float *) xLights);
	// render plane parts
	body.Render(grn);
	prop.Render(blu);
	glFlush();
}

// Mouse Handlers

void MouseButton(float x, float y, bool left, bool down) {
	picked = NULL;
	if (left && down) {
		// light picked?
		for (size_t i = 0; i < nLights; i++)
			if (MouseOver(x, y, lights[i], camera.fullview)) {
				picked = &mover;
				mover.Down(&lights[i], (int) x, (int) y, camera.modelview, camera.persp);
			}
		if (picked == NULL) {
			picked = &camera;
			camera.Down(x, y, Shift());
		}
	}
	else camera.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown) {
		if (picked == &mover)
			mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
		if (picked == &camera)
			camera.Drag(x, y);
	}
}

void MouseWheel(float spin) {
	camera.Wheel(spin, Shift());
}




// Application

void Resize(int width, int height) {
	camera.Resize(width, height);
}


int main(int argc, char **argv) {
	// enable anti-alias, init app window and GL context
	GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "Aerial Animation");
	// init shader, read from file, fill GPU vertex buffer, read texture
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	// fill GPU with object vertices
	body.Read(bodyObjectFilename);
	prop.Read(propObjectFilename);
	// callbacks
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Display(w);
		glfwPollEvents();
		glfwSwapBuffers(w);
	}
	// cleanup
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &body.vBuffer);
	glfwDestroyWindow(w);
	glfwTerminate();
}
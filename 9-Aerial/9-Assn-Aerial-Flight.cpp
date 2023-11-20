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
const char	*objectFilename = "C:/Users/Jules/Code/Assets/Models/Cat.obj";
const char	*textureFilename = "C:/Users/Jules/Code/Assets/Images/Cat.tga";

// window, camera
int          winWidth = 800, winHeight = 800;
Camera		 camera(0, 0, winWidth, winHeight, vec3(0, 0, 0), vec3(0, 0, -4.5f), 30, 0.001f, 500);
vec3		 grn(.1f, .6f, .1f), yel(1, 1, 0), blu(0, 0, 1);

// 3D mesh
vector<vec3> points, normals;           // vertices, normals
vector<vec2> uvs;						// texture coordinates
vector<int3> triangles;                 // triplets of vertex indices

// OpenGL IDs
GLuint		 vBuffer = 0, program = 0;
GLuint		 textureName = 0;			// texture map ID
int			 textureUnit = 0;			// GPU buffer

// lighting
vec3		lights[] = { {.5, 0, 1}, {1, 1, 0} };
const int	nLights = sizeof(lights)/sizeof(vec3);

// interaction
void	   *picked = NULL;
Mover		mover;

// options
bool		shading = true, faceted = false, showHighlights = true, showLines = false, showNormals = false;

// Display

const char *vertexShader = R"(
	#version 130
	in vec3 point, normal;
	in vec2 uv;
	out vec3 vPoint, vNormal;
	out vec2 vUv;
	uniform mat4 modelview, persp;
	void main() {
		vPoint = (modelview*vec4(point, 1)).xyz;
		vNormal = (modelview*vec4(normal, 0)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
		vUv = uv;
	}
)";

const char *pixelShader = R"(
	#version 130
	in vec3 vPoint, vNormal;
	in vec2 vUv;
	uniform int nLights = 0;
	uniform vec3 lights[20];
	uniform float amb = .1, dif = .7, spc =.7;		// ambient, diffuse, specular
	uniform sampler2D textureImage;
	uniform bool highlights = true;
	uniform bool faceted = false;
	out vec4 pColor;
	void main() {
		float d = 0, s = 0;							// diffuse, specular terms
		vec3 N = normalize(faceted?
			cross(dFdx(vPoint), dFdy(vPoint)) :
			vNormal);								// surface normal
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
		pColor = vec4(ads*texture(textureImage, vUv).rgb, 1);
	}
)";

void Display(GLFWwindow *w) {
	// clear screen, enable blend, z-buffer
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// enable shader program and GPU buffer, update matrices
	glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	SetUniform(program, "modelview", camera.modelview);
	SetUniform(program, "persp", camera.persp);
	// send transformed lights
	vec3 xLights[nLights];
	for (int i = 0; i < nLights; i++) {
		vec4 xLight = camera.modelview*vec4(lights[i]);
		xLights[i] = vec3(xLight.x, xLight.y, xLight.z);
	}
	SetUniform(program, "nLights", nLights);
	SetUniform3v(program, "lights", nLights, (float *) xLights);
	// texture map
	glActiveTexture(GL_TEXTURE0+textureUnit);
	glBindTexture(GL_TEXTURE_2D, textureName);
	SetUniform(program, "textureImage", textureUnit);
	// options
	SetUniform(program, "highlights", showHighlights);
	SetUniform(program, "faceted", faceted);
	// GPU buffer links
	int sPoints = points.size()*sizeof(vec3);
	VertexAttribPointer(program, "point", 3, 0, (void *) 0);
	VertexAttribPointer(program, "normal", 3, 0, (void *) sPoints);
	VertexAttribPointer(program, "uv", 2, 0, (void *) (2*sPoints));
	// render mesh
	if (shading)
		glDrawElements(GL_TRIANGLES, 3*triangles.size(), GL_UNSIGNED_INT, triangles.data());
	// annotations
	glDisable(GL_DEPTH_TEST);
	UseDrawShader(camera.fullview);
	for (int i = 0; i < nLights; i++)
		Star(lights[i], 8, vec3(1, .4f, 0), vec3(0, 0, 1));
	if (picked == &camera && !Shift())
		camera.arcball.Draw(Control());
	if (showLines) {
		UseDrawShader(camera.fullview);
		for (int i = 0; i < (int) triangles.size(); i++) {
			int3 t = triangles[i];
			vec3 p1 = points[t.i1], p2 = points[t.i2], p3 = points[t.i3];
			Line(p1, p2, 1, blu); Line(p2, p3, 1, blu); Line(p3, p1, 1, blu);
		}
	}
	if (showNormals) {
		UseDrawShader(mat4());
		for (size_t i = 0; i < normals.size(); i++)
			ArrowV(points[i], .1f*normals[i], camera.modelview, camera.persp, grn, 1, 6);
	}
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

void BufferVertices() {
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	int sPoints = points.size()*sizeof(vec3), sNormals = normals.size()*sizeof(vec3), sUvs = uvs.size()*sizeof(vec2);
	glBufferData(GL_ARRAY_BUFFER, sPoints+sNormals+sUvs, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points.data());
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sNormals, normals.data());
	glBufferSubData(GL_ARRAY_BUFFER, sPoints+sNormals, sUvs, uvs.data());
}

void Resize(int width, int height) {
	camera.Resize(width, height);
}

void Keyboard(int key, bool press, bool shift, bool control) {
	if (press) {
		if (key == 'F') faceted = !faceted;
		if (key == 'H') showHighlights = !showHighlights;
		if (key == 'L') showLines = !showLines;
		if (key == 'N') showNormals = !showNormals;
		if (key == 'S') shading = !shading;
	}
}

const char *usage = R"(Usage:
	F: faceted on/off
	H: highlights on/off
	L: lines on/off
	N: normals on/off
	S: shading on/off
)";

int main(int argc, char **argv) {
	// enable anti-alias, init app window and GL context
	GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "Smooth, Textured Mesh");
	// init shader, read from file, fill GPU vertex buffer, read texture
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	if (!ReadAsciiObj(objectFilename, points, triangles, &normals, &uvs))
		printf("can't read %s\n", objectFilename);
	Standardize(points.data(), points.size(), .8f);
	BufferVertices();
	textureName = ReadTexture((char *) textureFilename);
	// callbacks
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	RegisterKeyboard(Keyboard);
	printf(usage);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Display(w);
		glfwPollEvents();
		glfwSwapBuffers(w);
	}
	// cleanup
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
	glfwDestroyWindow(w);
	glfwTerminate();
}

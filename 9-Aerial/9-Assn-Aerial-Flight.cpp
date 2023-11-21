// Aerial animation with plane and propeller on Bezier curve

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
vec3		 charcoalGrey(34.0f / 255.0f, 34.0f / 255.0f, 34.0f / 255.0f), hotPink(255.0f / 255.0f, 105.0f / 255.0f, 180.0f / 255.0f), 
			 grn(.1f, .6f, .1f), orange(255.0f / 255.0f, 165.0f / 255.0f, 0.0f), blu(0, 0, 1);

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

class Bezier {
public:
	vec3* pts = NULL;					 // pointer to four control points
	Bezier(vec3* pts) : pts(pts) { }
	vec3 Position(float t) {
		// curve starts at t = 0, ends at t = 1
		float t2 = t * t, t3 = t * t2;
		return (-t3 + 3 * t2 - 3 * t + 1) * pts[0] + (3 * t3 - 6 * t2 + 3 * t) * pts[1] + (3 * t2 - 3 * t3) * pts[2] + t3 * pts[3];
	}
	void Draw(int res = 50, float curveWidth = 3.5f, float meshWidth = 2.5f) {
		vec3 lineColor = charcoalGrey, meshColor = charcoalGrey, pointColor = grn;
		// draw curve as res number of straight segments
		for (int i = 0; i < res; i++)
			Line(Position((float)i / res), Position((float)(i + 1) / res), curveWidth, lineColor);
		// draw control mesh
		for (int i = 0; i < 3; i++)
			LineDash(pts[i], pts[i + 1], meshWidth, meshColor, meshColor);
		// draw control points
		for (int i = 0; i < 4; i++)
			Disk(pts[i], 5 * curveWidth, pointColor);
	}
	vec3 Velocity(float t) {
		float t2 = t * t;
		// compute derivative of curve
		return (-3 * t2 + 6 * t - 3) * pts[0] + (9 * t2 - 12 * t + 3) * pts[1] + (6 * t - 9 * t2) * pts[2] + 3 * t2 * pts[3];
	}
	mat4 Frame(float t) {
		vec3 p = Position(t), v = normalize(Velocity(t)), n = normalize(cross(v, vec3(0, 1, 0)));
		vec3 b = normalize(cross(n, v));
		mat4 m = mat4(
			vec4(n[0], b[0], -v[0], p[0]),
			vec4(n[1], b[1], -v[1], p[1]),
			vec4(n[2], b[2], -v[2], p[2]),
			vec4(0, 0, 0, 1)
		);
		return m;
	}
};
vec3 path[] = {
	{2 / 3.f,0,2 / 3.f},     {1,0,1 / 3.f},     {1,.1f,-1 / 3.f},               // curve1: path[0]-path[3]
	{2 / 3.f,.1f,-2 / 3.f},  {1 / 3.f,.1f,-1},  {-1 / 3.f,.4f,-1},              // curve2: path[3]-path[6]
	{-2 / 3.f,.4f,-2 / 3.f}, {-1,.4f,-1 / 3.f}, {-1,0,1 / 3.f},                 // curve3: path[6]-path[9]
	{-2 / 3.f,0,2 / 3.f},    {-1 / 3.f,0,1},    {1 / 3.f,0,1}, {2 / 3.f,0,2 / 3.f}  // curve4: path[9]-path[12]
};
const int nPath = sizeof(path) / sizeof(vec3) - 1;                         // skip redundant path[12]

Bezier bezier[] = { Bezier(&path[0]), Bezier(&path[3]), Bezier(&path[6]), Bezier(&path[9]) };
const int nBezier = sizeof(bezier) / sizeof(Bezier);

time_t startTime = clock();                                      // app start time
float duration = 3;                                              // time to fly path 


// lighting

vec3		lights[] = { {.5, 0, 1}, {1, 1, 0}, {0.1, 0.75, 0} };
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
	body.Render(hotPink);
	prop.Render(blu);
	// draw flight path
	UseDrawShader(camera.fullview);
	for (int i = 0; i < 4; i++) {
		bezier[i].Draw();
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

void Resize(int width, int height) {
	camera.Resize(width, height);
}


void Animate() {
	float elapsed = (float)(clock() - startTime) / CLOCKS_PER_SEC, a = nBezier * elapsed / duration;
	float b = fmod(a, nBezier), t = b - floor(b);
	int i = (int)floor(b);
	mat4 f = bezier[i].Frame(t);
	body.toWorld = f * Scale(.35f) * RotateY(-90);
	prop.toWorld = body.toWorld * Translate(-.6f, 0, 0) * RotateY(-90) * Scale(.25f) * RotateZ(1500 * elapsed);
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
		Animate();
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
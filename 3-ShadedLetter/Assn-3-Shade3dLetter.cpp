// ColorfulTriangle.cpp - draw RGB letter "L" via GLSL, GPU

#include <glad.h>
#include <glfw3.h>
#include "GLXtras.h"
#include "VecMat.h"
#include "Draw.h"
#include "Text.h"
#include "Camera.h"

GLuint vBuffer = 0; // GPU buffer ID
GLuint program = 0; // GLSL shader program ID
vec2 mouseWas, mouseNow; // rotation control 

// the letter "L"
float zWidth = -15.0;
vec3 colors[] = {
					{0.8, 0.2, 0.7}, {0.5, 0.9, 0.1}, {0.2, 0.6, 0.9}, {0.7, 0.3, 0.6}, {0.9, 0.7, 0.2}, {0.4, 0.1, 0.8},
					{0.8, 0.2, 0.7}, {0.5, 0.9, 0.1}, {0.2, 0.6, 0.9}, {0.7, 0.3, 0.6}, {0.9, 0.7, 0.2}, {0.4, 0.1, 0.8}
};
vec3 points[] = {  // front                  back 
					{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, zWidth},
					{50.0f, 0.0f, 0.0f},{50.0f, 0.0f, zWidth},
					{50.0f, 20.0f,0.0f}, {50.0f, 20.0f,zWidth},
					{20.0f, 20.0f, 0.0f}, {20.0f, 20.0f, zWidth},
					{0.0f, 80.0f, 0.0f}, {0.0f, 80.0f, zWidth},
					{20.0f, 80.0f, 0.0f}, {20.0f, 80.0f, zWidth}
};

int nPoints = sizeof(points) / sizeof(vec3);
int triangles[][3] = {  // front   back 
						{0,1,2}, {4,6,5},
						{0,2,3}, {4,7,6},
						{0,4,5}, {4,9,8},
						{0,3,5}, {4,9,7}
};

int nTriangles = sizeof(triangles) / (3 * sizeof(int));

const char *vertexShader = R"(
	#version 130
	uniform mat4 modelview, persp; 
	in vec3 point;
	in vec3 color;
	out vec3 vPoint; 
	out vec3 vColor;
	void main() {
		vPoint = (modelview*vec4(point, 1)).xyz;   // transformed to world space 
		gl_Position = persp*vec4(vPoint, 1);       // transformed to perspective space 
		vColor = color; 
	}
)";

const char *pixelShader = R"(

	#version 130
	in vec3 vPoint;
	in vec3 vColor;
	out vec4 pColor;

	void main() {
		uniform vec3 light = vec3(1, 1, 1); 
		vec3 dx = dFdx(vPoint), dy = dFdy(vPoint);) // vPoint change along hor/vert raster
		vec3 N = normalize(cross(dx, dy));     // unit-length surface normal 

		// d, diffuse term calcuation 
		vec3 L = normalize(lights-vPoint);     // unit-length light vector 
		float d = abs(dot(N, L));         

		vec3 E = normalize(vPoint);  // specular highlight 

		vec3 R = reflect(L, N);          // reflection vector

		float h = max(0, dot(R, E));        // highlight term 
		float s = pow(h, 100);          // specular term 

		uniform float amb = .1, dif = .8, spc =.7;  // ambient, diffuse, specular weights 

		float intensity = min(1, amb+dif*d)+spc*s;  // weighted sum 
		pColor = vec4(intensity*vColor, 1);     // opaque 
	}
)";

int winWidth = 800; 
int winHeight = 800; 

Camera camera(0, 0, winWidth, winHeight, vec3(15, -30, 0), vec3(0, 0, -5), 30); 

void Display(GLFWwindow *w) {
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	SetUniform(program, "modelview", camera.modelview);
	SetUniform(program, "persp", camera.persp);

	// clear background
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	// run shader program, enable GPU vertex buffer
	glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// connect GPU point and color buffers to shader inputs
	VertexAttribPointer(program, "point", 3, 0, (void *) 0);
	VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));
	// render three vertices as one triangle
	int nVertices = sizeof(triangles) / sizeof(int);
	glDrawElements(GL_TRIANGLES, nTriangles*3, GL_UNSIGNED_INT, triangles);

	// test
	if (false) { // set false for HW turn-n 
		UseDrawShader(camera.fullview);
		// draw/label vertices in yellow 
		for (int i = 0; i < nPoints; i++) {
			vec3 p = points[i];
			Disk(p, 8, vec3(0.8, 0.2, 0.7));
			Text(p, camera.fullview, vec3(0.8, 0.2, 0.7), 10, " v%i", i);
		}
		// draw/label triangles in cyan 
		int nTriangles = sizeof(triangles) / (3 * sizeof(int));
		for (int i = 0; i < nTriangles; i++) {
			int3 t = triangles[i];
			vec3 p1 = points[t.i1], p2 = points[t.i2], p3 = points[t.i3], c = (p1 + p2 + p3) / 3;
			Line(p1, p2, 1, vec3(0, 1, 1));
			Line(p2, p3, 1, vec3(0, 1, 1));
			Line(p3, p1, 1, vec3(0, 1, 1));
			Text(c, camera.fullview, vec3(0, 1, 1), 10, "t%i", i);
		}
	}

	glDisable(GL_DEPTH_TEST);
	if (!Shift() && glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT))
		camera.arcball.Draw(Control());

	glFlush();
}

void BufferVertices() {
	// assign GPU buffer for points and colors, set it active
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// allocate
	int sPoints = sizeof(points), sColors = sizeof(colors);
	glBufferData(GL_ARRAY_BUFFER, sPoints+sColors, NULL, GL_STATIC_DRAW);
	// cppy points to beginning of buffer, for length of points array
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);
	// copy colors, starting at end of points buffer, for length of colors array
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sColors, colors);
}

void NormalizePoints(float s = 1) {
	// scale and offset so points are in range +/-s, centered at origin 
	vec3 min, max;
	float range = Bounds(points, nPoints, min, max); // in VecMat.h  
	float scale = 2 * s / range;
	vec3 center = (min + max) / 2;
	for (int i = 0; i < nPoints; i++)
		points[i] = scale * (points[i] - center);
}

void MouseButton(float x, float y, bool left, bool down) {
	if (left && down)
		camera.Down(x, y, Shift(), Control());
	else camera.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown)
		camera.Drag(x, y); 
}

void MouseWheel(float spin) {
	camera.Wheel(spin, Shift()); 
}

int main() {
	// init window
	GLFWwindow *w = InitGLFW(100, 100, 800, 800, "Shaded Letter");
	// build shader program
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	// fit the letter
	NormalizePoints(0.8);
	// allocate GPU vertex memory
	BufferVertices();
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Display(w);
		glfwSwapBuffers(w);
		glfwPollEvents();
		// mouse callback routines
		RegisterMouseButton(MouseButton);
		RegisterMouseMove(MouseMove);
		RegisterMouseWheel(MouseWheel); 
	}
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
	glfwDestroyWindow(w);
	glfwTerminate();
}

// ColorfulTriangle.cpp - draw RGB triangle via GLSL, GPU

#include <glad.h>
#include <glfw3.h>
#include "GLXtras.h"
#include "VecMat.h"

GLuint vBuffer = 0; // GPU buffer ID
GLuint program = 0; // GLSL shader program ID

// a triangle (3 2D locations, 3 RGB colors)
// vec2 points[] = { {-.9f, -.6f}, {0.f, .6f}, {.9f, -.6f} };
// vec3 colors[] = { {0, 0, 1}, {1, 0, 0}, {0, 1, 0} };

// the letter "B"
vec3 colors[] = {
	{0.8, 0.2, 0.7},
	{0.5, 0.9, 0.1},
	{0.2, 0.6, 0.9},
	{0.7, 0.3, 0.6},
	{0.9, 0.7, 0.2},
	{0.4, 0.1, 0.8},
	{0.6, 0.4, 0.7},
	{0.1, 0.8, 0.5},
	{0.8, 0.6, 0.1},
	{0.3, 0.7, 0.5}
};

vec2 points[] = {{125, 225}, {50, 50}, {200, 50}, {240, 100}, {240, 175},
{185, 225}, {217, 260}, {217, 315}, {175, 350}, {50, 350}};
int nPoints = sizeof(points) / sizeof(vec2);
int triangles[][3] = {{0,1,2}, {0,2,3}, {0,3,4}, {0,4,5}, {0,5,6}, {0,6,7}, {0,7,8}, {0,8,9}, {0,9,1}};
int nTriangles = sizeof(triangles) / (3 * sizeof(int));

const char *vertexShader = R"(
	#version 130
	in vec2 point;
	in vec3 color;
	out vec3 vColor;
	void main() {
		gl_Position = vec4(point, 0, 1);
		vColor = color;
	}
)";

const char *pixelShader = R"(
	#version 130
	in vec3 vColor;
	out vec4 pColor;
	void main() {
		pColor = vec4(vColor, 1); // 1: fully opaque
	}
)";

void Display() {
	// clear background
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	// run shader program, enable GPU vertex buffer
	glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// connect GPU point and color buffers to shader inputs
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);
	VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));
	// render three vertices as one triangle
	// glDrawArrays(GL_TRIANGLES, 0, 3);
	int nVertices = sizeof(triangles) / sizeof(int);
	glDrawElements(GL_TRIANGLES, nTriangles*3, GL_UNSIGNED_INT, triangles);
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
	vec2 min, max;
	float range = Bounds(points, nPoints, min, max); // in VecMat.h  
	float scale = 2 * s / range;
	vec2 center = (min + max) / 2;
	for (int i = 0; i < nPoints; i++)
		points[i] = scale * (points[i] - center);
}

int main() {
	// init window
	GLFWwindow *w = InitGLFW(100, 100, 800, 800, "Colorful Triangle");
	// build shader program
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	// Fit the letter
	NormalizePoints(0.8);
	// allocate GPU vertex memory
	BufferVertices();
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
	glfwDestroyWindow(w);
	glfwTerminate();
}

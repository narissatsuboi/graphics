// ColorfulTriangle.cpp - draw RGB triangle via GLSL, GPU

#include <glad.h>
#include <glfw3.h>
#include "GLXtras.h"
#include "VecMat.h"

GLuint vBuffer = 0; // GPU buffer ID
GLuint program = 0; // GLSL shader program ID

// a triangle (3 2D locations, 3 RGB colors)
vec2 points[] = { {-.9f, -.6f}, {0.f, .6f}, {.9f, -.6f} };
vec3 colors[] = { {0, 0, 1}, {1, 0, 0}, {0, 1, 0} };

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
	glDrawArrays(GL_TRIANGLES, 0, 3);
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

int main() {
	// init window
	GLFWwindow *w = InitGLFW(100, 100, 800, 800, "Colorful Triangle");
	// build shader program
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
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

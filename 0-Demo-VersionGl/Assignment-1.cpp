// ClearScreen.cpp using OpenGL shader architecture

#include <glad.h>													// OpenGL header file
#include <glfw3.h>													// OpenGL toolkit
#include "GLXtras.h"												// VertexAttribPointer, SetUniform
#include "VecMat.h"													// vec2

GLuint vBuffer = 0;													// GPU vertex buffer ID, valid if > 0
GLuint program = 0;													// shader program ID, valid if > 0

GLFWwindow *w = NULL;
int winWidth = 400, winHeight = 400;								// window size, in pixels

// vertex shader: operations before the rasterizer
const char *vertexShader = R"(
	#version 130
	in vec2 point;													// 2D point from GPU memory
	void main() {
		gl_Position = vec4(point, 0, 1);							// 'built-in' output variable
	}
)";

// pixel shader: operations after the rasterizer
const char *pixelShader = R"(
	#version 130
	out vec4 pColor;
	void main() {
		pColor = vec4(0, 1, 0, 1);									// r, g, b, alpha
	}
)";

void InitVertexBuffer() {
	vec2 v[] = { {-1,-1}, {1,-1}, {1,1}, {-1,1} };					// 1 ccw quad 
//	vec2 v[] = { {-1,-1}, {1,1}, {-1,1}, {-1,-1}, {1,-1}, {1,1} };	// 2 triangles: upper-left, lower-right 
	glGenBuffers(1, &vBuffer);										// ID for GPU buffer
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);							// enable buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);	// copy vertices
}

void Display() {
	glUseProgram(program);											// use shader program
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);							// enable GPU buffer
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);		// connect GPU buffer to vertex shader
	glDrawArrays(GL_QUADS, 0, 4);									// 4 vertices (1 quad)
//	glDrawArrays(GL_TRIANGLES, 0, 6);								// 6 vertices (2 triangles)
	glFlush();														// flush OpenGL ops
}

int main() {														// application entry
	w = InitGLFW(100, 100, winWidth, winHeight, "Clear to Green");
	program = LinkProgramViaCode(&vertexShader, &pixelShader);		// build shader program
	InitVertexBuffer();												// allocate GPU vertex buffer
	while (!glfwWindowShouldClose(w)) {								// event loop
		Display();
		glfwSwapBuffers(w);											// double-buffer is default
		glfwPollEvents();
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}

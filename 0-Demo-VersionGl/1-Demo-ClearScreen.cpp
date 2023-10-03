// ClearScreen.cpp using OpenGL shader architecture

#include <glad.h>													// OpenGL header file
#include <glfw3.h>													// OpenGL toolkit
#include "GLXtras.h"												// VertexAttribPointer, SetUniform
#include "VecMat.h"													// vec2

vec3 userColor(0, 1, 0);											// r, g, b 
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
	uniform vec3 userColor = vec3(0, 1, 0);                         // default is green 
	void main() {
		pColor = vec4(userColor, 1);  // rgb, alpha 
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
	SetUniform(program, "userColor", userColor);
	glDrawArrays(GL_QUADS, 0, 4);									// 4 vertices (1 quad)
//	glDrawArrays(GL_TRIANGLES, 0, 6);								// 6 vertices (2 triangles)
	glFlush();														// flush OpenGL ops
}

void Keyboard(int key, bool press, bool shift, bool control) {
	if (press && key == 'C') {
		vec3 c;
		printf("type r g b (range 0-1, no commas): ");
		if (fscanf(stdin, "%f%f%f", &c.x, &c.y, &c.z) == 3) {
			userColor = c;
			char title[500];
			sprintf(title, "Clear to (%g,%g,%g)", c.x, c.y, c.z);
			glfwSetWindowTitle(w, title);
		}
		rewind(stdin);
	}
}

int main() {														// application entry
	w = InitGLFW(100, 100, winWidth, winHeight, "Clear to Green");
	program = LinkProgramViaCode(&vertexShader, &pixelShader);		// build shader program
	InitVertexBuffer();												// allocate GPU vertex buffer
	RegisterKeyboard(Keyboard);										// callback for user key press 
	while (!glfwWindowShouldClose(w)) {								// event loop
		Display();
		glfwSwapBuffers(w);											// double-buffer is default
		glfwPollEvents();
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}

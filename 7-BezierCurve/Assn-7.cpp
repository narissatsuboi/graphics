// Assn-7.cpp: Bezier Curbe

#include <vector>
#include <glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "IO.h"
#include "Text.h"
#include "VecMat.h"
#include "Widgets.h"


// display
int winWidth = 800, winHeight = 800;
Camera camera(0, 0, winWidth, winHeight, vec3(15, -15, 0), vec3(0, 0, -5), 30);
bool showDebugView = false;

// movable lights       
vec3 lights[] = { {.5, 0, 1}, {1, 1, 0} };
const int nLights = sizeof(lights) / sizeof(vec3);

// interaction
void* picked = NULL;
Mover mover;

// Displauy

void Display(GLFWwindow* w) {

	glFlush();
}

// Mouse Callbacks

void MouseButton(float x, float y, bool left, bool down) {
	picked = NULL;
	if (left && down) {
		// light picked?
		for (int i = 0; i < nLights; i++)
			if (MouseOver(x, y, lights[i], camera.fullview)) {
				picked = &mover;
				mover.Down(&lights[i], (int)x, (int)y, camera.modelview, camera.persp);
			}
		if (picked == NULL) {
			picked = &camera;
			camera.Down(x, y, Shift(), Control());
		}
	}
	else camera.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown) {
		if (picked == &mover)
			mover.Drag((int)x, (int)y, camera.modelview, camera.persp);
		if (picked == &camera)
			camera.Drag(x, y);
	}
}

void MouseWheel(float spin) {
	camera.Wheel(spin, Shift());
}

// Initialization

void BufferVertices() {
	// create GPU buffer, make it active
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// allocate/load memory for points and uvs
	size_t sPoints = points.size() * sizeof(vec3), sUvs = uvs.size() * sizeof(vec2), sNormals = normals.size() * sizeof(vec3);;
	size_t sTotal = sPoints + sUvs + sNormals;
	glBufferData(GL_ARRAY_BUFFER, sTotal, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points.data());
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sUvs, uvs.data());
	glBufferSubData(GL_ARRAY_BUFFER, sPoints + sUvs, sNormals, normals.data());
}

// Application

void Resize(int width, int height) {
	camera.Resize(width, height);
	glViewport(0, 0, width, height);
}

int main(int ac, char** av) {

	// enable anti-alias, init app window and GL context
	GLFWwindow* w = InitGLFW(100, 100, winWidth, winHeight, "Bumpy Mesh");

	// callbacks
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		glfwPollEvents();
		Display(w);
		glfwSwapBuffers(w);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glfwDestroyWindow(w);
	glfwTerminate();
}

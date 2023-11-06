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

// control points
vec3 controlPoints[] = { {.8f, .5f, .0f} ,{.8f, -.5f, .0f} ,{-.8f, .5f, .0f} ,{-.8f, -.5f, .0f} };
vec3 controlPointsColor = { 0, 1, 0 }, curvePointColor = { 1, 0, 0 };
const int nControlPoints = sizeof(controlPoints) / sizeof(vec3);

// lines
float outlineWidth = 1;
vec3 controLineColor = { 0, 0, 1 }, curveLineColor1 = { 1, 0, 0 }, curveLineColor2 = { 1, 1, 0 };

// interaction
void* picked = NULL;
void* hover = NULL;
Mover mover;
int resolution = 25; 
time_t tEvent = clock(); 


class Bezier {
public:
	vec3 controlPoints;
	vec3 p1, p2, p3, p4;    // control points
	int res;                // display resolution
	
	Bezier(const vec3& controlPoints, int res = 50) : controlPoints(controlPoints), res(res) { 
		this->p1 = this->controlPoints[0]; 
		this->p2 = this->controlPoints[1]; 
		this->p3 = this->controlPoints[2]; 
		this->p4 = this->controlPoints[3]; 
	}

	vec3 Point(float t) {
		// return a point on the Bezier curve given parameter t, in (0,1)
		float t2 = t * t, t3 = t * t2, T = 1 - t, T2 = T * T, T3 = T * T2;
		return T3 * p1 + (3 * t * T2) * p2 + (3 * t2 * T) * p3 + t3 * p4;
	}

	void DrawBezierCurve(vec3 color, float width) {
		// break the curve into res number of straight pieces
		// *** render each piece with Line() ***
		
		for (int i = 0; i < res; i++) {
			Line(Point((float)i / this->res), Point((float) i + 1 / this->res), width, color);
		}
	}
	void DrawControlPolygon(vec3 pointColor, vec3 meshColor, float opacity, float width) {
		// draw the four control points and the mesh that connects them
		for (int i = 0; i < nControlPoints; i++) {
			Disk((vec3) this->controlPoints[i], width, controlPointsColor); 
			if (i == 0)
				continue; 
			LineDash(this->controlPoints[i - 1], this->controlPoints[i], outlineWidth, curveLineColor1, curveLineColor2);
		}
	}
	vec3* PickPoint(int x, int y, mat4 view) {
		// return pointer to nearest control point, if within 10 pixels of mouse (x,y), else NULL
		// hint: use ScreenDistSq
		return NULL;
	}
};

Bezier* curve = new Bezier(* controlPoints, resolution);

// Display

void Display(GLFWwindow* w) {
	// background, blending, zbuffer
	glClearColor(.6f, .6f, .6f, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	// draw curve and control polygon
	UseDrawShader(camera.fullview); // no shading, so single matrix
	curve.Draw(vec3(.7f, .2f, .5f), 3.5f);
	curve.DrawControlPolygon(vec3(0, .4f, 0), vec3(1, 1, 0), 1, 2.5f);
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


// Application

void Resize(int width, int height) {
	camera.Resize(width, height);
	glViewport(0, 0, width, height);
}

int main(int ac, char** av) {
	// init app window and GL context
	glfwInit();
	GLFWwindow* w = glfwCreateWindow(winWidth, winHeight, "Bezier Curve", NULL, NULL);
	glfwSetWindowPos(w, 100, 100);
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	// callbacks
	glfwSetCursorPosCallback(w, MouseMove);
	glfwSetMouseButtonCallback(w, MouseButton);
	glfwSetScrollCallback(w, MouseWheel);
	glfwSetWindowSizeCallback(w, Resize);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Display(w);
		glfwPollEvents();
		glfwSwapBuffers(w);
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}

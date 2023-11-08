// Assn-7.cpp: Bezier Curve with 4 Control Points by Narissa Tsuboi

#include <vector>
#include <time.h>
#include <glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "IO.h"
#include "Text.h"
#include "VecMat.h"
#include "Widgets.h"

time_t startTime = clock();

class Bezier {
private: 
	vector<vec3> ctrlPoints;
	vec3 lineColor = { 0, 0, 1 };
	vec3 curveColor = { 0.75, 0, 0 };
	float resolution = 666;
	float width = 3.0;
	float opacity = 1.0;

	int nCtrlPoints = 0; 

	const vec3 pointColor = { 0, 1, 0 };
	const vec3 dotColor = { 1, 0, 0 };
	const float ctrlPointThickness = 12.0;
	const float dotThickness = 12.0;
	const float duration = 4.0;

public:
	Bezier(vec3 points[]) {
		for (int i = 0; i < 4; i++) 
			this->ctrlPoints.push_back(points[i]);
		this->ctrlPoints = ctrlPoints;
		this->nCtrlPoints = ctrlPoints.size();
	}

	Bezier(vec3 points[], vec3 lineColor, vec3 curveColor, float resolution, float width, float opacity) {
		for (int i = 0; i < 4; i++) 
			this->ctrlPoints.push_back(points[i]);
		this->nCtrlPoints = ctrlPoints.size();
		this->lineColor = lineColor;
		this->curveColor = curveColor;
		this->resolution = resolution; 
		this->width = width;
		this->opacity = opacity;
	}

	vec3 Point(float t) {
		float t2 = t * t, t3 = t * t2, x = 1 - t, x2 = x * x, x3 = x * x2;
		return x3 * ctrlPoints[0] + (3 * t * x2) * ctrlPoints[1] + (3 * t2 * x) * ctrlPoints[2] + t3 * ctrlPoints[3];
	}

	void DrawBezierCurve() {
		for (int i = 0; i < resolution; i++)  
			Line(Point((float) (i + 1) / resolution), Point((float) i / resolution), width, curveColor, opacity);
	}

	void DrawControlPolygon() {
		for (int i = 0; i < nCtrlPoints - 1; i++)
			LineDash(ctrlPoints[i], ctrlPoints[i + 1], width, lineColor, lineColor, opacity);
	}

	void DrawControlPoints() {
		for (int i = 0; i < nCtrlPoints; i++)
			Disk(ctrlPoints[i], ctrlPointThickness, pointColor, opacity);
	}

	void DrawMovingDot() {
		const float PI = 3.1415;
		float elapsedTime = (float)(clock() - startTime) / CLOCKS_PER_SEC;
		float t = (float)(sin(2 * PI * elapsedTime / duration) + 1) / 2;
		Disk(Point(t), dotThickness, dotColor);
	}
};

GLuint program = 0;

int winWidth = 1000, winHeight = 1000;
Camera camera(0, 0, winWidth, winHeight, vec3(15, -15, 0), vec3(0, 0, -5), 30);

Mover mover;
void* picked = NULL;

vec3 cps[] = { {-1.0f, 0.5f, 0.0f} ,{-1.0f, -0.5f, 0.0f}, {1.0f, 0.5f, 0.0f} ,{1.0f, -0.5f, 0.0f} };
const int nCps = sizeof(cps) / sizeof(vec3);

void Display(GLFWwindow* w) {
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);

	vec3 xControlPoints[nCps];
	for (int i = 0; i < nCps; i++) {
		vec4 xControlPoint = camera.modelview * vec4(cps[i], 1);
		xControlPoints[i] = vec3((float*)&xControlPoint);
	}

	UseDrawShader(camera.fullview);
	Bezier bc = Bezier(cps);
	bc.DrawControlPolygon();
	bc.DrawControlPoints();
	bc.DrawBezierCurve();
	bc.DrawMovingDot(); 
	glFlush();
}

void MouseButton(float x, float y, bool left, bool down) {
	picked = NULL;
	if (left && down) {
		for (int i = 0; i < nCps; i++)
			if (MouseOver(x, y, cps[i], camera.fullview)) {
				picked = &mover;
				mover.Down(&cps[i], (int)x, (int)y, camera.modelview, camera.persp);
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

void Resize(int width, int height) {
	camera.Resize(width, height);
	glViewport(0, 0, width, height);
}

int main(int ac, char** av) {
	GLFWwindow* w = InitGLFW(100, 100, winWidth, winHeight, "Bezier Curve - 4 Control Points");

	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);

	while (!glfwWindowShouldClose(w)) {
		glfwPollEvents();
		Display(w);
		glfwSwapBuffers(w);
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}
// SmoothMesh.cpp: texture-map 3D letter

#include <glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "IO.h"
#include "VecMat.h"
#include "Widgets.h"

// display
int winWidth = 800, winHeight = 800;
Camera camera(0, 0, winWidth, winHeight, vec3(15, -15, 0), vec3(0, 0, -5), 30);

// vertex coordinates for position and texture
vec3 points[] = {
	// front letter 'B', z = 0
	{125, 225, 0}, {50, 50, 0}, {200, 50, 0}, {240, 100, 0}, {240, 175, 0},
	{185, 225, 0}, {217, 260, 0}, {217, 315, 0}, {175, 350, 0}, {50, 350, 0},
	// back, z = -50
	{125, 225, -50}, {50, 50, -50}, {200, 50, -50}, {240, 100, -50}, {240, 175, -50},
	{185, 225, -50}, {217, 260, -50}, {217, 315, -50}, {175, 350, -50}, {50, 350, -50}
};
const int nPoints = sizeof(points)/sizeof(vec3);
vec2 uvs[nPoints];

// triangles
int triangles[][3] = {
	// 9 front
	{0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 5},
	{0, 5, 6}, {0, 6, 7}, {0, 7, 8}, {0, 8, 9}, {0, 9, 1},
	// 9 back
	{12, 11, 10}, {13, 12, 10}, {14, 13, 10}, {15, 14, 10},
	{16, 15, 10}, {17, 16, 10}, {10, 18, 17}, {10, 19, 18}, {10, 11, 19},
	// 18 side
	{1, 12, 2}, {1, 11, 12}, {2, 13, 3}, {2, 12, 13}, {3, 14, 4}, {3, 13, 14}, {4, 15, 5}, {4, 14, 15},
	{5, 16, 6}, {5, 15, 16}, {6, 17, 7}, {6, 16, 17}, {7, 18, 8}, {7, 17, 18}, {8, 19, 9}, {8, 18, 19}, {9, 11, 1}, {9, 19, 11}
};
const int nTriangles = sizeof(triangles)/(3*sizeof(int));

// OpenGL IDs for vertex buffer, shader program
GLuint vBuffer = 0, program = 0;

// texture image
const char *texFilename = "texture_img.jpg";
GLuint textureName = 0;
int textureUnit = 0;

// movable lights       
vec3 lights[] = { {.5, 0, 1}, {1, 1, 0} };
const int nLights = sizeof(lights)/sizeof(vec3);

// interaction
void *picked = NULL;
Mover mover;

// Shaders

const char *vertexShader = R"(
	#version 130
	in vec3 point;
	in vec2 uv;
	out vec3 vPoint;
	out vec2 vUv;
	uniform mat4 modelview, persp;
	void main() {
		vPoint = (modelview*vec4(point, 1)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
		vUv = uv;
	}
)";

const char *pixelShader = R"(
	#version 130
	in vec3 vPoint;
	in vec2 vUv;
	out vec4 pColor;
	uniform sampler2D textureImage;
	uniform int nLights = 0;
	uniform vec3 lights[20];
	uniform float amb = .1, dif = .8, spc =.7;					// ambient, diffuse, specular
	void main() {
		float d = 0, s = 0;
		vec3 dx = dFdx(vPoint), dy = dFdy(vPoint);				// change in vPoint in horiz/vert directions
		vec3 N = normalize(cross(dx, dy));						// unit-length facet normal
		vec3 E = normalize(vPoint);								// eye vector
		for (int i = 0; i < nLights; i++) {
			vec3 L = normalize(lights[i]-vPoint);				// light vector
			vec3 R = reflect(L, N);								// highlight vector
			d += max(0, dot(N, L));								// one-sided diffuse
			float h = max(0, dot(R, E));						// highlight term
			s += pow(h, 100);									// specular term
		}
		float ads = clamp(amb+dif*d+spc*s, 0, 1);
		pColor = vec4(ads*texture(textureImage, vUv).rgb, 1);
	}
)";

// Display

void Display(GLFWwindow *w) {
	// clear screen, enable blend, z-buffer
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// init shader program, connect GPU buffer to vertex shader
	glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	VertexAttribPointer(program, "point", 3, 0, (void *) 0);
	VertexAttribPointer(program, "uv", 2, 0, (void *) sizeof(points));
	// update matrices
	SetUniform(program, "modelview", camera.modelview);
	SetUniform(program, "persp", camera.persp);
	// transform and update lights
	vec3 xLights[nLights];
	for (int i = 0; i < nLights; i++) {
		vec4 xLight = camera.modelview*vec4(lights[i], 1);
		xLights[i] = vec3((float*) &xLight);
	}
	SetUniform(program, "nLights", nLights);
	SetUniform3v(program, "lights", nLights, (float *) xLights);
	// bind textureName to textureUnit
	glBindTexture(GL_TEXTURE_2D, textureName);
	glActiveTexture(GL_TEXTURE0+textureUnit);
	SetUniform(program, "textureImage", textureUnit);
	// render
	glDrawElements(GL_TRIANGLES, 3*nTriangles, GL_UNSIGNED_INT, triangles);
	// annotation
	glDisable(GL_DEPTH_TEST);
	UseDrawShader(camera.fullview);
	for (int i = 0; i < nLights; i++)
		Star(lights[i], 8, vec3(1, .8f, 0), vec3(0, 0, 1));
	if (picked == &camera && !Shift())
		camera.arcball.Draw(Control());
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
				mover.Down(&lights[i], (int) x, (int) y, camera.modelview, camera.persp);
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
			mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
		if (picked == &camera)
			camera.Drag(x, y);
	}
}

void MouseWheel(float spin) {
	camera.Wheel(spin, Shift());
}

// Initialization

void SetUvs() {
	vec3 min, max;
	Bounds(points, nPoints, min, max);
	vec3 dif(max-min);
	for (int i = 0; i < nPoints; i++)
		uvs[i] = vec2((points[i].x-min.x)/dif.x, (points[i].y-min.y)/dif.y);
}

void BufferVertices() {
	// create GPU buffer, make it active
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// allocate/load memory for points and uvs
	int sPoints = sizeof(points), sUvs = sizeof(uvs);
	glBufferData(GL_ARRAY_BUFFER, sPoints+sUvs, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sUvs, uvs);
}

// Application

void WriteObjFile(const char *filename) {
	FILE *file = fopen(filename, "w");
	if (!file)
		printf("can't save %s\n", filename);
	else {
		fprintf(file, "\n# %i vertices\n", nPoints);
		for (int i = 0; i < nPoints; i++)
			fprintf(file, "v %f %f %f \n", points[i].x, points[i].y, points[i].z);
		fprintf(file, "\n# %i textures\n", nPoints);
		for (int i = 0; i < nPoints; i++)
			fprintf(file, "vt %f %f \n", uvs[i].x, uvs[i].y);
		fprintf(file, "\n# %i triangles\n", nTriangles);
		for (int i = 0; i < nTriangles; i++)
			fprintf(file, "f %i %i %i \n", 1+triangles[i][0], 1+triangles[i][1], 1+triangles[i][2]); // OBJ format
		fclose(file);
		printf("%s written\n", filename);
	}
}

void Keyboard(int key, bool press, bool shift, bool control) {
	if (press && key == 'S')
		WriteObjFile("C:/Users/Jules/Code/G-Assns/LetterB.obj");
}

void Resize(int width, int height) {
	camera.Resize(width, height);
	glViewport(0, 0, width, height);
}

int main(int ac, char **av) {
	// enable anti-alias, init app window and GL context
	GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "Textured Letter");
	// init shader program, set GPU buffer, read texture image
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	SetUvs();
	Standardize(points, nPoints, .8f);
	BufferVertices();
	textureName = ReadTexture(texFilename);
	// callbacks
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	RegisterKeyboard(Keyboard);
	printf("Usage: S to save as OBJ file\n");
	// event loop
	while (!glfwWindowShouldClose(w)) {
		glfwPollEvents();
		Display(w);
		glfwSwapBuffers(w);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
	glfwDestroyWindow(w);
	glfwTerminate();
}

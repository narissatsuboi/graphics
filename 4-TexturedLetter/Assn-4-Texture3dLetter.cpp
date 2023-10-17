// Texture3dLetter.cpp: facet-shade extruded letter 

#include <glad.h>
#include <glfw3.h>
#include "GLXtras.h"
#include "VecMat.h"
#include "Text.h"
#include "Camera.h"
#include "Draw.h"  // ScreenD, Star 
#include "IO.h"   // ReadTexture 
#include "Widgets.h" // Mover 

GLuint vBuffer = 0; // GPU buffer ID
GLuint program = 0; // GLSL shader program ID

/** globals ************************************************************************************/

/** 
	letter geometry globals 
*/
float zDepth = -15.0f;

vec3 points[] = { 
	{0.0f, 0.0f, 0.0f},{0.0f, 80.0f, 0.0f},{50.0f, 0.0f, 0.0f},{50.0f, 20.0f, 0.0f},{20.0f, 20.0f, 0.0f},{20.0f, 80.0f, 0.0f},
	{0.0f, 0.0f, zDepth},{0.0f, 80.0f, zDepth},{50.0f, 0.0f, zDepth},{50.0f, 20.0f, zDepth},{20.0f, 20.0f, zDepth},{20.0f, 80.0f, zDepth}
};

const int nPoints = sizeof(points) / sizeof(vec3);

int triangles[][3] = {
	{0, 2, 3},{0, 3, 4},{0, 4, 5}, {0, 1, 5},  {6, 9, 8}, {6, 10, 9},{6, 11, 10},{6, 11, 7},{0, 7, 6}, {0, 1, 7},
	{4, 10, 11},{4, 11, 5},{2, 8, 9},{2, 9, 3},{1, 5, 11},{1, 7, 11},{0, 2, 8},{0, 6, 8},{4, 10, 9},{4, 3, 9}
};

int nTriangles = sizeof(triangles) / (3 * sizeof(int));


/**
	texture globals 
*/
vec2 uvs[nPoints]; 
const char* textureFilename = "texture_img.jpg";
GLuint textureName = 0;  // id for texture image, set by ReadTexture 
int textureUnit = 0;    // id for GPU image buffer, may be freely set 

/**
	camera globals
*/
int winWidth = 800;
int winHeight = 800;
Camera camera(0, 0, winWidth, winHeight, vec3(15, -30, 0), vec3(0, 0, -5), 30);

/**
	lighting globals
*/
vec3 lights[] = { {.5, 0, 1}, {1, 1, 0} };  // movable lights 
const int nLights = sizeof(lights) / sizeof(vec3);
Mover mover;         // to move light 
void* picked = NULL; // user selection (&mover or null/camera) 

/** methods **********************************************************************************/

/** 
	texture methods 
*/
void SetUvs() {
	vec3 min, max;
	Bounds(points, nPoints, min, max);
	vec3 dif(max - min);
	for (int i = 0; i < nPoints; i++)
		uvs[i] = vec2((points[i].x - min.x) / dif.x, (points[i].y - min.y) / dif.y);
}

/** 
	shaders 
*/
const char *vertexShader = R"(
	#version 130
	uniform mat4 modelview, persp; 
	in vec3 point;
	out vec3 vPoint; 
	in vec2 uv; 
	out vec2 vUv; 
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

	uniform int nLights = 0; 
	uniform vec3 lights[20];
	uniform sampler2D textureImage; 
	uniform float amb = .1, dif = .8, spc =.7; 
	uniform bool highlights = true; 	

	void main() {
		
		vec3 dx = dFdx(vPoint), dy = dFdy(vPoint);  
		vec3 N = normalize(cross(dx, dy));          
		vec3 E = normalize(vPoint);                 
                 
		// init diffuse and spec to 0, increment along nLights 
		float d = 0.0; float s = 0.0;                  
        for (int i = 0; i < nLights; i++) {
            vec3 L = normalize(lights[i] - vPoint);
            d += abs(dot(N, L)); 
            vec3 R = reflect(L, N);                       
            float h = max(0.0, dot(R, E));               
            s += pow(h, 100.0); 
        }
		float intensity = min(1, amb+dif*d)+spc*s;  
		vec3 col = texture(textureImage, vUv).rgb; 
		pColor = vec4(intensity*col, 1);    
	}
)";

void Display(GLFWwindow *w) {

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// clear background
	glClearColor(1, 1, 1, 1);

	// run shader program, enable GPU vertex buffer
	glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);

	// send modelview and perspective matrices to vertex shader
	SetUniform(program, "modelview", camera.modelview);
	SetUniform(program, "persp", camera.persp);

	// connect GPU point and uv coordinates to shader inputs
	VertexAttribPointer(program, "point", 3, 0, (void *) 0);
	VertexAttribPointer(program, "uv", 2, 0, (void*)sizeof(points));

	// Inform pixel shade which image buffer to read from 
	SetUniform(program, "textureImage", textureUnit);

	// enable GPU buffer of uv coordinates
	glBindTexture(GL_TEXTURE_2D, textureName);
	glActiveTexture(GL_TEXTURE0 + textureUnit);

	// transform lights, send to GPU 
	vec3 xLights[nLights];
	for (int i = 0; i < nLights; i++)
		xLights[i] = Vec3(camera.modelview * vec4(lights[i], 1));
	SetUniform(program, "nLights", nLights);
	SetUniform3v(program, "lights", nLights, (float*)xLights);

	// render three vertices as one triangle
	glDrawElements(GL_TRIANGLES, nTriangles*3, GL_UNSIGNED_INT, triangles);

	// draw lights as disks 
	UseDrawShader(camera.fullview);
	// draw lights 
	for (int i = 0; i < nLights; i++)
		Star(lights[i], 8, vec3(1, .8f, 0), vec3(0, 0, 1));

	if (!Shift() && glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT))
		camera.arcball.Draw(Control());

	glFlush();
}

void BufferVertices() {
	// assign GPU buffer for points and colors, set it active
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// allocate
	int sUvs = nPoints * sizeof(vec2);
	int sPoints = sizeof(points);
	
	glBufferData(GL_ARRAY_BUFFER, sPoints + sUvs, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sUvs, uvs);

	// cppy points to beginning of buffer, for length of points array
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);

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

/**
	mouse
*/
void MouseButton(float x, float y, bool left, bool down) {
	picked = NULL;

	if (picked == NULL) {
		picked = &camera;
		camera.Down((int)x, (int)y, Shift(), Control());
	}

	for (int i = 0; i < nLights; i++) {
		if (MouseOver(x, y, lights[i], camera.fullview)) {
			picked = &mover;
			mover.Down(&lights[i], (int)x, (int)y, camera.modelview, camera.persp);
		}
	}
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (picked == &mover)
		mover.Drag((int)x, (int)y, camera.modelview, camera.persp);
	if (picked == &camera)
		camera.Drag((int)x, (int)y);
}

void MouseWheel(float spin) {
	camera.Wheel(spin, Shift()); 
}

void Resize(int width, int height) {
	camera.Resize(width, height);
}


/**
	IO
*/
void WriteObjFile(const char* filename) {
	FILE* file = fopen(filename, "w");
	if (file) {
		fprintf(file, "\n# %i vertices\n", nPoints);
		for (int i = 0; i < nPoints; i++)
			fprintf(file, "v %f %f %f \n", points[i].x, points[i].y, points[i].z);
		fprintf(file, "\n# %i textures\n", nPoints);
		for (int i = 0; i < nPoints; i++)
			fprintf(file, "vt %f %f \n", uvs[i].x, uvs[i].y);
		fprintf(file, "\n# %i triangles\n", nTriangles);
		for (int i = 0; i < nTriangles; i++)
			fprintf(file, "f %i %i %i \n",
				1 + triangles[i][0], 1 + triangles[i][1], 1 + triangles[i][2]);
		fclose(file);
	}
}


int main() {
	
	// write to file 
	const char* fileName = "output.obj";
	WriteObjFile(fileName);

	GLFWwindow *w = InitGLFW(100, 100, 800, 800, "Shaded Letter");
	
	program = LinkProgramViaCode(&vertexShader, &pixelShader);  // build shader program
	textureName = ReadTexture(textureFilename);  // read and store texture img in GPU
	SetUvs();                                    // init uv coords 
	NormalizePoints(0.8);                        // fit the letter
	BufferVertices();                            // allocate GPU vertex memory

	while (!glfwWindowShouldClose(w)) {
		Display(w);
		glfwSwapBuffers(w);
		glfwPollEvents();
		RegisterResize(Resize);
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

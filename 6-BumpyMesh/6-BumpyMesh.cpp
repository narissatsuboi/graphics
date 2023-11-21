// BumpMap.cpp: bumpy object 

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

// dynamic arrays
vector<vec3> points;        // vertex locations 
vector<vec3> normals;       // surface normals 
vector<vec2> uvs;           // texture coordinates 
vector<int3> triangles;     // triplets of vertex indices 

// OpenGL IDs for vertex buffer, shader program
GLuint vBuffer = 0, program = 0;

// obj file
const char* objFilename = "Fish.obj";

// texture image
const char* texFilename = "Monster_Color.jpg";
GLuint textureName = 0;
int textureUnit = 0;

// bump map 
const char* bumpFilename = "Monster_Normal.jpg";
GLuint bumpName = 0;
int bumpUnit = 1;

// movable lights       
vec3 lights[] = { {.5, 0, 1}, {1, 1, 0} };
const int nLights = sizeof(lights) / sizeof(vec3);

// interaction
void* picked = NULL;
Mover mover;

// Shaders

const char* vertexShader = R"(
	#version 130
	in vec3 point;
	in vec2 uv;
	in vec3 normal;
	out vec3 vPoint;
	out vec2 vUv;
	out vec3 vNormal;
	uniform mat4 modelview, persp;
	void main() {
		vPoint = (modelview*vec4(point, 1)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
		vUv = uv;
		vNormal = (modelview*vec4(normal, 0)).xyz;
	}
)";

const char* pixelShader = R"(
    #version 130
    in vec3 vPoint;
    in vec2 vUv;
    in vec3 vNormal; 
    out vec4 pColor;
    uniform sampler2D textureImage;
    uniform sampler2D bumpMap;
    uniform int nLights;
    uniform vec3 lights[20];
    
    uniform float amb = 0.1;
    uniform float dif = 0.8;
    uniform float spc = 0.7;
    
    void main() {
        vec3 dx = dFdx(vPoint), dy = dFdy(vPoint);
        vec2 du = dFdy(vUv), dv = dFdx(vUv);
        
        vec3 X = normalize(du.x * dx + du.y * dy); 
        vec3 Y = normalize(dv.x * dx + dv.y * dy);
        vec3 Z = normalize(vNormal);
        vec4 t = texture(bumpMap, vUv);
        vec3 b = vec3(2.0 * t.r - 1.0, 2.0 * t.g - 1.0, t.b);
        vec3 N = normalize(b.x * X + b.y * Y + b.z * Z); 
        
        float d = 0.0, s = 0.0;
        vec3 E = normalize(vPoint);  
        for (int i = 0; i < nLights; i++) {
            vec3 L = normalize(lights[i] - vPoint);  
            vec3 R = reflect(L, N);  
            d += max(0.0, dot(N, L));  
            float h = max(0.0, dot(R, E));  
            s += pow(h, 100.0);  
        }
        
        float ads = clamp(amb + dif * d + spc * s, 0.0, 1.0);
        pColor = vec4(ads * texture(textureImage, vUv).rgb, 1.0);
    }
)";


// Display

void Display(GLFWwindow* w) {
	// clear screen, enable blend, z-buffer
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// init shader program, connect GPU buffer to vertex shader
	glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// [point, uv, normal]
	const size_t pointsOffset = points.size() * sizeof(vec3);
	const size_t pointsUvOffeset = pointsOffset + uvs.size() * sizeof(vec2);
	VertexAttribPointer(program, "point", 3, 0, (GLvoid*)0);
	VertexAttribPointer(program, "uv", 2, 0, (GLvoid*)pointsOffset);
	VertexAttribPointer(program, "normal", 3, 0, (GLvoid*)pointsUvOffeset);
	// update matrices
	SetUniform(program, "modelview", camera.modelview);
	SetUniform(program, "persp", camera.persp);
	// transform and update lights
	vec3 xLights[nLights];
	for (int i = 0; i < nLights; i++) {
		vec4 xLight = camera.modelview * vec4(lights[i], 1);
		xLights[i] = vec3((float*)&xLight);
	}
	SetUniform(program, "nLights", nLights);
	SetUniform3v(program, "lights", nLights, (float*)xLights);
	// bind textureName to textureUnit and bumpMap to bumpUnit
	SetUniform(program, "textureImage", textureUnit);
	glBindTexture(GL_TEXTURE_2D, textureName);

	glActiveTexture(GL_TEXTURE0 + bumpUnit);
	glBindTexture(GL_TEXTURE_2D, bumpName);
	SetUniform(program, "bumpMap", bumpUnit);
	// render
	glDrawElements(GL_TRIANGLES, (GLsizei)3 * triangles.size(), GL_UNSIGNED_INT, triangles.data());
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
	// read OBJ file
	if (!ReadAsciiObj(objFilename, points, triangles, &normals, &uvs))
		printf("can’t read %s\n", objFilename);
	else
		printf("opened %s\n", objFilename);

	// enable anti-alias, init app window and GL context
	GLFWwindow* w = InitGLFW(100, 100, winWidth, winHeight, "Bumpy Mesh");
	// init shader program, set GPU buffer, read texture image
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	Standardize(points.data(), (int)points.size(), .8f);

	BufferVertices();
	textureName = ReadTexture(texFilename);
	printf("opened %s\n", texFilename);
	bumpName = ReadTexture(bumpFilename);
	printf("opened %s\n", bumpFilename);

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
	glDeleteBuffers(1, &vBuffer);
	glfwDestroyWindow(w);
	glfwTerminate();
}

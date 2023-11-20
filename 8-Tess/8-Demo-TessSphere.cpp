// TessSphere.cpp - use tessellation shader to display a texture-mapped sphere

#include <glad.h>
#include <GLFW/glfw3.h>
#include <Time.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "IO.h"
#include "Widgets.h"

// display parameters
int         winWidth = 800, winHeight = 600;
Camera		camera(0, 0, winWidth, winHeight, vec3(0, 0, 0), vec3(0, 0, -6));
GLuint      program = 0;

// texture
GLuint		textureName = 0;
int			textureUnit = 0;
const char *textureFilename = "katsbits-rock5/rocks_4.tga";

// interaction
vec3        light(-1.4f, 1.f, 1.f);
void       *picked = NULL;
Mover       mover;

// vertex shader (no op)
const char *vShader = R"(
	#version 130
	void main() { };
)";

time_t startTime = clock();
float PI = 3.141592;
float duration = 4.0; 

vec3        ctrlPts[] = {
	{-1.5f, -1.5f, 0}, {-.5f, -1.5f,   0}, {.5f, -1.5f,   0}, {1.5f, -1.5f, 0},
	{-1.5f,  -.5f, 0}, {-.5f,  -.5f, .7f}, {.5f,  -.5f, .7f}, {1.5f,  -.5f, 0},
	{-1.5f,   .5f, 0}, {-.5f,   .5f, .7f}, {.5f,   .5f, .7f}, {1.5f,   .5f, 0},
	{-1.5f,  1.5f, 0}, {-.5f,  1.5f,   0}, {.5f,  1.5f,   0}, {1.5f,  1.5f, 0}
};

// tessellation evaluation shader
const char* teShader = R"(
	#version 400 core 
    layout (quads, equal_spacing, ccw) in; 
    uniform vec3 ctrlPts[16]; 
    uniform mat4 modelview, persp; 
    out vec3 point, normal; 
    out vec2 uv; 
    vec3 BezTangent(float t, vec3 b1, vec3 b2, vec3 b3, vec3 b4) { 
        float t2 = t*t; 
        return (-3*t2+6*t-3)*b1+(9*t2-12*t+3)*b2+(6*t-9*t2)*b3+3*t2*b4; 
    } 
    vec3 BezPoint(float t, vec3 b1, vec3 b2, vec3 b3, vec3 b4) { 
        float t2 = t*t, t3 = t*t2; 
        return (-t3+3*t2-3*t+1)*b1+(3*t3-6*t2+3*t)*b2+(3*t2-3*t3)*b3+t3*b4; 
    } 
    void main() { 
        vec3 spts[4], tpts[4]; 
        uv = gl_TessCoord.st; 
 // define a curve in s-direction and one in t-direction 
        for (int i = 0; i < 4; i++) { 
           spts[i] = BezPoint(uv.s, ctrlPts[4*i], ctrlPts[4*i+1], ctrlPts[4*i+2], ctrlPts[4*i+3]); 
           tpts[i] = BezPoint(uv.t, ctrlPts[i], ctrlPts[i+4], ctrlPts[i+8], ctrlPts[i+12]); 
        } 
        // compute point on patch and transform by modelview 
        vec3 p = BezPoint(uv.t, spts[0], spts[1], spts[2], spts[3]); 
        point = (modelview*vec4(p, 1)).xyz; 
        // compute tangents in s-direction and t-direction 
        vec3 tTan = BezTangent(uv.t, spts[0], spts[1], spts[2], spts[3]); 
        vec3 sTan = BezTangent(uv.s, tpts[0], tpts[1], tpts[2], tpts[3]); 
 // compute normal as cross-product of tangents and transform by modelview 
        vec3 n = normalize(cross(sTan, tTan)); 
        normal = (modelview*vec4(n, 0)).xyz; 
        gl_Position = persp*vec4(point, 1); 
    
)";


// pixel shader
const char *pShader = R"(
	#version 130
	in vec3 point, normal;
	in vec2 uv;
	out vec4 pColor;
	uniform sampler2D textureMap;
	uniform vec3 light;
	void main() {
		vec3 N = normalize(normal);					// surface normal
		vec3 L = normalize(light-point);			// light vector
		vec3 E = normalize(point);					// eye vertex
		vec3 R = reflect(L, N);						// highlight vector
		float dif = max(0, dot(N, L));				// one-sided diffuse
		float spec = pow(max(0, dot(E, R)), 50);
		float ad = clamp(.15+dif, 0, 1);
		vec3 texColor = texture(textureMap, uv).rgb;
		pColor = vec4(ad*texColor+vec3(spec), 1);
	}
)";

// display

void Display(GLFWwindow *w) {
	float elapsedTime = (float)(clock() - startTime) / CLOCKS_PER_SEC;
	float alpha = (float)(sin(2 * PI * elapsedTime / duration) + 1) / 2;
	// background, zbuffer, anti-alias lines
	glClearColor(.6f, .6f, .6f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glUseProgram(program);

	SetUniform3v(program, "ctrlPts", 16, (float*)ctrlPts);

	// set alpha for interpolation between shapes
	SetUniform(program, "alpha", alpha);
	// send matrices to vertex shader
	SetUniform(program, "modelview", camera.modelview);
	SetUniform(program, "persp", camera.persp);
	// send transformed light to pixel shader
	vec3 xLight = Vec3(camera.modelview*vec4(light, 1));
	SetUniform(program, "light", xLight);
	// set texture
	glActiveTexture(GL_TEXTURE0+textureUnit);       // active texture corresponds with textureUnit
	glBindTexture(GL_TEXTURE_2D, textureName);      // bind active texture to textureName
	SetUniform(program, "textureMap", textureUnit);
	// draw 4-sided, tessellated patch
	glPatchParameteri(GL_PATCH_VERTICES, 4);
	float res = 64, outerLevels[] = { res, res, res, res }, innerLevels[] = { res, res };
	glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, outerLevels);
	glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, innerLevels);
	glDrawArrays(GL_PATCHES, 0, 4);
	// draw arcball, light
	glDisable(GL_DEPTH_TEST);
	if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && picked == &camera)
		camera.arcball.Draw();
	UseDrawShader(camera.fullview);
	Star(light, 9, vec3(1, 0, 0), vec3(0, 0, 1));
	glFlush();
}

// mouse callbacks

void MouseButton(float x, float y, bool left, bool down) {
	picked = NULL;
	if (left && !down)
		camera.Up();
	if (left && down) {
		if (MouseOver(x, y, light, camera.fullview)) {
			mover.Down(&light, (int) x, (int) y, camera.modelview, camera.persp);
			picked = &mover;
		}
		if (!picked) {
			camera.Down(x, y, Shift());
			picked = &camera;
		}
	}
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown && picked == &mover)
		mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
	if (leftDown && picked == &camera)
		camera.Drag((int) x, (int) y);
}

void MouseWheel(float spin) {
	camera.Wheel(spin, Shift());
}

// application

void Resize(int width, int height) {
	camera.Resize(winWidth = width, winHeight = height);
	glViewport(0, 0, width, height);
}

int main(int ac, char **av) {
	// init app window, OpenGL, shader program, texture
	GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "Tessellate a Sphere");
	program = LinkProgramViaCode(&vShader, NULL, &teShader, NULL, &pShader);
	textureName = ReadTexture(textureFilename);
	// callbacks
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Display(w);
		glfwPollEvents();
		glfwSwapBuffers(w);
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}

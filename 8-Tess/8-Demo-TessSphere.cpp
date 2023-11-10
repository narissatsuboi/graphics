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
float duration = 1.0; 

// tessellation evaluation shader
const char* teShader = R"(
    #version 400 
    layout (quads, equal_spacing, ccw) in; 
    uniform mat4 modelview, persp; 
    uniform float innerRadius = 1, outerRadius = 1; 
	uniform float alpha;
    out vec3 point, normal;
    out vec2 uv;
    float PI = 3.141592; 
    vec3 RotateAboutY(vec2 p, float radians) { 
        return vec3(cos(radians)*p.x, p.y, sin(radians)*p.x);; 
    } 
    void Straight(float v, out vec2 p, out vec2 n) { 
        p = vec2(innerRadius, 2*v-1); 
        n = vec2(1, 0); 
    } 
    void Slant(float v, out vec2 p, out vec2 n) { 
        p = vec2((1-v)*innerRadius, 2*v-1); 
        n = normalize(vec2(2, -innerRadius)); 
    } 
    void SemiCircle(float v, out vec2 p, out vec2 n) { 
        float angle = PI*v-PI/2; 
        p = vec2(cos(angle), sin(angle)); 
        n = p;                       // for unit circle, normal = point 
        p *= innerRadius; 
    } 
    void Circle(float v, float t, out vec2 p, out vec2 n) { 
        float angle = 2*v*PI-PI, c = cos(angle), s = sin(angle); 
        p = innerRadius*vec2(c+1+1.5*t, s); 
        n = vec2(c, s); 
    }
    void main() { 
        
        uv = gl_TessCoord.st;        // unique TessCoord per invocation 
        vec2 xp1, xn1;    // cross-section is in XY plane 
        SemiCircle(uv.y, xp1, xn1);   // set cross-section point, normal 
        vec3 p1 = RotateAboutY(xp1, uv.x*2*PI); // rotate point longitudinally 
        vec3 n1 = RotateAboutY(xn1, uv.x*2*PI); // rotate normal longitudinally 

        vec2 xp2, xn2;    // cross-section is in XY plane 
        Slant(uv.y, xp2, xn2);   // set cross-section point, normal 
        vec3 p2 = RotateAboutY(xp2, uv.x*2*PI); // rotate point longitudinally 
        vec3 n2 = RotateAboutY(xn2, uv.x*2*PI); // rotate normal longitudinally 

		vec3 p3 = mix(p1, p2, alpha);
		vec3 n3 = normalize(mix(n1, n2, alpha));

        point = (modelview*vec4(p3, 1)).xyz; // transform point 
        normal = (modelview*vec4(n3, 0)).xyz; // transform normal 

        gl_Position = persp*vec4(point, 1); 
    } 
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

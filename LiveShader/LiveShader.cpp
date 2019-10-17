// LiveShader.cpp : Defines the entry point for the console application.
//
#include <iostream>

#include "Graphics.h"

using namespace std;


void resizeWrapper(GLFWwindow* window, int width, int height);
void mouseWrapper(GLFWwindow* window, double xpos, double ypos);
void scrollWrapper(GLFWwindow* window, double xpos, double ypos);
void keyWrapper(GLFWwindow* window, int key, int scancode, int action, int mods);
void characterWrapper(GLFWwindow* window, unsigned int keycode);

Graphics *graphics;

int main()
{

	graphics = new Graphics();

	glfwSetFramebufferSizeCallback(graphics->window, resizeWrapper);
	glfwSetCursorPosCallback(graphics->window, mouseWrapper);
	glfwSetScrollCallback(graphics->window, scrollWrapper);
	glfwSetKeyCallback(graphics->window, keyWrapper);
	glfwSetCharCallback(graphics->window, characterWrapper);

	glfwSetInputMode(graphics->window,GLFW_STICKY_KEYS, 1);
	
	graphics->mainLoop();

	graphics->terminate();


	system("pause");

    return 0;
}

void resizeWrapper(GLFWwindow* window, int width, int height) {
	graphics->framebuffer_resize_callback(window, width, height);
}

void mouseWrapper(GLFWwindow* window, double xpos, double ypos) {
	graphics->mouse_callback(window, xpos, ypos);
}
void scrollWrapper(GLFWwindow* window, double xoffset, double yoffset) {
	graphics->scroll_callback(window, xoffset, yoffset);
}

void keyWrapper(GLFWwindow* window, int key, int scancode, int action, int mods) {
	graphics->key_callback(window, key, scancode, action, mods);
}

void characterWrapper(GLFWwindow* window, unsigned int keycode) {
	graphics->character_callback(window, keycode);
}


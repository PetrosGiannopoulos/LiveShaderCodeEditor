// LiveShader.cpp : Defines the entry point for the console application.
//
#include <iostream>

#include "Graphics.h"

using namespace std;


void resizeWrapper(GLFWwindow* window, int width, int height);
void mouseWrapper(GLFWwindow* window, double xpos, double ypos);
void scrollWrapper(GLFWwindow* window, double xpos, double ypos);

Graphics *graphics;

int main()
{

	graphics = new Graphics();

	glfwSetFramebufferSizeCallback(graphics->window, resizeWrapper);
	glfwSetCursorPosCallback(graphics->window, mouseWrapper);
	glfwSetScrollCallback(graphics->window, scrollWrapper);

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


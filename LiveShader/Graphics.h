#pragma once

#include <glad\glad.h>
#include <GLFW\glfw3.h>

#include "Shader.h"
#include "CShader.h"

#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "Camera.h"
#include "Models.h"
#include "Text.h"

#include <omp.h>

class Graphics {

public:
	GLFWwindow *window;
	Camera camera;
	const GLFWvidmode* mode;
	unsigned int screenQuadVAO, screenQuadVBO;
	unsigned int codeScreenTexture, displayScreenTexture;

	int cubemapWidth, cubemapHeight;

	bool firstMouse = true;
	bool firstClick = true;

	float lastX = 1000 / 2.0f;
	float lastY = 800 / 2.0f;

	// timing
	float deltaTime = 0.0f;	// time between current frame and last frame
	float lastFrame = 0.0f;

	Shader screenShader;

	Text codeEditor;

	int oldLeftMouseButtonState;

	int oldLeftArrowKeyState;

	glm::vec2 caretPos;

	unsigned int computeShaderTexture;
	CShader computeShader;

public:


	~Graphics() {
		delete this;
	}

	Graphics() {

		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_SAMPLES, 4);

		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		mode = glfwGetVideoMode(monitor);
		window = glfwCreateWindow(mode->width, mode->height, "My Title", NULL, NULL);


		if (window == NULL)
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
		}
		glfwMakeContextCurrent(window);

		glfwSetWindowPos(window, 0, 0);

		//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "Failed to initialize GLAD" << std::endl;

		}

		glEnable(GL_DEPTH_TEST);
		glViewport(0, 0, mode->width, mode->height);

		screenShader = Shader("screenShader.vs", "screenShader.fs");
		screenShader.use();
		screenShader.setInt("codeScreenTexture", 0);
		screenShader.setInt("displayScreenTexture",1);

		codeScreenTexture = loadTexture("Textures/bg.png");
		displayScreenTexture = loadTexture("Textures/floor.jpg");

		codeEditor = Text(Shader("textShader.vs", "textShader.fs"));

		caretPos = codeEditor.caretPos;

		caretPos = codeEditor.initCaretPos(mode->height);


		computeShader = CShader("Presets/ComputeShaderPreset1.glslcs");
		computeShaderTexture = computeShader.createFrameBufferTexture();

		computeShader.use();
		computeShader.setInt("skybox", 0);
		computeShader.setInt("diffuseTexture", 1);
		computeShader.setInt("ssao", 2);
		computeShader.setInt("shadowMap", 3);

		codeEditor.readLines(computeShader.getLines());
	}


	void mainLoop() {

		while (!glfwWindowShouldClose(window))
		{

			float currentFrame = glfwGetTime();
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;
			//cout <<"deltaTime: " << deltaTime << endl;
			//Input
			processInput();

			//Render
			render();

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	}


	void render() {

		glViewport(0, 0, mode->width, mode->height);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		screenShader.use();

		screenShader.setFloat("width", mode->width);
		screenShader.setFloat("height", mode->height);

		//set caretPos
		codeEditor.caretPos = caretPos;

		screenShader.setVec2("caretPos", codeEditor.caretPos);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, codeScreenTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, displayScreenTexture);
		renderScreenQuad();
		glBindVertexArray(screenQuadVAO);
		glDisable(GL_DEPTH_TEST);
		
		//glDisable(GL_DEPTH_TEST);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//draw Code
		codeEditor.renderCode(mode->width,mode->height);

		glEnable(GL_DEPTH_TEST);
	}

	unsigned int squadVAO = 0;
	unsigned int squadVBO;
	void renderScreenQuad()
	{
		if (squadVAO == 0)
		{
			float squadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};
			// setup plane VAO
			glGenVertexArrays(1, &squadVAO);
			glGenBuffers(1, &squadVBO);
			glBindVertexArray(squadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, squadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(squadVertices), &squadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		}
		glBindVertexArray(squadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}

	void processInput()
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			camera.ProcessKeyboard(FORWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			camera.ProcessKeyboard(BACKWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			camera.ProcessKeyboard(LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			camera.ProcessKeyboard(RIGHT, deltaTime);


		int newLeftMouseButtonState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		if (newLeftMouseButtonState == GLFW_RELEASE && oldLeftMouseButtonState == GLFW_PRESS) {
			//cout << "mouse pressed" << endl;
			double x, y;

			glfwGetCursorPos(window, &x, &y);
			
			if (codeEditor.searchingLine == false) {
				
				caretPos = glm::vec2(x,y+25);
				//caretPos = glm::vec2(x*2-5,y);
				glm::vec2 cPos = caretPos;
				cPos.y = mode->height - cPos.y;
				//caretPos = cPos;
				caretPos = codeEditor.convertScreenToTextPoint(cPos, mode->width, mode->height);
				//cout << caretPos.x << endl;
				//caretPos.y = mode->height - caretPos.y;
				codeEditor.searchingLine = false;
				
			}
		}

		oldLeftMouseButtonState = newLeftMouseButtonState;
		
		/*int newLeftArrowKeyState = glfwGetKey(window, GLFW_KEY_LEFT);
		if (newLeftArrowKeyState == GLFW_PRESS && oldLeftArrowKeyState != GLFW_PRESS) {
			cout << "left" << endl;
		}

		oldLeftArrowKeyState = newLeftArrowKeyState;
		*/
	}


	void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void mouse_callback(GLFWwindow* window, double xpos, double ypos)
	{
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

		lastX = xpos;
		lastY = ypos;

		camera.ProcessMouseMovement(xoffset, yoffset);

		
	}

	// glfw: whenever the mouse scroll wheel scrolls, this callback is called
	// ----------------------------------------------------------------------
	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	{

		if (yoffset > 0)caretPos = codeEditor.updateCaretByScroll(true);
		else if (yoffset < 0)caretPos = codeEditor.updateCaretByScroll(false);
		camera.ProcessMouseScroll(yoffset);
	}

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		//action: GLFW_PRESS / GLFW_RELEASE / GLFW_REPEAT
		//string keyName = string(glfwGetKeyName(key, scancode));
		
		
		switch (action) {
			case GLFW_PRESS:
				//cout << key << endl;
				
				keyAction(key);
				break;
			case GLFW_RELEASE:
				break;
			case GLFW_REPEAT:
				keyAction(key);
				break;
			default:
				break;
		};
	}

	void keyAction(int key) {

		switch (key) {
			case 263:

				//left arrow button
				caretPos = codeEditor.moveCaretLeft(mode->width);
				break;
			case 262:
				//right arrow button
				caretPos = codeEditor.moveCaretRight(mode->width);
				break;
			case 264:
				//down arrow button
				caretPos = codeEditor.moveCaretDown(mode->height);
				break;
			case 265:
				//up arrow button
				caretPos = codeEditor.moveCaretUp(mode->height);
				break;
			case 259:
				//backspace button
				caretPos = codeEditor.deleteBackSpaceCharacter(mode->height);
				break;
			case 261:
				//delete button
				caretPos = codeEditor.deleteCharacter();
				break;
			case 257:
				//enter button
				caretPos = codeEditor.enterNewLine(mode->height);
				break;
			case 258:
				//tab button
				caretPos = codeEditor.addTabSpace();
				break;
			default:
				break;
		};
	}

	void character_callback(GLFWwindow* window, unsigned int keycode) {
		//cout << keycode << endl;

		char c = keycode;
		caretPos = codeEditor.insertCharacter(c);
	}

	void terminate() {
		glfwTerminate();
	}

	// utility function for loading a 2D texture from file
	// ---------------------------------------------------
	unsigned int loadTexture(char const * path)
	{

		stbi_set_flip_vertically_on_load(1);
		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;
		unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);

		if (data)
		{
			GLenum format;
			GLenum dataFormat;
			if (nrComponents == 1) {
				format = GL_RED;
				dataFormat = GL_RED;
			}
			else if (nrComponents == 3) {
				format = GL_SRGB;
				dataFormat = GL_RGB;
			}
			else if (nrComponents == 4) {
				format = GL_SRGB_ALPHA;
				dataFormat = GL_RGBA;
			}

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);
		}
		else
		{
			std::cout << "Texture failed to load at path: " << path << std::endl;
			stbi_image_free(data);
		}

		return textureID;
	}

	unsigned int loadCubemap(vector<std::string> faces)
	{
		unsigned int textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

		int width, height, nrComponents;
		for (unsigned int i = 0; i < faces.size(); i++)
		{
			unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
			if (data)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
				cubemapWidth = width;
				cubemapHeight = height;
				stbi_image_free(data);
			}
			else
			{
				std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
				stbi_image_free(data);
			}
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		return textureID;
	}
};
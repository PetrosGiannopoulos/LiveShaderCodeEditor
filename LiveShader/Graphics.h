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
	Shader blurShader;

	Text codeEditor;

	int oldLeftMouseButtonState;

	int oldLeftArrowKeyState;

	glm::vec2 caretPos;

	unsigned int computeShaderTexture;
	CShader computeShader;

	glm::vec3 lightPos;

	glm::mat4 projection;

	unsigned int cubemapTexture;
	unsigned int planeTexture;
	unsigned int depthTexture;
	unsigned int ssaoColorBufferBlur;
	unsigned int ssaoColorBuffer;
	unsigned int ssaoBlurFBO;

	bool displayMode = false;

	glm::vec2 minSelectedArea;
	glm::vec2 maxSelectedArea;
	glm::vec2 pressCoords;
	glm::vec2 prevCoords;

	float prevSelectY,prevSelectW;

	float currentY,currentW;
	float currentDiff;
	int prevYState;
	int firstYDrag;

	float totalDistanceX = 0;

	float currentCharWidth;

	bool isPressSelected = false;
	bool isDragSelected = false;
	bool isSelected = false;

	float time = 0;

	Models model;
	unsigned int ssbo_sphere;

	unsigned int uboSelectionBoxBlock;

	struct SelectionBox{
		glm::vec2 minPoint;
		glm::vec2 maxPoint;
	};

	vector<SelectionBox> selectionBoxData;

public:


	~Graphics() {
		delete this;
	}

	Graphics() {
		
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		//glfwWindowHint(GLFW_SAMPLES, 4);

		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		mode = glfwGetVideoMode(monitor);
		window = glfwCreateWindow(mode->width, mode->height, "My Title", NULL, NULL);

		//cout << "width: " << mode->width << " | height: " << mode->height << endl;
		
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

		//glEnable(GL_MULTISAMPLE);
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


		computeShader = CShader("Presets/ComputeShaderPreset.glslcs");
		computeShaderTexture = computeShader.createFrameBufferTexture();

		computeShader.use();
		computeShader.setInt("skybox", 0);
		computeShader.setInt("diffuseTexture", 1);
		computeShader.setInt("ssao", 2);
		computeShader.setInt("shadowMap", 3);

		codeEditor.readLines(computeShader.getLines());

		blurShader = Shader("screenBlur.vs","screenBlur.fs");
		blurShader.use();
		blurShader.setInt("ssaoInput",0);

		// SSAO color buffer blur
		glGenFramebuffers(1, &ssaoBlurFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
		glGenTextures(1, &ssaoColorBufferBlur);
		glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mode->width, mode->height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		camera = Camera(glm::vec3(10.0f, 8.0f, 20.0f));
		camera.Pitch -= 15;
		camera.updateCameraVectors();

		lightPos = glm::vec3(10.0f, 16.0f, -5.0f);

		vector<std::string> faces
		{
			"Textures/s_right.jpg",
			"Textures/s_left.jpg",
			"Textures/s_top.jpg",
			"Textures/s_bottom.jpg",
			"Textures/s_front.jpg",
			"Textures/s_back.jpg",

		};
		cubemapTexture = loadCubemap(faces);

		planeTexture = loadTexture("Textures/floor_.jpg");

		model = Models();
		//model.loadModel("icosahedron.obj");

		//model.rebuildStructure();
		//model.rescaleModel(3);

		//model.fillSSBO();
		
		//cout << model.ssbo_triangles.size() << endl;
		//initSSBOData();

		int numBoxes = (mode->height - 100) / (codeEditor.rows + codeEditor.fontSize*0.5);
		
		cout << numBoxes << endl;
		glGenBuffers(1, &uboSelectionBoxBlock);
		glBindBuffer(GL_UNIFORM_BUFFER, uboSelectionBoxBlock);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(SelectionBox)*(numBoxes), NULL, GL_STATIC_DRAW); // allocate 150 bytes of memory
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		unsigned int boxes_index = glGetUniformBlockIndex(screenShader.ID, "SelectionBoxes");
		glUniformBlockBinding(screenShader.ID, boxes_index, 2);

		for (int i = 0; i < numBoxes; i++) {
			SelectionBox selectionBox = SelectionBox();
			selectionBox.minPoint = glm::vec2(100, mode->height - (100- (codeEditor.rows + codeEditor.fontSize*0.5) + (codeEditor.rows + codeEditor.fontSize*0.5)*i));
			selectionBox.maxPoint = glm::vec2(100, mode->height - (100- (codeEditor.rows + codeEditor.fontSize*0.5) + (codeEditor.rows + codeEditor.fontSize*0.5) + (codeEditor.rows + codeEditor.fontSize*0.5)*i));
			selectionBoxData.push_back(selectionBox);
		}
	}

	void updateSelectionBoxes(const Shader &shader) {
		glBindBufferBase(GL_UNIFORM_BUFFER, 2, uboSelectionBoxBlock);

		glBindBuffer(GL_UNIFORM_BUFFER, uboSelectionBoxBlock);
		int numBoxes = (mode->height - 100) / (codeEditor.rows + codeEditor.fontSize*0.5);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(SelectionBox)*(numBoxes), selectionBoxData.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		shader.setFloat("numBox", numBoxes);

		
	}

	int getSelectionBoxID(float y) {

		float lineHeight = codeEditor.getLineHeight();
		return (mode->height - 100 + lineHeight-y) / lineHeight;
	}

	void initSSBOData() {

		//GL_SHADER_STORAGE_BUFFER
		//compute Shader Buffer Data
		glCreateBuffers(1, &ssbo_sphere);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_sphere);

		glNamedBufferData(ssbo_sphere, model.ssbo_triangles.size() * sizeof(Models::TriangleSSBO), model.ssbo_triangles.data(), GL_STATIC_DRAW);

		//glNamedBufferData(ssbo_sphere, mcCubes.triangulator.mesh.ssbo_triangles.size() * sizeof(Models::TriangleSSBO), mcCubes.triangulator.mesh.ssbo_triangles.data(), GL_STATIC_DRAW);
		//glNamedBufferData(ssbo_sphere, 1000 * sizeof(Models::TriangleSSBO), NULL, GL_DYNAMIC_DRAW);

		//glBufferData(GL_SHADER_STORAGE_BUFFER, collObjects[0].ssbo_triangles.size() * sizeof(Models::TriangleSSBO), collObjects[0].ssbo_triangles.data(), GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_sphere);//binding number = 1
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind




	}


	void mainLoop() {
		int frameCount = 0;
		while (!glfwWindowShouldClose(window))
		{

			float currentFrame = glfwGetTime();
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;
			//cout <<"deltaTime: " << deltaTime << endl;
			//frameCount++;

			//if (deltaTime >= 1.0) {
				//cout << "FPS: " << frameCount << endl;
				//frameCount = 0;
				//lastFrame = currentFrame;
			//}

			//Input
			processInput();

			//Render
			render();

			glfwSwapBuffers(window);
			glfwPollEvents();

			time+= 0.1;
		}
	}


	void render() {

		glViewport(0, 0, mode->width, mode->height);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//execute compute shader
		
		if (displayMode) {
			projection = glm::perspective(glm::radians(camera.Zoom), (float)(mode->width*0.5) / (float)mode->height, 0.1f, 1000.0f);
			computeShaderTrace(projection*camera.GetViewMatrix());

			//bilateral blur
			//glDisable(GL_DEPTH_TEST);
			glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
			glClear(GL_COLOR_BUFFER_BIT);
			blurShader.use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, computeShaderTexture);
			renderScreenQuad();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			//glEnable(GL_DEPTH_TEST);
		}

		screenShader.use();

		screenShader.setFloat("width", mode->width);
		screenShader.setFloat("height", mode->height);
		screenShader.setInt("startY", codeEditor.selectionScrollY);

		screenShader.setFloat("lineHeight", (codeEditor.rows + codeEditor.fontSize*0.5));

		//selection
		updateSelectionBoxes(screenShader);


		screenShader.setBool("isSelected",isSelected);
		
		screenShader.setVec2("minSelectedArea",minSelectedArea);
		screenShader.setVec2("maxSelectedArea",maxSelectedArea);

		//set caretPos
		codeEditor.caretPos = caretPos;

		screenShader.setVec2("caretPos", codeEditor.caretPos);
		glDisable(GL_DEPTH_TEST);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, codeScreenTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
		renderScreenQuad();
		//glBindVertexArray(screenQuadVAO);
		
		
		//glDisable(GL_DEPTH_TEST);
		//glDrawArrays(GL_TRIANGLES, 0, 6);

		//draw Code
		if(displayMode == false)codeEditor.renderCode(mode->width,mode->height);

		glEnable(GL_DEPTH_TEST);
	}

	int nextPowerOfTwo(int x) {
		x--;
		x |= x >> 1; // handle 2 bit numbers
		x |= x >> 2; // handle 4 bit numbers
		x |= x >> 4; // handle 8 bit numbers
		x |= x >> 8; // handle 16 bit numbers
		x |= x >> 16; // handle 32 bit numbers
		x++;
		return x;
	}

	void computeShaderTrace(glm::mat4 projectionview) {
		glm::mat4 invProjectionView = glm::inverse(projectionview);

		float lightLength = 25;
		glm::mat4 lightViewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(0, -1, 0), glm::vec3(0, 1, 0));
		//lightProjection = glm::ortho(lightPos.x - lightLength, lightPos.x + lightLength, lightPos.y - lightLength, lightPos.y + lightLength, lightPos.z + lightLength, lightPos.z - lightLength)*lightViewMatrix;

		computeShader.use();

		computeShader.setVec3("eye", camera.Position);
		computeShader.setVec3("ray00", camera.GetEyeRay(-1, -1, invProjectionView));
		computeShader.setVec3("ray01", camera.GetEyeRay(-1, 1, invProjectionView));
		computeShader.setVec3("ray10", camera.GetEyeRay(1, -1, invProjectionView));
		computeShader.setVec3("ray11", camera.GetEyeRay(1, 1, invProjectionView));

		lightPos.y = 2;// 10 * (sin(time)*0.5 + 0.5);
		//lightPos.z = -15;
		computeShader.setVec4("sphere", glm::vec4(5,20,20,1));
		computeShader.setVec4("lightPos", glm::vec4(lightPos,1));
		computeShader.setVec4("mirror", glm::vec4(17,20,20,10));
		computeShader.setVec4("customModel", glm::vec4(0,5,20,1));
		computeShader.setVec4("cube", glm::vec4(5,5,20,1));
		computeShader.setFloat("time", time);
		computeShader.setInt("sphereTriangleSize", model.ssbo_triangles.size());

		glBindImageTexture(0, computeShaderTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		/* Compute appropriate invocation dimension. */
		unsigned int worksizeX = nextPowerOfTwo(mode->width/2);
		unsigned int worksizeY = nextPowerOfTwo(mode->height);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, planeTexture);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		//glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);


		glDispatchCompute(worksizeX / 32, worksizeY / 32, 1);
		
		/* Reset image binding. */
		glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
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
		/*
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
		*/

		if (displayMode) {
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
				camera.ProcessKeyboard(FORWARD, deltaTime);
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
				camera.ProcessKeyboard(BACKWARD, deltaTime);
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				camera.ProcessKeyboard(LEFT, deltaTime);
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
				camera.ProcessKeyboard(RIGHT, deltaTime);
		}

		int newLeftMouseButtonState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		if (newLeftMouseButtonState == GLFW_RELEASE && oldLeftMouseButtonState == GLFW_PRESS) {
			//cout << "mouse pressed" << endl;

			double x, y;

			glfwGetCursorPos(window, &x, &y);
			isPressSelected = false;
			if (pressCoords.x == x && pressCoords.y == (y + 25)) {
				
				isDragSelected = false;
				isSelected = false;
				codeEditor.isSelectionOn = false;
			}
			
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

		if (newLeftMouseButtonState == GLFW_PRESS && oldLeftMouseButtonState != GLFW_PRESS) {
			double x, y;
			

			glfwGetCursorPos(window, &x, &y);
			isPressSelected = true;
			
			y = y + 25;
			
			pressCoords.x = x;
			pressCoords.y = y;

			glm::vec4 pPoint = codeEditor.getScreenToTextPoint(glm::vec2(x, mode->height - y), mode->width, mode->height);

			currentCharWidth = codeEditor.getCurrentCharWidth(glm::vec2(pPoint.z,pPoint.w));
			prevCoords = glm::vec2(x,y);
			totalDistanceX = 0;

			for (int i = 0; i < selectionBoxData.size(); i++) {
				selectionBoxData[i].minPoint.x = pPoint.x;
				selectionBoxData[i].maxPoint.x = pPoint.x;
			}

			codeEditor.selectionScrollY = 0;
			codeEditor.isSelectionOn = true;

			prevSelectY = pPoint.y;
			prevSelectW = pPoint.w;
			currentY = pPoint.y;
			currentW = pPoint.w;
			prevYState = -1;
			firstYDrag = -1;
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
			if (displayMode) camera.ProcessMouseMovement(xoffset, yoffset);

			if (isPressSelected) {
				isSelected = true;
				isDragSelected = true;

				double x, y;
				glfwGetCursorPos(window, &x, &y);
				y = y + 25;
				int xmoveState = -1, ymoveState = -1;
				if (x < prevCoords.x)xmoveState = 0;
				else xmoveState = 1;

				if (y < prevCoords.y)ymoveState = 0;
				else ymoveState = 1;
				
				if (x < pressCoords.x) {
					minSelectedArea.x = x;
					maxSelectedArea.x = pressCoords.x;
				}
				else {
					minSelectedArea.x = pressCoords.x;
					maxSelectedArea.x = x;
				}

				if (y < pressCoords.y) {
					minSelectedArea.y = mode->height-y;
					maxSelectedArea.y = mode->height - pressCoords.y;
				}
				else {
					minSelectedArea.y = mode->height - pressCoords.y;
					maxSelectedArea.y = mode->height - y;
				}

			

				glm::vec4 currentPoint = codeEditor.getScreenToTextPoint(glm::vec2(x,mode->height-y), mode->width, mode->height);
				glm::vec4 prevPoint = codeEditor.getScreenToTextPoint(glm::vec2(prevCoords.x,mode->height-prevCoords.y),mode->width,mode->height);
				glm::vec4 pressPoint = codeEditor.getScreenToTextPoint(glm::vec2(pressCoords.x, mode->height - pressCoords.y), mode->width, mode->height);
				
				selectionBoxData[getSelectionBoxID(currentPoint.y)].minPoint.x = pressPoint.x;
				if (xmoveState == 1) {
					
					float tempMaxPointX = selectionBoxData[getSelectionBoxID(mode->height - y)].maxPoint.x;
					if(tempMaxPointX<x)selectionBoxData[getSelectionBoxID(mode->height - y)].maxPoint.x += codeEditor.selectNextCharacters(glm::vec2(prevPoint.z, prevPoint.w), currentPoint.z);
					if (selectionBoxData[getSelectionBoxID(mode->height - y)].maxPoint.x > codeEditor.getLineWidth(prevPoint.w))selectionBoxData[getSelectionBoxID(mode->height - y)].maxPoint.x = tempMaxPointX;
				}
				else if(xmoveState == 0){

					float tempMaxPointX = selectionBoxData[getSelectionBoxID(mode->height - y)].maxPoint.x;
					if (tempMaxPointX>=x)selectionBoxData[getSelectionBoxID(mode->height - y)].maxPoint.x -= codeEditor.selectPreviousCharacters(glm::vec2(prevPoint.z, prevPoint.w), currentPoint.z);
					if (selectionBoxData[getSelectionBoxID(mode->height - y)].maxPoint.x < codeEditor.startX)selectionBoxData[getSelectionBoxID(mode->height - y)].maxPoint.x = codeEditor.startX;
				}

				//float currentW = pressPoint.w;
				if (firstYDrag == -1)firstYDrag = ymoveState;

				if (ymoveState == 1) {
					
					if (prevYState == 0) {
						prevSelectW -= currentDiff;
						prevSelectY += currentDiff*codeEditor.getLineHeight();
						currentW = prevSelectW;
						currentY = prevSelectY;
					}
					//codeEditor.getLineY()
					//if(pressPoint.w)
					int diff = abs((int)(currentPoint.w - currentW));
					if (diff > 0) {

						for (int i = 0; i < diff;i++) {
							selectionBoxData[getSelectionBoxID(currentY) + i].minPoint.x = codeEditor.startX;
							float v = (firstYDrag==1) ? codeEditor.getLineEndX(glm::vec2(0, currentW + i)) : 0;
							selectionBoxData[getSelectionBoxID(currentY) + i].maxPoint.x = codeEditor.startX+v;
						}
						
						currentDiff = diff;
						
					}
					
				}
				else if (ymoveState == 0) {

					if (prevYState == 1) {

						prevSelectW += currentDiff;
						prevSelectY -= currentDiff*codeEditor.getLineHeight();
						currentW = prevSelectW;
						currentY = prevSelectY;
					}
					int diff = abs((int)(currentPoint.w - currentW));
					if (diff > 0) {
						//cout << diff << endl;
						for (int i = 0; i < diff; i++) {
							selectionBoxData[getSelectionBoxID(currentY) - i].minPoint.x = codeEditor.startX;
							float v = (firstYDrag == 0) ? codeEditor.getLineEndX(glm::vec2(0, currentW - i)) : 0;
							selectionBoxData[getSelectionBoxID(currentY) - i].maxPoint.x = codeEditor.startX+v;
						}

						currentDiff = diff;
						
					}
				}
				//caretPos = maxSelectedArea;

				prevCoords = glm::vec2(x, y);
				prevYState = ymoveState;
			}
		
	}

	// glfw: whenever the mouse scroll wheel scrolls, this callback is called
	// ----------------------------------------------------------------------
	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	{

		if (yoffset > 0)caretPos = codeEditor.updateCaretByScroll(true);
		else if (yoffset < 0)caretPos = codeEditor.updateCaretByScroll(false);
		
		if(displayMode)camera.ProcessMouseScroll(yoffset);
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
			case 268:
				//home button
				caretPos = codeEditor.backToHome();
				break;
			case 256:
				//esc button
				if (displayMode) {
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
					displayMode = false;
				}
				else glfwSetWindowShouldClose(window, true);
				break;
			default:
				break;
		};
	}

	void character_callback(GLFWwindow* window, unsigned int keycode) {
		//cout << keycode << endl;

		if (keycode == 96 && displayMode == false) {
			return;
			codeEditor.updateFile("Presets/ComputeShaderPreset.glslcs");
			computeShader = CShader("Presets/ComputeShaderPreset.glslcs");
			computeShaderTexture = computeShader.createFrameBufferTexture();

			computeShader.use();
			computeShader.setInt("skybox", 0);
			computeShader.setInt("diffuseTexture", 1);
			computeShader.setInt("ssao", 2);
			computeShader.setInt("shadowMap", 3);

			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			displayMode = true;
		}
		else {
			if (displayMode == false) {
				char c = keycode;
				caretPos = codeEditor.insertCharacter(c);
			}
		}
	}

	void terminate() {
		glfwTerminate();
	}

	// utility function for loading a 2D texture from file
	// ---------------------------------------------------
	unsigned int loadTexture(char const * path)
	{

		//stbi_set_flip_vertically_on_load(1);
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
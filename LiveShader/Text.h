#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <freetype\ftglyph.h>
#include <iostream>
#include <fstream>

class Text {

public:

	vector<string> codeText;
	vector<string> copyText;
	vector<vector<bool>> selectedCharacters;

	Shader textShader, *screenShader;

	struct Character {
		GLuint     TextureID;  // ID handle of the glyph texture
		glm::ivec2 Size;       // Size of glyph
		glm::ivec2 Bearing;    // Offset from baseline to left/top of glyph
		GLuint     Advance;    // Offset to advance to next glyph
	};

	map<GLchar, Character> Characters;

	FT_Face face;
	FT_Library ft;

	unsigned int VAO, VBO;

	unsigned int rows, cols;
	int fontSize;

	glm::vec2 caretPos;
	glm::vec2 caretPosI;

	int startX, startY, scrollX;
	bool searchingLine = false;
	int selectionScrollY;
	bool isSelectionOn = false;

public:

	Text() {

	}

	Text(Shader textShader) {
		
		this->textShader = textShader;
		//textShader = Shader("textShader.vs", "textShader.fs");

		//load font
	
		if (FT_Init_FreeType(&ft))cout << "ERROR::FREETYPE: Could not init FreeType Library" << endl;
		if (FT_New_Face(ft, "Fonts/NotoMono-Regular.ttf", 0, &face))cout << "ERROR::FREETYPE: Failed to load font" << endl;

		fontSize = 14;// 20;
		FT_Set_Pixel_Sizes(face, 0, fontSize);

		loadCharacters();

		clearResources();

		codeText.push_back("if(i==b){");
		codeText.push_back("b=i;");
		codeText.push_back("}else if");

		caretPos = glm::vec2(100,100);

		startX = 100;
		startY = 0;
		scrollX = 0;
		
	}

	void readLines(vector<string> lines) {
		codeText.clear();
		codeText = lines;

		clearSelectedChars();
	}

	void clearSelectedChars() {
		selectedCharacters.clear();
		int N = codeText.size();
		for (int i = 0; i < N; i++) {
			vector<bool> selectedChars;
			int L = codeText[i].size();
			for (int l = 0; l < L; l++) { selectedChars.push_back(false); }
			selectedCharacters.push_back(selectedChars);
		}
	}

	glm::vec2 initCaretPos(int height) {

		caretPosI.y = codeText.size() - 1;
		caretPosI.x = codeText[caretPosI.y].length();

		caretPos.y = height - 100 - caretPosI.y*(rows + fontSize*0.5);
		

		float x, y;

		x = startX;
		y = caretPos.y;
		float scale = 1;
		string line = codeText[caretPosI.y];
		std::string::const_iterator c;
		for (c = line.begin(); c != line.end(); c++) {
			Character ch_ = Characters[*c];

			GLfloat xpos = x + ch_.Bearing.x * scale;
			GLfloat ypos = y - (ch_.Size.y - ch_.Bearing.y) * scale;

			GLfloat w = ch_.Size.x * scale;
			GLfloat h = ch_.Size.y * scale;

			x += (ch_.Advance >> 6)*scale;
		}
		caretPos.x = x;
		caretPos.y += (rows + fontSize*0.5)*0.3;

		caretPos.y += startY*(rows+fontSize*0.5);

		return caretPos;
	}

	void loadCharacters() {

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction

		for (GLubyte c = 0; c < 128; c++)
		{
			// Load character glyph 
			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			{
				std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
				continue;
			}
			// Generate texture
			GLuint texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer
			);
			// Set texture options
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// Now store character for later use
			Character character = {
				texture,
				glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
				glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
				face->glyph->advance.x
			};
			Characters.insert(std::pair<GLchar, Character>(c, character));
		}

		// Configure VAO/VBO for texture quads
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

	}

	void clearResources() {

		rows = face->glyph->bitmap.rows;
		cols = face->glyph->bitmap.width;

		FT_Done_Face(face);
		FT_Done_FreeType(ft);
	}

	void renderCode(float width, float height) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glm::mat4 projection = glm::ortho(0.0f, width, 0.0f, height);
		textShader.use();
		textShader.setMat4("projection", projection);

		//RenderText(textShader,"exampleText", 100,height-200, 1.0f, glm::vec3(1,0,0));

		int N = codeText.size();
		

		for (int i = 0; i < N;i++) {

			if ((height - 100 - (i + startY)*(rows + fontSize*0.5))<0 || (height - 100 - (i + startY)*(rows + fontSize*0.5))>height)continue;

			vector<int> keywordType = preprocessText(codeText[i]);
			RenderText(width,textShader, codeText[i], keywordType , startX-scrollX, height - 100-(i+startY)*(rows+fontSize*0.5), 1.0f, glm::vec3(0, 0, 0));
		

			//render line numbers
			int nD = numDigits(i+1);

			string spaces;
			for (int k = nD; k < 4; k++) {
				spaces += ' ';
			}

			RenderText(width,textShader, spaces+to_string(i+1), keywordType, 10, height - 100 - (i+startY)*(rows + fontSize*0.5), 1.0, glm::vec3(1, 1, 1), false);
		}


	}

	int numDigits(int number)
	{
		int digits = 0;
		while (number) {
			number /= 10;
			digits++;
		}
		return digits;
	}

	bool isAlphaBeta(char c) {

		string ab = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#";
		
		bool found = false;
		for (int i = 0; i < ab.length();i++)if (ab[i] == c)return true;
		return found;
	}

	vector<int> preprocessText(string text) {


		//TODO: add keywords
		string testSet[] = { "uniform","float","vec2","vec3","vec4","gl_FragCoord","gl_FragColor","return",
			"if","else","mix","step","smoothstep","dot","void","length","normalize","sampler2D", "samplerCube",
			"in","out","clamp","for","break","continue","int","sin","cos","tan","sinh","cosh","tanh",
			"asin","acos","atan","mod","min","max","sign","cross","fract","ceil","floor","abs","inout","bool",
			"mat2", "mat3", "mat4", "struct", "#define", "const"};

		vector<int> keywordType;

		int numElements = sizeof(testSet)/sizeof(*testSet);
		
		string app = "";

		//remove_if(text.begin(), text.end(), isspace);
		int counter = 0;
		int commentI = -1;
		for (int i = 0; i < text.length(); i++) keywordType.push_back(0);
		for (int i = 0; i < text.length();i++) {

			char ci = text[i];

			if (ci == '/' && text[i + 1] == '/') {
				commentI = i;
				break;
			}

			if (ci == ' ' || isAlphaBeta(ci)==false) {
				app.clear();
				counter = 0;
				continue;
			}
			app += ci;
			counter++;

			for (int j = 0; j < numElements;j++) {

				string test = testSet[j];

				if (app.compare(test)==0) {
					if (isAlphaBeta(text[i+1])==false) {
						if (isAlphaBeta(text[i - counter]) == false) {
							for (int k = 0; k < counter; k++) keywordType[i - k] = 1;
							app.clear();
							counter = 0;
							break;
						}
					}
				}
				else {

					int diff = test.length() - counter;
					string temp = app;
					for (int k = 0; k < diff; k++) {
						temp += text[i+k + counter];
					}

					if (temp.compare(test) == 0) {
						if (isAlphaBeta(text[i-counter]) == false) {
							if (isAlphaBeta(text[i+diff+1]) == false) {
								for (int k = 0; k < counter; k++)keywordType[i - k] = 1;
								for (int k = 0; k < diff; k++)keywordType[i + k + 1] = 1;

								i = i + diff;
								app.clear();
								counter = 0;
								break;
							}
						}
						
					}
					
					if (j < numElements - 1) continue;
					

					if (app.length() == test.length()) {

						i = i - (counter - 1);

						counter = 0;
						app.clear();
						
						
					}	

				}
			}
		}
		
		if (commentI != -1) {
			for (int i = commentI; i < text.length();i++) {
				keywordType[i] = 2;
			}
		}

		return keywordType;

	}

	void RenderText(int width, Shader &shader, string text, vector<int> keywordType, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color, bool keywords=true)
	{
		// Activate corresponding render state	
		shader.use();
		shader.setVec3("textColor", color);
		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(VAO);

		// Iterate through all characters
		std::string::const_iterator c;
		int counter = 0;
		for (c = text.begin(); c != text.end(); c++)
		{
			if (keywords) {
				if (keywordType[counter] == 0)shader.setVec3("textColor", color);
				else if (keywordType[counter] == 1)shader.setVec3("textColor", glm::vec3(51 / 255., 131 / 255., 247 / 255.));
				else if(keywordType[counter] == 2) shader.setVec3("textColor", glm::vec3(45 / 255., 155 / 255., 45 / 255.));
			}
			else shader.setVec3("textColor", color);
			
			counter++;
			Character ch = Characters[*c];

			GLfloat xpos = x + ch.Bearing.x * scale;
			GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

			if (x > width*0.5)return;
			if (keywords == true && x < startX) {
				x += (ch.Advance >> 6) * scale;
				continue;
			}
			shader.setVec2("pos", glm::vec2(xpos,ypos));

			GLfloat w = ch.Size.x * scale;
			GLfloat h = ch.Size.y * scale;
			// Update VBO for each character
			GLfloat vertices[6][4] = {
				{ xpos,     ypos + h,   0.0, 0.0 },
				{ xpos,     ypos,       0.0, 1.0 },
				{ xpos + w, ypos,       1.0, 1.0 },

				{ xpos,     ypos + h,   0.0, 0.0 },
				{ xpos + w, ypos,       1.0, 1.0 },
				{ xpos + w, ypos + h,   1.0, 0.0 }
			};
			// Render glyph texture over quad
			glBindTexture(GL_TEXTURE_2D, ch.TextureID);
			// Update content of VBO memory
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			// Render quad
			glDrawArrays(GL_TRIANGLES, 0, 6);
			// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
			x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
		}
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);

	}

	float getLineHeight() {
		return (rows + fontSize*0.5);
	}

	float getLineWidth(int line) {

		string currentLine = codeText[line];
		float scale = 1.0;
		float xSum = startX;
		int N = currentLine.length();
		for (int i = 0; i < N;i++) {
			Character ch = Characters[currentLine[i]];

			xSum += ((ch.Advance >> 6)*scale);
		}

		return xSum;
	}

	float getLineY(float height, float i) {
		
		return height - 100 - (i+startY)*(rows + fontSize*0.5);
	}

	glm::vec2 getLineEndX(glm::vec2 pos) {

		string currentLine = codeText[pos.y];

		float xSum = 0;
		float scale = 1.0;
		int counter = 0;
		int N = currentLine.length();
		for (int i = pos.x; i < N;i++) {
			Character ch = Characters[currentLine[i]];

			xSum += ((ch.Advance >> 6)*scale);
			counter++;
		}

		return glm::vec2(xSum,counter);
	}

	glm::vec2 getLineTillX(glm::vec2 pos) {
		
		string currentLine = codeText[pos.y];

		float xSum = 0;
		float scale = 1.0;
		int counter = 0;
		for (int i = 0; i < pos.x;i++) {
			Character ch = Characters[currentLine[i]];

			xSum += ((ch.Advance >> 6)*scale);
			counter++;
		}

		return glm::vec2(xSum,counter);
	}

	void selectLine(glm::vec2 from, int selectionSize) {

		int counter = 0;
		
		string currentLine = codeText[from.y];
		for (int i = from.x; i < currentLine.length() ;i++) {
			selectedCharacters[from.y][i] = true;
			counter++;
			if (counter >= selectionSize)break;
		}

		/*
		
		for (int i = 0; i < currentLine.length(); i++) {
			cout << currentLine[i];
		}
		cout << " selectionSize: " << selectionSize << " | pointx: " << from.x<< endl;
		*/
	}

	void selectSubLine(glm::vec2 from, float posX) {

		int signDiff = from.x - posX;

		if (signDiff < 0) {
			for (int i = from.x; i < posX;i++) {
				selectedCharacters[from.y][i] = true;
			}
		}
		else {
			for (int i = posX; i < from.x;i++) {
				selectedCharacters[from.y][i] = true;
			}
		}

	}

	void clearSelectionData() {
		int n = selectedCharacters.size();
		for (int i = 0; i < n;i++) {
			int nj = selectedCharacters[i].size();
			for (int j = 0; j < nj;j++) {
				selectedCharacters[i][j] = false;
			}
		}
	}

	void getSystemClipboardText(GLFWwindow *window) {
		string s = glfwGetClipboardString(window);

		//cout << s << endl;
		int counter = 0;
		copyText.clear();
		string line = "";
		for (int i = 0; i < s.length(); i++) {

			if (i == 0) {
				copyText.push_back("");
			}

			if (s[i] != '\n' && s[i] != '\t' && !isspace(s[i]))copyText[counter] += s[i];

			if (isspace(s[i]))copyText[counter] += ' ';

			if (s[i] == '\t') {
				for (int j = 0; j < 4;j++)copyText[counter] += ' ';
			}

			if (s[i] == '\n') {
				copyText.push_back("");
				counter++;
			}

		}

	}

	void pasteSelectionFromClipboard(GLFWwindow *window) {
		getSystemClipboardText(window);
		pasteSelection();
	}

	void pasteSelection() {

		int x = caretPosI.x;
		int y = caretPosI.y;

		int lineDiff = copyText.size();

		if (lineDiff == 0)return;

		//copy text before caretPosI.x

		string firstLine = codeText[y];
		string startLine;
		for (int i = 0; i < firstLine.length();i++) {
			if (i >= x)break;
			startLine += firstLine[i];
		}

		string cpyStr = codeText[caretPosI.y].substr(caretPosI.x, codeText[caretPosI.y].length() - caretPosI.x);

		codeText[caretPosI.y].erase(codeText[caretPosI.y].begin() + caretPosI.x, codeText[caretPosI.y].end());

		//if (codeText[caretPosI.y].length() == 0)codeText[caretPosI.y].push_back(' ');

		//codeText.insert(codeText.begin() + caretPosI.y + 1, cpyStr);

		//TODO: push lines below 1 line down

		for (int i = 0; i < lineDiff;i++) {
			string currentString = copyText[i];
			int N = currentString.length();
			if(i==0)for (int j = 0; j < N;j++) insertCharacter(currentString[j]);
			else {
				if (i == lineDiff - 1)currentString += cpyStr;
				codeText.insert(codeText.begin() + caretPosI.y + i, currentString);
			}	
		}
	}

	void cutSelection(glm::vec2 from, glm::vec2 to, glm::vec2 fromXY, glm::vec2 toXY) {
		copySelection(from,to);
		eraseSelection(from, to, fromXY, toXY);
	}

	void copySelection(glm::vec2 from, glm::vec2 to) {

		int lineDiff = abs(from.y-to.y);
		int lengthDiff = abs(from.x - to.x);

		if (lineDiff == 0 && lengthDiff == 0)return;

		copyText.clear();

		for (int i = 0; i <= lineDiff; i++) {

			string currentLine = codeText[from.y + i];
			int selectionStart = 0;
			int selectionCounter = 0;
			for (int j = 0; j < currentLine.length(); j++) {

				if (j > 0) {
					if (selectedCharacters[from.y + i][j] == true && selectedCharacters[from.y + i][j - 1] == false) {
						selectionStart = j;
					}
				}
				else {
					if (selectedCharacters[from.y + i][j] == true) {
						selectionStart = j;
					}
				}

				if (selectedCharacters[from.y + i][j] == true) {
					selectionCounter++;
					//cout << currentLine[j];
				}
				else if (j > 0) if (selectedCharacters[from.y + i][j - 1] == true)break;

			}
			copyText.push_back(codeText[from.y + i].substr(selectionStart, selectionCounter));
			//cout << codeText[from.y + i].substr(selectionStart, selectionCounter)  << endl;
		}
	}

	void eraseSelection(glm::vec2 from, glm::vec2 to, glm::vec2 fromXY, glm::vec2 toXY) {

		int lineDiff = abs(from.y-to.y);
		int lengthDiff = abs(from.x-to.x);

		if (lineDiff == 0 && lengthDiff == 0)return;

		for (int i = 0; i <= lineDiff;i++) {
			
			string currentLine = codeText[from.y + i];
			int selectionStart = 0;
			int selectionCounter = 0;
			for (int j = 0; j < currentLine.length();j++) {

				if (j > 0) {
					if (selectedCharacters[from.y + i][j] == true && selectedCharacters[from.y + i][j - 1] == false) {
						selectionStart = j;
					}
				}
				else {
					if (selectedCharacters[from.y + i][j] == true) {
						selectionStart = j;
					}
				}

				if (selectedCharacters[from.y+i][j]==true) {
					selectionCounter++;
					//cout << currentLine[j];
				}
				else if (j > 0) if (selectedCharacters[from.y + i][j - 1] == true)break;

			}
			//cout << " | " << selectionCounter<< endl;
			
			//cout << codeText[from.y + i].substr(selectionStart, selectionCounter) << endl;

			codeText[from.y + i].erase(selectionStart, selectionCounter);

			string copyStr = codeText[from.y + i];
			if (selectionStart == 0) {
				codeText[from.y + i].erase(0, copyStr.length());
				//append to end of first line
				codeText[from.y].insert(codeText[from.y].length(), copyStr);
			}
		}

		for (int i = lineDiff; i >= 0; i--) {
			if (codeText[from.y + i].length() == 0) {
				//replace it with lines below
				int N = codeText.size();
				for (int j = (from.y + i); j < N - 1; j++) codeText[j] = codeText[j + 1];
				codeText[N - 1].erase(codeText[N - 1].begin(), codeText[N - 1].end());
				codeText.resize(codeText.size() - 1);
			}
		}

		
		

		if (from.y <= to.y) {
			caretPosI.y = from.y;
			caretPos.y = fromXY.y;
			caretPosI.x = from.x;
			caretPos.x = fromXY.x;
		}
		else {
			caretPosI.y = to.y;
			caretPos.y = toXY.y;

			if (from.x <= to.x) {
				caretPosI.x = from.x;
				caretPos.x = fromXY.x;
			}
			else {
				caretPosI.x = to.x;
				caretPos.x = toXY.x;
			}
		}
		
	}

	glm::vec2 updateCaretByScroll(bool upwards) {

		int scrollSpeed = 5;
		if (upwards > 0) {

			if (startY == 0)return caretPos;

			caretPos.y -= (rows + fontSize*0.5)*scrollSpeed;
			startY+= scrollSpeed;
			if (isSelectionOn)selectionScrollY += scrollSpeed;
		}
		else {

			caretPos.y += (rows + fontSize*0.5)*scrollSpeed;
			startY-= scrollSpeed;
			if (isSelectionOn)selectionScrollY -= scrollSpeed;
		}

		

		return caretPos;
	
	}

	glm::vec2 convertScreenToTextPoint(glm::vec2 pos, int width, int height) {

		float minDistX = FLT_MAX;
		float minDistY = FLT_MAX;
		float minDist = FLT_MAX;
		float minX = FLT_MAX;
		float minY = FLT_MAX;
		float scale = 1;

		float minIX = -1;
		float minIY = -1;

		float x, y;

		x = startX;
		
		searchingLine = true;

		/* Logic: First we find the discrete row by looking at
		the smallest difference between current y caret position coord 
		and the minimum. Then for the selected row we find the smallest 
		difference between current x caret position coord and the minimum
		*/


		for (int i = 0; i < codeText.size();i++) {

			string text = codeText[i];

			y = height-100 - (i+startY)*(rows + fontSize*0.5);

			// Iterate through all characters
			std::string::const_iterator c;
			int counter = 0;
			for (c = text.begin(); c != text.end(); c++){

				
				Character ch = Characters[*c];

				GLfloat xpos = x + ch.Bearing.x * scale;
				GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

				GLfloat w = ch.Size.x * scale;
				GLfloat h = ch.Size.y * scale;

				float diffY = abs(y + (rows + fontSize*0.5)*0.3 - pos.y);

				if (diffY < minDistY) {
					minDistY = diffY;
					minY = y +(rows + fontSize*0.5)*0.3;
					minIY = i;
				}

				x += (ch.Advance >> 6) * scale;
				counter++;
			}

		}

		x = startX;
		string text = codeText[minIY];
		std::string::const_iterator c;
		int counter = 0;
		for (c = text.begin(); c != text.end(); c++) {
			Character ch = Characters[*c];

			GLfloat xpos = x + ch.Bearing.x * scale;
			GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

			GLfloat w = ch.Size.x * scale;
			GLfloat h = ch.Size.y * scale;

			float diffX = abs(x - pos.x);

			if (diffX < minDistX) {
				minDistX = diffX;
				minX = x;// xpos;
				minIX = counter;
			}

			x += (ch.Advance >> 6) * scale;
			counter++;

		}

		Character lch = Characters[text[text.length() - 1]];

		if (minIX == (text.length() - 1) && pos.x >= (minX + ((lch.Advance >> 6)*scale))) {
			minX += ((lch.Advance >> 6)*scale);
			minIX++;
		}


		caretPosI = glm::vec2(minIX,minIY);

		return glm::vec2(minX,minY);


	}

	glm::vec4 getScreenToTextPoint(glm::vec2 pos, int width, int height) {

		float minDistX = FLT_MAX;
		float minDistY = FLT_MAX;
		float minDist = FLT_MAX;
		float minX = FLT_MAX;
		float minY = FLT_MAX;
		float scale = 1;

		float minIX = -1;
		float minIY = -1;

		float x, y;

		x = startX;

		for (int i = 0; i < codeText.size(); i++) {

			string text = codeText[i];

			y = height - 100 - (i + startY)*(rows + fontSize*0.5);

			// Iterate through all characters
			std::string::const_iterator c;
			int counter = 0;
			for (c = text.begin(); c != text.end(); c++) {


				Character ch = Characters[*c];

				GLfloat xpos = x + ch.Bearing.x * scale;
				GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

				GLfloat w = ch.Size.x * scale;
				GLfloat h = ch.Size.y * scale;

				float diffY = abs(y + (rows + fontSize*0.5)*0.3 - pos.y);

				if (diffY < minDistY) {
					minDistY = diffY;
					minY = y + (rows + fontSize*0.5)*0.3;
					minIY = i;
				}

				x += (ch.Advance >> 6) * scale;
				counter++;
			}

		}

		x = startX;
		string text = codeText[minIY];
		std::string::const_iterator c;
		int counter = 0;
		for (c = text.begin(); c != text.end(); c++) {
			Character ch = Characters[*c];

			GLfloat xpos = x + ch.Bearing.x * scale;
			GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

			GLfloat w = ch.Size.x * scale;
			GLfloat h = ch.Size.y * scale;

			float diffX = abs(x - pos.x);

			if (diffX < minDistX) {
				minDistX = diffX;
				minX = x;// xpos;
				minIX = counter;
			}

			x += (ch.Advance >> 6) * scale;
			counter++;

		}

		Character lch = Characters[text[text.length() - 1]];

		if (minIX == (text.length() - 1) && pos.x >= (minX + ((lch.Advance >> 6)*scale))) {
			minX += ((lch.Advance >> 6)*scale);
			minIX++;
		}

		//caretPosI = glm::vec2(minIX, minIY);

		return glm::vec4(minX,minY, minIX,minIY);


	}

	float getCharWidth(glm::vec2 pos, int moveState) {
		
		float scale = 1.0;
		if (moveState == 0) {
			//left
			if (pos.x <= 0)return 0;
			string currentLine = codeText[pos.y];
			Character ch = Characters[currentLine[pos.x]];
			return ((ch.Advance >> 6) * scale);

		}
		else if (moveState == 1) {
			//right
			
			string currentLine = codeText[pos.y];
			if (pos.x >= currentLine.length())return 0;
			if (pos.x == currentLine.length() - 1)pos.x--;
			Character ch = Characters[currentLine[pos.x+1]];
			return (ch.Advance >> 6) * scale;
			//cout << currentLine << endl;
		}

		return 0;
	}

	float getCharX(glm::vec2 pos, int moveState) {
		float scale = 1.0;

		if (moveState == 0) {
			//left
			if (pos.x <= 0)return 10000;
			string currentLine = codeText[pos.y];
			float xSum = startX;
			for (int i = 0; i < pos.x - 1; i++) {
				Character ch = Characters[currentLine[i]];
				xSum += ((ch.Advance >> 6)*scale);
			}
			return xSum;
		}
		else if (moveState == 1) {
			//right
			
			string currentLine = codeText[pos.y];
			if (pos.x > currentLine.length()-1)return 10000;
			float xSum = startX;
			if (pos.x == currentLine.length() - 1) {
				for (int i = 0; i < pos.x; i++) {
					Character ch = Characters[currentLine[i]];
					xSum += ((ch.Advance >> 6)*scale);
				}
				
				return xSum;
			}
			
			for (int i = 0; i < pos.x+1; i++) {
				Character ch = Characters[currentLine[i]];
				xSum += ((ch.Advance >> 6)*scale);
			}
			return xSum;
		}

		return 10000;
	}

	float selectNextCharacters(glm::vec2 pos, float ix) {

		string currentLine = codeText[pos.y];

		float xSum = 0;
		float scale = 1.0;
		for (int i = pos.x; i < ix;i++) {
			Character ch = Characters[currentLine[i]];

			xSum += ((ch.Advance >> 6)*scale);
		}

		

		return xSum;
	}

	float selectPreviousCharacters(glm::vec2 pos, float ix) {

		string currentLine = codeText[pos.y];

		float xSum = 0;
		float scale = 1.0;
		for (int i = pos.x; i > ix; i--) {
			Character ch = Characters[currentLine[i]];

			xSum += ((ch.Advance >> 6)*scale);
		}

		return xSum;
	}

	float getCurrentCharWidth(glm::vec2 pos) {

		float scale = 1.0;
		string currentLine = codeText[pos.y];
		Character ch = Characters[currentLine[pos.x]];
		return (ch.Advance >> 6) * scale;
	}

	glm::vec2 moveCaretLeft(int width) {

		if (caretPosI.x <= 0)return caretPos;

		string lineText = codeText[caretPosI.y];

		char previousC = lineText[caretPosI.x - 1];

		Character pc = Characters[previousC];

		caretPosI.x--;

		float scale = 1;

		if(caretPos.x <= startX){
			scrollX -= (pc.Advance >> 6)*scale;
		}
		else caretPos.x = caretPos.x-(pc.Advance>>6)*scale;

		return caretPos;

	}

	glm::vec2 moveCaretRight(int width) {

		

		string lineText = codeText[caretPosI.y];

		if (caretPosI.x >= lineText.length())return caretPos;
		char c = lineText[caretPosI.x];

		Character nc = Characters[c];
		caretPosI.x++;

		float scale = 1;

		if (caretPos.x > width*0.5) {
			scrollX += (nc.Advance >> 6)*scale;
		}
		else caretPos.x = caretPos.x + (nc.Advance >> 6)*scale;

		return caretPos;
	}

	glm::vec2 moveCaretUp(int height) {

		if (caretPosI.y <= 0)return caretPos;

		string plineText = codeText[caretPosI.y - 1];
		
		caretPosI.y--;

		//find closest x
		float minDistX = FLT_MAX;
		float minX = FLT_MAX;
		float minIX = -1;

		float scale = 1;
	
		int counter = 0;
		caretPos.y = height - 100 - (caretPosI.y+startY)*(rows + fontSize*0.5);
		float x = startX;
		float y = caretPos.y;
		std::string::const_iterator c;
		for (c = plineText.begin(); c != plineText.end(); c++) {
			Character ch = Characters[*c];

			GLfloat xpos = x + ch.Bearing.x * scale;
			GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

			GLfloat w = ch.Size.x * scale;
			GLfloat h = ch.Size.y * scale;

			float diffX = abs(xpos - caretPos.x);

			if (diffX < minDistX) {
				minDistX = diffX;
				minX = x;// xpos;
				minIX = counter;
			}
			counter++;

			x += (ch.Advance >> 6) * scale;
		}

		caretPosI.x = minIX;
		caretPos.x = minX;
		caretPos.y += (rows + fontSize*0.5)*0.3;

		return caretPos;
	}

	glm::vec2 moveCaretDown(int height) {
		if (caretPosI.y >= codeText.size()-1)return caretPos;

		string nlineText = codeText[caretPosI.y + 1];

		caretPosI.y++;

		//find closest x
		float minDistX = FLT_MAX;
		float minX = FLT_MAX;
		float minIX = -1;

		float scale = 1;

		int counter = 0;
		caretPos.y = height - 100 - (caretPosI.y+startY)*(rows + fontSize*0.5);
		float x = startX;
		float y = caretPos.y;
		std::string::const_iterator c;
		for (c = nlineText.begin(); c != nlineText.end(); c++) {
			Character ch = Characters[*c];

			GLfloat xpos = x + ch.Bearing.x * scale;
			GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

			GLfloat w = ch.Size.x * scale;
			GLfloat h = ch.Size.y * scale;

			float diffX = abs(xpos - caretPos.x);

			if (diffX < minDistX) {
				minDistX = diffX;
				minX = x;// xpos;
				minIX = counter;
			}
			counter++;

			x += (ch.Advance >> 6) * scale;
		}

		caretPosI.x = minIX;
		caretPos.x = minX;
		caretPos.y += (rows + fontSize*0.5)*0.3;

		return caretPos;
	}

	glm::vec2 insertCharacter(char c) {

		codeText[caretPosI.y].insert(caretPosI.x,1,c);
		Character ch = Characters[c];
		float scale = 1;
		caretPos.x += (ch.Advance >> 6)*scale;
		caretPosI.x++;

		return caretPos;
	}

	glm::vec2 deleteCharacter() {

		codeText[caretPosI.y].erase(caretPosI.x,1);

		return caretPos;

	}

	glm::vec2 deleteBackSpaceCharacter(int height) {

		float scale = 1;
		//TODO: if on start of the string paste line to the end of the previous one
		// and move all line below it to that height
		if (caretPosI.x <= 0) {

			if (caretPosI.y > 0) {

				string cpyStr = codeText[caretPosI.y];

				
				//codeText[caretPosI.y].erase(codeText[caretPosI.y].begin(), codeText[caretPosI.y].end());

				//move next lines 1 row up
				int N = codeText.size();
				for (int i = caretPosI.y; i < N - 1; i++) codeText[i] = codeText[i + 1];
				codeText[N - 1].erase(codeText[N - 1].begin(), codeText[N - 1].end());
				codeText.resize(codeText.size() - 1);

				caretPosI.y--;

				caretPos.y = height - 100 - (caretPosI.y+startY)*(rows + fontSize*0.5);
				
				string currentLine = codeText[caretPosI.y];

				float x, y;
				x = startX;
				y = caretPos.y;

				int counter = 0;
				std::string::const_iterator c;
				for (c = currentLine.begin(); c != currentLine.end(); c++) {
					Character ch_ = Characters[*c];

					GLfloat xpos = x + ch_.Bearing.x * scale;
					GLfloat ypos = y - (ch_.Size.y - ch_.Bearing.y) * scale;

					GLfloat w = ch_.Size.x * scale;
					GLfloat h = ch_.Size.y * scale;

					x += (ch_.Advance >> 6)*scale;
					counter++;
				}


				caretPos.x = x;
				caretPos.y += (rows + fontSize*0.5)*0.3;
				caretPosI.x = counter;

				codeText[caretPosI.y].insert(codeText[caretPosI.y].length(), cpyStr);

			}
			//cout << codeText[caretPosI.y-1] << endl;

			return caretPos;
		}

		char pc = codeText[caretPosI.y][caretPosI.x - 1];
		codeText[caretPosI.y].erase(caretPosI.x-1,1);

		Character ch = Characters[pc];
		
		caretPos.x -= (ch.Advance >> 6)*scale;
		caretPosI.x--;


		return caretPos;
	}

	glm::vec2 enterNewLine(int height) {
		
		//TODO: clear string from caretPos plus
		// move all string below that line 1 line down
		// paste cleared part of string to the empty line

		
		string cpyStr = codeText[caretPosI.y].substr(caretPosI.x, codeText[caretPosI.y].length()-caretPosI.x);

		codeText[caretPosI.y].erase(codeText[caretPosI.y].begin()+caretPosI.x, codeText[caretPosI.y].end());

		//if (codeText[caretPosI.y].length() == 0)codeText[caretPosI.y].push_back(' ');

		codeText.insert(codeText.begin()+caretPosI.y+1,cpyStr);

		caretPosI.y++;
		caretPosI.x = 0;

		caretPos.x = startX;

		if (caretPosI.y < codeText.size() - 1) {
			float scale = 1;
			std::string::const_iterator c;
			string nline = codeText[caretPosI.y + 1];
			for (c = nline.begin(); c != nline.end(); c++) {
				Character ch_ = Characters[*c];
				if (*c == ' ') {
					caretPos.x += (ch_.Advance >> 6)*scale;
					codeText[caretPosI.y].insert(codeText[caretPosI.y].begin(), 1, ' ');
					caretPosI.x++;
				}
				else break;
			}
		}

		if (codeText[caretPosI.y-1].length() == 0)codeText[caretPosI.y-1].push_back(' ');

		caretPos.y = height - 100 - (caretPosI.y+startY)*(rows + fontSize*0.5);
		caretPos.y += (rows + fontSize*0.5)*0.3;

		scrollX = 0;

		return caretPos;

	}

	glm::vec2 addTabSpace() {

		codeText[caretPosI.y].insert(caretPosI.x,"    ");
		
		caretPosI.x += 4;
		
		float scale = 1;

		char c = ' ';
		Character ch = Characters[c];

		caretPos.x += 4 * (ch.Advance >> 6)*scale;

		return caretPos;

	}

	glm::vec2 backToHome() {

		scrollX = 0;

		caretPos.x = startX;
		caretPosI.x = 0;


		return caretPos;
	}

	void updateFile(string path) {
		ofstream myfile(path);
		
		if (myfile.is_open()) {
			myfile.clear();
			int N = codeText.size();
			for (int i = 0; i < N;i++) {
				myfile << codeText[i] << endl;
			}

		}
		else {
			cout << "error opening file. Check if path is correct." << endl;
		}
		myfile.close();
	}
};
#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <freetype\ftglyph.h>

class Text {

public:

	vector<string> codeText;

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

public:

	Text() {

	}

	Text(Shader textShader) {
		
		this->textShader = textShader;
		//textShader = Shader("textShader.vs", "textShader.fs");

		//load font
	
		if (FT_Init_FreeType(&ft))cout << "ERROR::FREETYPE: Could not init FreeType Library" << endl;
		if (FT_New_Face(ft, "Fonts/NotoMono-Regular.ttf", 0, &face))cout << "ERROR::FREETYPE: Failed to load font" << endl;

		fontSize = 20;
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

			RenderText(width,textShader, spaces+to_string(i+1), keywordType, 5, height - 100 - (i+startY)*(rows + fontSize*0.5), 1.0, glm::vec3(1, 1, 1), false);
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

		string ab = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
		
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
			"mat2", "mat3", "mat4", "struct", "#define"};

		vector<int> keywordType;

		int numElements = sizeof(testSet)/sizeof(*testSet);
		
		string app = "";

		//remove_if(text.begin(), text.end(), isspace);
		int counter = 0;
		for (int i = 0; i < text.length(); i++) keywordType.push_back(0);
		for (int i = 0; i < text.length();i++) {

			char ci = text[i];

			if (ci == ' ')continue;
			app += ci;
			counter++;

			for (int j = 0; j < numElements;j++) {

				string test = testSet[j];

				

				if (app.compare(test)==0) {
					if (isAlphaBeta(text[i+1])==false && isAlphaBeta(text[i-counter]==false)) {
						for (int k = 0; k < counter; k++) keywordType[i - k] = 1;
						app.clear();
						counter = 0;
						break;
					}
				}
				else {

					int diff = test.length() - counter;
					string temp = app;
					for (int k = 0; k < diff; k++) {
						temp += text[i+k + counter];
					}

					if (temp.compare(test) == 0) {
						if (isAlphaBeta(text[i + diff + counter]) == false && isAlphaBeta(text[i-counter]) == false) {
							for (int k = 0; k < counter; k++)keywordType[i - k] = 1;
							for (int k = 0; k < diff; k++)keywordType[i + k + 1] = 1;
							break;
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

	glm::vec2 updateCaretByScroll(bool upwards) {

		if (upwards > 0) {

			if (startY == 0)return caretPos;

			caretPos.y -= (rows + fontSize*0.5)*2;
			startY+=2;
		}
		else {

			caretPos.y += (rows + fontSize*0.5)*2;
			startY-=2;
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

			float diffX = abs(xpos - pos.x);

			if (diffX < minDistX) {
				minDistX = diffX;
				minX = x;// xpos;
				minIX = counter;
			}

			x += (ch.Advance >> 6) * scale;
			counter++;

		}

		caretPosI = glm::vec2(minIX,minIY);

		return glm::vec2(minX,minY);


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
};
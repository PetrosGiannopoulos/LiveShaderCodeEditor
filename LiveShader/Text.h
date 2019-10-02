#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <freetype\ftglyph.h>

class Text {

public:

	vector<string> codeText;

	Shader textShader;

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

public:

	Text() {

	}

	Text(Shader textShader) {
		
		this->textShader = textShader;
		//textShader = Shader("textShader.vs", "textShader.fs");

		//load font
	
		if (FT_Init_FreeType(&ft))cout << "ERROR::FREETYPE: Could not init FreeType Library" << endl;
		if (FT_New_Face(ft, "Fonts/arial.ttf", 0, &face))cout << "ERROR::FREETYPE: Failed to load font" << endl;

		fontSize = 24;
		FT_Set_Pixel_Sizes(face, 0, fontSize);

		loadCharacters();

		clearResources();

		codeText.push_back("if(i==b){");
		codeText.push_back("b=i;");
		codeText.push_back("}else if");
		
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

		//RenderText(textShader,"LEMAOOOOOOOOOOOOOOO", 100,height-200, 1.0f, glm::vec3(1,0,0));

		for (int i = 0; i < codeText.size();i++) {
			vector<int> keywordType = preprocessText(codeText[i]);
			RenderText(textShader, codeText[i], keywordType , 20, height - 100-i*(rows+fontSize*0.5), 1.0f, glm::vec3(0, 0, 0));
		}
	}

	vector<int> preprocessText(string text) {

		string testSet[] = { "if", "else", "class"};

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
					for (int k = 0; k < counter;k++) keywordType[i - k] = 1;
					app.clear();
					counter = 0;
					break;
				}
				else {

					int diff = test.length() - counter;
					string temp = app;
					for (int k = 0; k < diff; k++) {
						temp += text[i+k + counter];
					}

					if (temp.compare(test) == 0) {
						for (int k = 0; k < counter;k++)keywordType[i - k] = 1;
						for (int k = 0; k < diff; k++)keywordType[i + k+1] = 1;
						break;
					}
					
					if (j < numElements - 1) continue;
					

					if (app.length() == test.length()) {
						i = i - (counter-1);
						
						counter = 0;
						app.clear();
						
					}	

				}
			}
		}
		

		return keywordType;

	}

	void RenderText(Shader &shader, string text, vector<int> keywordType, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
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
			if (keywordType[counter] == 0)shader.setVec3("textColor", color);
			else if (keywordType[counter] == 1)shader.setVec3("textColor", glm::vec3(51/255.,131/255.,247/255.));
			counter++;
			Character ch = Characters[*c];

			GLfloat xpos = x + ch.Bearing.x * scale;
			GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

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

};
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

using namespace std;

class CShader
{
public:
	unsigned int ID;

	int workGroupSizeX;
	int workGroupSizeY;

	int work_grp_size[3], work_grp_inv, work_grp_count[3];

	std::string computeCode;

	CShader() {

	}

	// constructor generates the shader on the fly
	// ------------------------------------------------------------------------
	CShader(const char* computePath)
	{
		// 1. retrieve the vertex/fragment source code from filePath
		
		
		std::ifstream cShaderFile;
		
		// ensure ifstream objects can throw exceptions:
		cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			// open files
			cShaderFile.open(computePath);
			
			std::stringstream cShaderStream;
			// read file's buffer contents into streams
			cShaderStream << cShaderFile.rdbuf();
			
			// close file handlers
			cShaderFile.close();
			
			// convert stream into string
			computeCode = cShaderStream.str();
			
			
		}
		catch (std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}

		string uvMappingCode = preLoadMethod("Presets/Methods/UVCoordinatesMethod.method");
		string noiseCode = preLoadMethod("Presets/Methods/NoiseMethod.method");
		string intersectBoxesCode = preLoadMethod("Presets/Methods/IntersectBoxesMethod.method");
		string intersectTrianglesCode = preLoadMethod("Presets/Methods/IntersectTrianglesMethod.method");
		string intersectSpheresCode = preLoadMethod("Presets/Methods/IntersectSpheresMethod.method");
		string fresnelReflectAmountCode = preLoadMethod("Presets/Methods/FresnelReflectAmountMethod.method");
		string distanceMapCode = preLoadMethod("Presets/Methods/DistanceMapMethod.method");
		string rayMarchedLightingCode = preLoadMethod("Presets/Methods/RayMarchedLightingMethod.method");
		string blinnPhongModelCode = preLoadMethod("Presets/Methods/BlinnPhongModelMethod.method");

		string intersectCode = intersectBoxesCode + intersectTrianglesCode + intersectSpheresCode;

		//cout << uvMappingCode << endl;
		string shaderCode = uvMappingCode+noiseCode+intersectCode+fresnelReflectAmountCode+distanceMapCode+rayMarchedLightingCode+blinnPhongModelCode+computeCode;

		const char* cShaderCode = shaderCode.c_str();
		
		// 2. compile shaders
		unsigned int compute;
		// compute shader
		compute = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(compute, 1, &cShaderCode, NULL);
		glCompileShader(compute);
		checkCompileErrors(compute, "COMPUTE");
		
		// shader Program
		ID = glCreateProgram();
		glAttachShader(ID, compute);
	
		glLinkProgram(ID);
		checkCompileErrors(ID, "PROGRAM");
		// delete the shaders as they're linked into our program now and no longer necessery
		glDeleteShader(compute);

		//init compute program
		glUseProgram(ID);
		// maximum global work group (total work in a dispatch)
		
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_count[0]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_count[1]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_count[2]);
		// maximum local work group (one shader's slice)
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
		// maximum compute shader invocations (x * y * z)
		glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);

		workGroupSizeX = work_grp_count[0];
		workGroupSizeY = work_grp_count[1];
 
		//local group 1536, 1024 , 64
		//cout << work_grp_count[0] << endl;
		//cout << work_grp_count[1] << endl;
		//cout << work_grp_count[2] << endl;

		//global group 2147483647, 65535, 65535
		glUseProgram(0);
	}

	string preLoadMethod(string methodPath) {

		std::ifstream cMethodFile;
		string methodCode;
		// ensure ifstream objects can throw exceptions:
		cMethodFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			// open files
			cMethodFile.open(methodPath);

			std::stringstream cMethodStream;
			// read file's buffer contents into streams
			cMethodStream << cMethodFile.rdbuf();

			// close file handlers
			cMethodFile.close();

			// convert stream into string
			methodCode = cMethodStream.str();


		}
		catch (std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}

		return methodCode;

	}

	vector<string> getLines() {

		vector<string> lines;

		string app;
		for (int i = 0; i < computeCode.length();i++) {

			if (computeCode[i] == '\n') {
				if (app.length() == 0)app += ' ';
				lines.push_back(app);
				app.clear();
				continue;
			}

			

			if (computeCode[i] == '\t') {
				for (int i = 0; i < 4;i++) {
					app += ' ';
				}
			}
			else {
				app += computeCode[i];
			}
		}

		//cout << lines.size() << endl;

		return lines;
	}

	// activate the shader
	// ------------------------------------------------------------------------
	void use()
	{
		glUseProgram(ID);
	}
	// utility uniform functions
	// ------------------------------------------------------------------------
	void setBool(const std::string &name, bool value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
	}
	// ------------------------------------------------------------------------
	void setInt(const std::string &name, int value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	}
	// ------------------------------------------------------------------------
	void setFloat(const std::string &name, float value) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	}
	// ------------------------------------------------------------------------
	void setVec2(const std::string &name, const glm::vec2 &value) const
	{
		glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec2(const std::string &name, float x, float y) const
	{
		glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
	}
	// ------------------------------------------------------------------------
	void setVec3(const std::string &name, const glm::vec3 &value) const
	{
		glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec3(const std::string &name, float x, float y, float z) const
	{
		glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
	}
	// ------------------------------------------------------------------------
	void setVec4(const std::string &name, const glm::vec4 &value) const
	{
		glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec4(const std::string &name, float x, float y, float z, float w)
	{
		glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
	}
	// ------------------------------------------------------------------------
	void setMat2(const std::string &name, const glm::mat2 &mat) const
	{
		glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	// ------------------------------------------------------------------------
	void setMat3(const std::string &name, const glm::mat3 &mat) const
	{
		glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	// ------------------------------------------------------------------------
	void setMat4(const std::string &name, const glm::mat4 &mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

	int createFrameBufferTexture() {
		unsigned int tex;
		glGenTextures(1,&tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1000, 800, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);
		return tex;
	}


private:
	// utility function for checking shader compilation/linking errors.
	// ------------------------------------------------------------------------
	void checkCompileErrors(GLuint shader, std::string type)
	{
		GLint success;
		GLchar infoLog[1024];
		if (type != "PROGRAM")
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}
};
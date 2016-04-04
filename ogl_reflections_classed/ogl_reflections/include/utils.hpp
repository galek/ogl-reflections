#pragma once
#include "includes.hpp"

size_t tiled(const size_t &sz, const size_t &gmaxtile) {
	size_t dv = gmaxtile - (((sz - 1) % gmaxtile) + 1);
	size_t gs = sz + dv;
	return gs;
}

double milliseconds() {
	auto duration = std::chrono::high_resolution_clock::now();
	double millis = std::chrono::duration_cast<std::chrono::nanoseconds>(duration.time_since_epoch()).count();
	return millis / 1000000.0;
}

std::string readFile(const char *filePath) {
	std::string content;
	std::ifstream fileStream(filePath, std::ios::in);

	if (!fileStream.is_open()) {
		std::cerr << "Could not read file " << filePath << ". File does not exist." << std::endl;
		return "";
	}

	std::string line = "";
	while (!fileStream.eof()) {
		std::getline(fileStream, line);
		content.append(line + "\n");
	}

	fileStream.close();
	return content;
}

int validateShader(GLuint shader) {
	GLint isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
		errorLog.resize(maxLength);
		glDeleteShader(shader);
		std::string err(errorLog.begin(), errorLog.end());
		std::cout << err << std::endl;
		system("pause");
		exit(0);
		return 0;
	}
	return 1;
}

GLuint includeShader(std::string filename, std::string path) {
	std::string src = readFile(filename.c_str());
	const GLchar * csrc = src.c_str();
	const GLint len = src.size();
	glNamedStringARB(GL_SHADER_INCLUDE_ARB, path.size(), path.c_str(), len, csrc);
	return 1;
}

GLuint loadShader(std::string filename, GLuint type, std::vector<std::string>& ipath) {
	GLuint shader = glCreateShader(type);
	std::string src = readFile(filename.c_str());

	const GLchar * csrc = src.c_str();
	const GLint len = src.size();
	glShaderSource(shader, 1, &csrc, &len);

	std::vector<GLint> sizes;
	std::vector<const GLchar *> pstr;
	GLint count = 0;
	for (int i = 0;i < ipath.size();i++) {
		if (ipath[i] != "") {
			sizes.push_back(ipath[i].size());
			pstr.push_back(ipath[i].data());
			count++;
		}
	}
	//glCompileShaderIncludeARB(shader, count, pstr.data(), sizes.data());
	glCompileShader(shader);

	std::cout << "\n" << filename << "\n" << std::endl;
	int valid = validateShader(shader);
	return valid ? shader : -1;
}

GLuint loadShader(std::string filename, GLuint type, std::string ipath) {
	std::vector<std::string> a; a.push_back(ipath);
	return loadShader(filename, type, a);
}

GLuint loadShader(std::string filename, GLuint type) {
	return loadShader(filename, type, "");
}

GLuint loadWithDefault(std::string tex, glm::vec4 def) {
	sf::Image img_data;
	GLuint texture_handle;
	glGenTextures(1, &texture_handle);
	glBindTexture(GL_TEXTURE_2D, texture_handle);
	if (tex != "" && img_data.loadFromFile(tex)) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_data.getSize().x, img_data.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data.getPixelsPtr());
	}
	else {

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, glm::value_ptr(def));
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	return texture_handle;
}

GLuint loadBump(std::string tex) {
	return loadWithDefault(tex, glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));
}

GLuint loadDiffuse(std::string tex) {
	return loadWithDefault(tex, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

GLuint loadSpecular(std::string tex) {
	return loadWithDefault(tex, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}
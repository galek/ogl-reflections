// Link statically with GLEW
#define GLEW_STATIC
#define _USE_MATH_DEFINES
//#define GLM_GTC_matrix_transform

// Headers
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <tiny_obj_loader/tiny_obj_loader.h>
#include <ctime>

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

struct Thash {
	GLuint triangle = 0xFFFFFFFF;
	GLuint previous = 0xFFFFFFFF;
};

struct TexelInfo {
	glm::vec4 position;
	//glm::uvec4 texel;
	GLuint triangle = 0xFFFFFFFF;
	//float dist = 10000.0f;
	GLuint previous = 0xFFFFFFFF;
};

size_t tiled(const size_t &sz, const size_t &gmaxtile) {
	size_t dv = gmaxtile - (((sz - 1) % gmaxtile) + 1);
	size_t gs = sz + dv;
	return gs;
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

GLuint loadShader(std::string filename, GLuint type) {
	GLuint shader = glCreateShader(type);
	std::string src = readFile(filename.c_str());
	const GLchar * csrc = src.c_str();
	const GLint len = src.size();
	glShaderSource(shader, 1, &csrc, &len);
	glCompileShader(shader);

	std::cout << "\n" << filename << "\n" << std::endl;
	int valid = validateShader(shader);
	return valid ? shader : -1;
}

const GLuint width = 800;
const GLuint height = 600;
const GLuint _zero = 0;
const GLuint cube_s = 128;

void prepare_cubemaps(std::vector<GLuint>& fbcubetextures, GLuint &frameBuffer) {
	glGenTextures(fbcubetextures.size(), &fbcubetextures[0]);

	GLenum target = GL_TEXTURE_CUBE_MAP;
		for (int j = 0;j < fbcubetextures.size();j++) {

			glBindTexture(target, fbcubetextures[j]);
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			for (int face = 0; face < 6; ++face) {
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_R32UI, cube_s, cube_s, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, 0);
			}
		}

	GLuint tempBuffer;
	GLuint rboDepthStencil;

	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

	glGenTextures(1, &tempBuffer);
	glBindTexture(GL_TEXTURE_2D, tempBuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cube_s, cube_s, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tempBuffer, 0);

	glGenRenderbuffers(1, &rboDepthStencil);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepthStencil);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, cube_s, cube_s);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void prepare_projections(glm::mat4 looks[6], glm::mat4 &proj, glm::vec3 center) {
	proj = glm::perspective((float)M_PI / 2.0f, 1.0f, 0.0001f, 10000.0f);
	looks[0] = glm::lookAt(glm::vec3(0.0), glm::vec3(1.0, 0.0, 0.0), -glm::vec3(0.0, 1.0, 0.0));
	looks[1] = glm::lookAt(glm::vec3(0.0), glm::vec3(-1.0, 0.0, 0.0), -glm::vec3(0.0, 1.0, 0.0));
	looks[2] = glm::lookAt(glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0), -glm::vec3(0.0, 0.0, 1.0));
	looks[3] = glm::lookAt(glm::vec3(0.0), glm::vec3(0.0, -1.0, 0.0), -glm::vec3(0.0, 0.0, 1.0));
	looks[4] = glm::lookAt(glm::vec3(0.0), glm::vec3(0.0, 0.0, 1.0), -glm::vec3(0.0, 1.0, 0.0));
	looks[5] = glm::lookAt(glm::vec3(0.0), glm::vec3(0.0, 0.0, -1.0), -glm::vec3(0.0, 1.0, 0.0));
}


int main()
{
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	sf::Window window(sf::VideoMode(width, height, 32), "OpenGL", sf::Style::Titlebar | sf::Style::Close, settings);

	glewExperimental = GL_TRUE;
	glewInit();

	GLuint vbo;
	{
		glGenBuffers(1, &vbo);
		GLfloat vertices[] = {
			-1.0f, -1.0f,
			-1.0f,  1.0f,
			 1.0f,  1.0f,
			 1.0f, -1.0f
		};
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	}

	GLuint ebo;
	{
		glGenBuffers(1, &ebo);
		GLuint elements[] = {
			0, 1, 2,
			2, 3, 0
		};
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
	}

	std::string inputfile = "models/teapot.obj";
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	bool ret = tinyobj::LoadObj(shapes, materials, err, inputfile.c_str());

	unsigned id = 0;
	std::vector<float>& vertices = shapes[id].mesh.positions;
	std::vector<unsigned>& indices = shapes[id].mesh.indices;
	std::vector<float>& normals = shapes[id].mesh.normals;

	GLuint vbo_triangle;
	GLuint vbo_triangle_ssbo;
	{
		glGenBuffers(1, &vbo_triangle);
		glGenBuffers(1, &vbo_triangle_ssbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbo_triangle_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);
	}

	GLuint ebo_triangle;
	GLuint ebo_triangle_ssbo;
	{
		glGenBuffers(1, &ebo_triangle);
		glGenBuffers(1, &ebo_triangle_ssbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_triangle);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * indices.size(), indices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ebo_triangle_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * indices.size(), indices.data(), GL_DYNAMIC_DRAW);
	}

	GLuint norm_triangle;
	GLuint norm_triangle_ssbo;
	{
		glGenBuffers(1, &norm_triangle);
		glGenBuffers(1, &norm_triangle_ssbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, norm_triangle);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(float) * normals.size(), normals.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, norm_triangle_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * normals.size(), normals.data(), GL_DYNAMIC_DRAW);
	}

	GLuint cubemapProgram;
	{
		GLuint vertexShader = loadShader("./cubemapper/cubemapper.vert", GL_VERTEX_SHADER);
		GLuint geometryShader = loadShader("./cubemapper/cubemapper.geom", GL_GEOMETRY_SHADER);
		GLuint fragmentShader = loadShader("./cubemapper/cubemapper.frag", GL_FRAGMENT_SHADER);

		cubemapProgram = glCreateProgram();
		glAttachShader(cubemapProgram, vertexShader);
		glAttachShader(cubemapProgram, geometryShader);
		glAttachShader(cubemapProgram, fragmentShader);
		glBindFragDataLocation(cubemapProgram, 0, "outColor");
		glLinkProgram(cubemapProgram);
		glUseProgram(cubemapProgram);
	}

	GLuint cubemapClearProgram;
	{
		GLuint vertexShader = loadShader("./cubemapper/clear.vert", GL_VERTEX_SHADER);
		GLuint geometryShader = loadShader("./cubemapper/clear.geom", GL_GEOMETRY_SHADER);
		GLuint fragmentShader = loadShader("./cubemapper/clear.frag", GL_FRAGMENT_SHADER);

		cubemapClearProgram = glCreateProgram();
		glAttachShader(cubemapClearProgram, vertexShader);
		glAttachShader(cubemapClearProgram, geometryShader);
		glAttachShader(cubemapClearProgram, fragmentShader);
		glBindFragDataLocation(cubemapClearProgram, 0, "outColor");
		glLinkProgram(cubemapClearProgram);
		glUseProgram(cubemapClearProgram);
	}

	GLuint renderProgram;
	{
		GLuint vertexShader = loadShader("./render/render.vert", GL_VERTEX_SHADER);
		GLuint fragmentShader = loadShader("./render/render.frag", GL_FRAGMENT_SHADER);

		renderProgram = glCreateProgram();
		glAttachShader(renderProgram, vertexShader);
		glAttachShader(renderProgram, fragmentShader);
		glBindFragDataLocation(renderProgram, 0, "outColor");
		glLinkProgram(renderProgram);
		glUseProgram(renderProgram);
	}

	glUseProgram(cubemapProgram);
	glEnableVertexAttribArray(glGetAttribLocation(cubemapProgram, "position"));

	glUseProgram(renderProgram);
	glEnableVertexAttribArray(glGetAttribLocation(renderProgram, "position"));

	GLuint texelInfoBuf;
	GLuint texelCounter;
	{
		glGenBuffers(1, &texelInfoBuf);
		glGenBuffers(1, &texelCounter);
	}

	clock_t t = clock();


	GLuint fbo;
	std::vector<GLuint> cubemaps(2);
	prepare_cubemaps(cubemaps, fbo);

	GLuint vsize = 512 * 512 * 128;
	std::vector<TexelInfo> infos(vsize);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texelInfoBuf);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, infos.size() * sizeof(TexelInfo), nullptr, GL_DYNAMIC_STORAGE_BIT);










	bool running = true;
	while (running)
	{
		sf::Event windowEvent;
		while (window.pollEvent(windowEvent))
		{
			switch (windowEvent.type)
			{
			case sf::Event::Closed:
				running = false;
				break;
			}
		}


		clock_t tt = clock();
		clock_t c = tt - t;

		glm::vec3 eye = glm::vec3(9.0, 9.0, 9.0);
		glm::vec3 view = glm::vec3(0.0, 6.0, 0.0);
		//glm::vec3 eye = glm::vec3(3.0, 0.0, 3.0);
		//glm::vec3 view = glm::vec3(0.0, 0.0, 0.0);
		eye -= view;
		eye = rotate(eye, ((float)c) / 1000.0f, glm::vec3(0.0, 1.0, 0.0));
		eye += view;










		glm::mat4 cams[6];
		glm::mat4 proj;
		glm::vec3 center = eye;
		prepare_projections(cams, proj, center);

		std::vector<float> floats(6 * 16);
		unsigned off = 0;
		for (int i = 0;i < 6;i++) {
			float * arr = glm::value_ptr(cams[i]);
			for (int j = 0;j < 16;j++) {
				floats[off] = arr[j];
				off++;
			}
		}

		{
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, texelCounter);
			glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);
		}


		{ //CLEAR CUBEMAP
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glUseProgram(0);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glBlendFunc(GL_ONE, GL_ONE);
			glEnable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
			glUseProgram(cubemapClearProgram);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);

			glBindImageTexture(0, cubemaps[0], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
			glBindImageTexture(1, cubemaps[1], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, texelCounter);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texelInfoBuf);

			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glVertexAttribPointer(glGetAttribLocation(cubemapClearProgram, "position"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glViewport(0, 0, width, height);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}


		{ //RENDER CUBEMAP
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glUseProgram(0);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glBlendFunc(GL_ONE, GL_ONE);
			glEnable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
			glSubpixelPrecisionBiasNV(GL_SUBPIXEL_PRECISION_BIAS_X_BITS_NV, GL_SUBPIXEL_PRECISION_BIAS_Y_BITS_NV);
			glEnable(GL_CONSERVATIVE_RASTERIZATION_NV);
			glUseProgram(cubemapProgram);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);

			glBindImageTexture(0, cubemaps[0], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
			glBindImageTexture(1, cubemaps[1], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, texelCounter);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texelInfoBuf);

			glUniformMatrix4fv(glGetUniformLocation(cubemapProgram, "proj"), 1, false, glm::value_ptr(proj));
			glUniformMatrix4fv(glGetUniformLocation(cubemapProgram, "cams"), 6, false, floats.data());
			glUniform3fv(glGetUniformLocation(cubemapProgram, "center"), 1, glm::value_ptr(center));

			glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle);
			glVertexAttribPointer(glGetAttribLocation(cubemapProgram, "position"), 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_triangle);
			glViewport(0, 0, cube_s, cube_s);
			glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}

















		{
			glUseProgram(0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
			glCullFace(GL_FRONT_AND_BACK);
			glUseProgram(renderProgram);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texelInfoBuf);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, vbo_triangle_ssbo);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ebo_triangle_ssbo);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, norm_triangle_ssbo);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemaps[0]);
			glUniform1i(glGetUniformLocation(renderProgram, "texelCounts"), 0);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemaps[1]);
			glUniform1i(glGetUniformLocation(renderProgram, "texelLasts"), 1);

			glUniformMatrix4fv(glGetUniformLocation(renderProgram, "projInv"), 1, false, glm::value_ptr(glm::inverse(glm::perspective(3.14f / 3.0f, float(width) / float(height), 0.001f, 1000.0f))));
			glUniformMatrix4fv(glGetUniformLocation(renderProgram, "camInv"), 1, false, glm::value_ptr(glm::inverse(glm::lookAt(eye, view, glm::vec3(0.0, 1.0, 0.0)))));
			glUniform2f(glGetUniformLocation(renderProgram, "sceneRes"), float(width), float(height));
			glUniform3fv(glGetUniformLocation(renderProgram, "center"), 1, glm::value_ptr(center));

			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glVertexAttribPointer(glGetAttribLocation(renderProgram, "position"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glViewport(0, 0, width, height);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}

		window.display();
	}

	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);
	window.close();

	return 0;
}
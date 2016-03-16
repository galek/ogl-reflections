// Link statically with GLEW
#define GLEW_STATIC
#define GLM_SWIZZLE 
#define GLM_SWIZZLE_XYZW 
//#define GLM_GTC_matrix_transform

// Headers
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/component_wise.hpp>
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

struct Voxel {
	GLuint last = 0xFFFFFFFF;
	GLuint count = 0;
	GLuint coordX = 0;
	GLuint coordY = 0;
	GLuint coordZ = 0;
};

struct Minmax {
	glm::vec4 mn;
	glm::vec4 mx;
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
	//std::cout << src << std::endl;
	int valid = validateShader(shader);
	return valid ? shader : -1;
}

const GLuint width = 800;
const GLuint height = 600;
const GLuint maxDepth = 8; //Size3D = 2^(X-1)
const GLuint _zero = 0;

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

	GLuint voxelizerFixProgram;
	{
		GLuint compShader = loadShader("./voxelizer/fix.comp", GL_COMPUTE_SHADER);
		voxelizerFixProgram = glCreateProgram();
		glAttachShader(voxelizerFixProgram, compShader);
		glBindFragDataLocation(voxelizerFixProgram, 0, "outColor");
		glLinkProgram(voxelizerFixProgram);
		glUseProgram(voxelizerFixProgram);
	}

	GLuint voxelizerMinmaxProgram;
	{
		GLuint compShader = loadShader("./voxelizer/minmax.comp", GL_COMPUTE_SHADER);
		voxelizerMinmaxProgram = glCreateProgram();
		glAttachShader(voxelizerMinmaxProgram, compShader);
		glBindFragDataLocation(voxelizerMinmaxProgram, 0, "outColor");
		glLinkProgram(voxelizerMinmaxProgram);
		glUseProgram(voxelizerMinmaxProgram);
	}

	GLuint voxelizerProgram;
	{
		GLuint vertexShader = loadShader("./voxelizer/voxelizer.vert", GL_VERTEX_SHADER);
		GLuint geometryShader = loadShader("./voxelizer/voxelizer.geom", GL_GEOMETRY_SHADER);
		GLuint fragmentShader = loadShader("./voxelizer/voxelizer.frag", GL_FRAGMENT_SHADER);

		voxelizerProgram = glCreateProgram();
		glAttachShader(voxelizerProgram, vertexShader);
		glAttachShader(voxelizerProgram, geometryShader);
		glAttachShader(voxelizerProgram, fragmentShader);
		glBindFragDataLocation(voxelizerProgram, 0, "outColor");
		glLinkProgram(voxelizerProgram);
		glUseProgram(voxelizerProgram);
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

	glUseProgram(voxelizerProgram);
	glEnableVertexAttribArray(glGetAttribLocation(voxelizerProgram, "position"));

	glUseProgram(renderProgram);
	glEnableVertexAttribArray(glGetAttribLocation(renderProgram, "position"));




	GLuint vspace;
	GLuint tspace;
	GLuint subgrid;
	GLuint vcounter;
	GLuint scounter;

	glGenBuffers(1, &vspace);
	glGenBuffers(1, &tspace);
	glGenBuffers(1, &subgrid);
	glGenBuffers(1, &vcounter);
	glGenBuffers(1, &scounter);



	GLuint vsize = sizeof(Voxel) * 128 * 128 * 128;
	GLuint tsize = sizeof(Thash) * 512 * 512 * 64;
	GLuint subgridc = sizeof(GLuint) * 128 * 128 * 128 * 8;

	std::vector<Voxel> dvoxels(vsize / sizeof(Voxel), Voxel());
	std::vector<unsigned> dvoxels_subgrid(subgridc / sizeof(unsigned), 0xFFFFFFFF);
	std::vector<Thash> dthash(tsize / sizeof(Thash), Thash());

	GLuint frameBuffer;
	GLuint tempBuffer;
	GLuint rboDepthStencil;

	{
		glGenFramebuffers(1, &frameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

		glGenTextures(1, &tempBuffer);
		glBindTexture(GL_TEXTURE_2D, tempBuffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tempBuffer, 0);

		glGenRenderbuffers(1, &rboDepthStencil);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepthStencil);
	}
	
	
	

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, tspace);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, tsize, nullptr, GL_DYNAMIC_STORAGE_BIT /*| GL_SPARSE_STORAGE_BIT_ARB*/);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, vspace);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, vsize, nullptr, GL_DYNAMIC_STORAGE_BIT /*| GL_SPARSE_STORAGE_BIT_ARB*/);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, subgrid);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, subgridc, nullptr, GL_DYNAMIC_STORAGE_BIT /*| GL_SPARSE_STORAGE_BIT_ARB*/);

	

	

	
	GLuint mcounter;
	GLuint minmaxBuf;

	glGenBuffers(1, &mcounter);
	glGenBuffers(1, &minmaxBuf);

	unsigned trisize = indices.size() / 3;
	{
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, mcounter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, minmaxBuf);
		unsigned size = indices.size() / 3;

		unsigned scounter = size;
		unsigned csize = size;
		do {
			size = size / 2 + size % 2;
			scounter += size;
		} while (size > 1);

		glBufferStorage(GL_SHADER_STORAGE_BUFFER, scounter * sizeof(Minmax), nullptr, GL_DYNAMIC_STORAGE_BIT);
	}

	Minmax bound;
	GLuint count = 0;
	GLuint prev = 0;
	GLuint from = 0;
	do {
		prev = from;

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, mcounter);
		glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &from);

		GLuint gcount = (from - prev);
		count = from < 1 ? ((trisize / 2) + (trisize % 2)) : ((gcount / 2 + gcount % 2));

		glUseProgram(voxelizerMinmaxProgram);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, minmaxBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_triangle_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ebo_triangle_ssbo);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, mcounter);

		glUniform1ui(glGetUniformLocation(voxelizerMinmaxProgram, "count"), count);
		glUniform1ui(glGetUniformLocation(voxelizerMinmaxProgram, "prev"), prev);
		glUniform1ui(glGetUniformLocation(voxelizerMinmaxProgram, "from"), from);
		glUniform1ui(glGetUniformLocation(voxelizerMinmaxProgram, "tcount"), trisize);

		GLuint dsize = tiled(count, 256);
		glDispatchCompute((dsize < 256 ? 256 : dsize) / 256, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	} while (count > 1);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, mcounter);
	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &from);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, minmaxBuf);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, (from - 1) * sizeof(Minmax), sizeof(Minmax), &bound);





	glm::vec3 scale(glm::compMax(bound.mx.xyz - bound.mn.xyz));
	glm::vec3 offset((bound.mx.xyz + bound.mx.xyz) / 2.0f - scale);

	{
		glUseProgram(0);

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, vcounter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, scounter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);

		//glBindBuffer(GL_SHADER_STORAGE_BUFFER, tspace);
		//glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, tsize, dthash.data());

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vspace);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, vsize, dvoxels.data());

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, subgrid);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, subgridc, dvoxels_subgrid.data());

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	GLuint lscounter;
	GLuint lscounter_to;

	GLuint helper;
	GLuint helper_to;
	{
		glGenBuffers(1, &helper);
		glGenBuffers(1, &lscounter);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, helper);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_STORAGE_BIT);

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, lscounter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);
	}

	GLuint size = 1;
	for (int i = 0;i < maxDepth;i++) {
		glUseProgram(0);

		{
			glGenBuffers(1, &helper_to);
			glGenBuffers(1, &lscounter_to);
		}

		{
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, lscounter_to);
			glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, helper_to);
			glBufferStorage(GL_SHADER_STORAGE_BUFFER, size * sizeof(GLuint), nullptr, GL_DYNAMIC_STORAGE_BIT);
		}

		{
			glUseProgram(voxelizerFixProgram);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vspace);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, subgrid);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, tspace);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, vbo_triangle_ssbo);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ebo_triangle_ssbo);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, helper_to);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, helper);

			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, vcounter);
			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, scounter);
			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, lscounter_to);
			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 3, lscounter);

			glUniform1ui(glGetUniformLocation(voxelizerFixProgram, "maxDepth"), maxDepth);
			glUniform1ui(glGetUniformLocation(voxelizerFixProgram, "currentDepth"), i);
			glUniform3fv(glGetUniformLocation(voxelizerFixProgram, "offset"), 1, glm::value_ptr(offset));
			glUniform3fv(glGetUniformLocation(voxelizerFixProgram, "scale"), 1, glm::value_ptr(scale));

			GLuint tsize = tiled(size, 256);
			glDispatchCompute((tsize < 256 ? 256 : tsize) / 256, 1, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			GLuint osize = size;
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, lscounter_to);
			glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &size);
			size *= 8;
		}

		{
			unsigned size_r = pow(2, i);

			glBindRenderbuffer(GL_RENDERBUFFER, rboDepthStencil);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size_r, size_r);

			glBindTexture(GL_TEXTURE_2D, tempBuffer);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size_r, size_r, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glDisable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
			glSubpixelPrecisionBiasNV(GL_SUBPIXEL_PRECISION_BIAS_X_BITS_NV, GL_SUBPIXEL_PRECISION_BIAS_Y_BITS_NV);
			glEnable(GL_CONSERVATIVE_RASTERIZATION_NV);
			glCullFace(GL_FRONT_AND_BACK);

			glUseProgram(voxelizerProgram);
			glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, vcounter);
			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, scounter);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, tspace);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, subgrid);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vspace);

			glUniform1ui(glGetUniformLocation(voxelizerProgram, "maxDepth"), maxDepth);
			glUniform1ui(glGetUniformLocation(voxelizerProgram, "currentDepth"), i);
			glUniform3fv(glGetUniformLocation(voxelizerProgram, "offset"), 1, glm::value_ptr(offset));
			glUniform3fv(glGetUniformLocation(voxelizerProgram, "scale"), 1, glm::value_ptr(scale));

			glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle);
			glVertexAttribPointer(glGetAttribLocation(voxelizerProgram, "position"), 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_triangle);
			glViewport(0, 0, size_r, size_r);
			glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			GLuint tsz = 0;
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, vcounter);
			glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &tsz);

			GLuint vsz = 0;
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, scounter);
			glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &vsz);
		}

		lscounter = lscounter_to;
		helper = helper_to;
	}









	

	





	clock_t t = clock();

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

		








		










		

		

		{
			glUseProgram(0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glDisable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
			glCullFace(GL_FRONT_AND_BACK);
			glUseProgram(renderProgram);

			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, vcounter);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vspace);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, subgrid);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, tspace);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, vbo_triangle_ssbo);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ebo_triangle_ssbo);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, norm_triangle_ssbo);

			glUniformMatrix4fv(glGetUniformLocation(renderProgram, "projInv"), 1, false, glm::value_ptr(glm::inverse(glm::perspective(3.14f / 3.0f, float(width) / float(height), 0.001f, 1000.0f))));
			glUniformMatrix4fv(glGetUniformLocation(renderProgram, "camInv"), 1, false, glm::value_ptr(glm::inverse(glm::lookAt(eye, view, glm::vec3(0.0, 1.0, 0.0)))));
			glUniform2f(glGetUniformLocation(renderProgram, "sceneRes"), float(width), float(height));
			glUniform3fv(glGetUniformLocation(renderProgram, "offset"), 1, glm::value_ptr(offset));
			glUniform3fv(glGetUniformLocation(renderProgram, "scale"), 1, glm::value_ptr(scale));
			glUniform1ui(glGetUniformLocation(renderProgram, "maxDepth"), maxDepth);

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
// Link statically with GLEW
#define GLEW_STATIC
#define GLM_SWIZZLE 
#define GLM_SWIZZLE_XYZW 
//#define GLM_GTC_matrix_transform

// Headers
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
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

const GLuint width = 1280;
const GLuint height = 720;
const GLuint maxDepth = 9; //Size3D = 2^(X-1)
const GLuint _zero = 0;

struct Ray {
	GLint active;
	glm::vec4 origin;
	glm::vec4 direct;
	glm::vec4 color;
	glm::vec4 final;
	glm::vec4 params;
};

struct Hit {
	GLfloat dist;
	GLuint triangle;
	GLuint materialID;
	glm::vec4 normal;
	glm::vec4 tangent;
	glm::vec4 texcoord;
	glm::vec4 params;
};




class RObject {
private:
	GLuint beginProgram;
	GLuint cameraProgram;
	GLuint renderProgram;
	GLuint closeProgram;
	GLuint rays;
	GLuint hits;

	GLuint vbo;
	GLuint ebo;

	GLuint cubeTex;

public:
	RObject() {
		init();
	}

private:
	void init() {
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
		
		{
			glGenBuffers(1, &ebo);
			GLuint elements[] = {
				0, 1, 2,
				2, 3, 0
			};
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
		}

		GLuint rsize = width * height * 2;

		glGenBuffers(1, &rays);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, rays);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, rsize * sizeof(Ray), nullptr, GL_DYNAMIC_STORAGE_BIT);

		glGenBuffers(1, &hits);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, hits);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, rsize * sizeof(Hit), nullptr, GL_DYNAMIC_STORAGE_BIT);

		{
			GLuint compShader = loadShader("./render/begin.comp", GL_COMPUTE_SHADER);
			beginProgram = glCreateProgram();
			glAttachShader(beginProgram, compShader);
			glBindFragDataLocation(beginProgram, 0, "outColor");
			glLinkProgram(beginProgram);
			glUseProgram(beginProgram);
		}

		{
			GLuint compShader = loadShader("./render/close.comp", GL_COMPUTE_SHADER);
			closeProgram = glCreateProgram();
			glAttachShader(closeProgram, compShader);
			glBindFragDataLocation(closeProgram, 0, "outColor");
			glLinkProgram(closeProgram);
			glUseProgram(closeProgram);
		}

		{
			GLuint compShader = loadShader("./render/camera.comp", GL_COMPUTE_SHADER);
			cameraProgram = glCreateProgram();
			glAttachShader(cameraProgram, compShader);
			glBindFragDataLocation(cameraProgram, 0, "outColor");
			glLinkProgram(cameraProgram);
			glUseProgram(cameraProgram);
		}

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

		glUseProgram(renderProgram);
		glEnableVertexAttribArray(glGetAttribLocation(renderProgram, "position"));
	}

public:
	void includeCubemap(GLuint cube) {
		cubeTex = cube;
	}

	void camera(glm::vec3 eye, glm::vec3 view) {
		glUseProgram(cameraProgram);
		
		bind(); 

		glUniformMatrix4fv(glGetUniformLocation(cameraProgram, "projInv"), 1, false, glm::value_ptr(glm::inverse(glm::perspective(3.14f / 3.0f, float(width) / float(height), 0.1f, 10000.0f))));
		glUniformMatrix4fv(glGetUniformLocation(cameraProgram, "camInv"), 1, false, glm::value_ptr(glm::inverse(glm::lookAt(eye, view, glm::vec3(0.0, 1.0, 0.0)))));
		glUniform2f(glGetUniformLocation(cameraProgram, "sceneRes"), width, height);

		glDispatchCompute(tiled(width, 16) / 16, tiled(height, 16) / 16, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void begin() {
		glUseProgram(beginProgram);
		bind();

		glUniform2f(glGetUniformLocation(beginProgram, "sceneRes"), width, height);
		glDispatchCompute(tiled(width, 16) / 16, tiled(height, 16) / 16, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void close() {
		glUseProgram(closeProgram);
		bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cubeTex);
		glUniform1i(glGetUniformLocation(closeProgram, "cubeTex"), 0);

		glUniform2f(glGetUniformLocation(closeProgram, "sceneRes"), width, height);
		glDispatchCompute(tiled(width, 16) / 16, tiled(height, 16) / 16, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void bind() {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, rays);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, hits);
	}

	void render() {
		glUseProgram(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
		glCullFace(GL_FRONT_AND_BACK);
		glUseProgram(renderProgram);

		bind();

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glUniform2f(glGetUniformLocation(renderProgram, "sceneRes"), width, height);
		glVertexAttribPointer(glGetAttribLocation(renderProgram, "position"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glViewport(0, 0, width, height);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
};








class TObject {
public:
	TObject() {
		init();
	}

private:
	GLuint vsize = sizeof(Voxel) * 256 * 256 * 256;
	GLuint tsize = sizeof(Thash) * 256 * 256 * 256;
	GLuint subgridc = sizeof(GLuint) * 256 * 256 * 256 * 8;

	std::vector<Voxel> dvoxels;
	std::vector<GLuint> dvoxels_subgrid;
	std::vector<Thash> dthash;

	GLuint ebo_triangle_ssbo;
	GLuint vbo_triangle_ssbo;
	GLuint norm_triangle_ssbo;
	GLuint tex_triangle_ssbo;
	GLuint mat_triangle_ssbo;
	GLuint voxelizerFixProgram;
	GLuint voxelizerMinmaxProgram;
	GLuint voxelizerProgram;

	GLuint vspace;
	GLuint tspace;
	GLuint subgrid;
	GLuint vcounter;
	GLuint scounter;

	GLuint frameBuffer;
	GLuint tempBuffer;
	GLuint rboDepthStencil;

	Minmax bound;
	GLuint triangleCount = 0;
	GLuint materialID = 0;

	GLuint intersectionProgram;

	glm::vec3 offset;
	glm::vec3 scale;

public:
	void setMaterialID(GLuint id) {
		materialID = id;
	}

	void loadMesh(std::vector<tinyobj::shape_t>& shape) {
		std::vector<float> vertices;
		std::vector<unsigned> indices;
		std::vector<float> normals;
		std::vector<float> texcoords;
		std::vector<int> material_ids;

		for (unsigned i = 0;i < shape.size();i++) {
			for (unsigned j = 0;j < shape[i].mesh.indices.size();j++) {
				indices.push_back(shape[i].mesh.indices[j] + vertices.size() / 3);
			}
			for (unsigned j = 0;j < shape[i].mesh.positions.size();j++) {
				vertices.push_back(shape[i].mesh.positions[j]);
			}
			for (unsigned j = 0;j < shape[i].mesh.normals.size();j++) {
				normals.push_back(shape[i].mesh.normals[j]);
			}
			for (unsigned j = 0;j < shape[i].mesh.texcoords.size();j++) {
				texcoords.push_back(shape[i].mesh.texcoords[j]);
			}
			for (unsigned j = 0;j < shape[i].mesh.material_ids.size();j++) {
				material_ids.push_back(shape[i].mesh.material_ids[j]);
			}
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbo_triangle_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ebo_triangle_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * indices.size(), indices.data(), GL_DYNAMIC_DRAW);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, norm_triangle_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * normals.size(), normals.data(), GL_DYNAMIC_DRAW);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, tex_triangle_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * texcoords.size(), texcoords.data(), GL_DYNAMIC_DRAW);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, mat_triangle_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * material_ids.size(), material_ids.data(), GL_DYNAMIC_DRAW);

		triangleCount = indices.size() / 3;
	}

private:
	void init() {
		//dvoxels_subgrid = std::vector<unsigned>(subgridc / sizeof(unsigned), 0xFFFFFFFF);
		//dvoxels = std::vector<Voxel>(vsize / sizeof(Voxel), Voxel());
		//dthash = std::vector<Thash>(tsize / sizeof(Thash), Thash());

		glGenBuffers(1, &vspace);
		glGenBuffers(1, &tspace);
		glGenBuffers(1, &subgrid);
		glGenBuffers(1, &vcounter);
		glGenBuffers(1, &scounter);
		glGenBuffers(1, &vbo_triangle_ssbo);
		glGenBuffers(1, &ebo_triangle_ssbo);
		glGenBuffers(1, &norm_triangle_ssbo);
		glGenBuffers(1, &tex_triangle_ssbo);
		glGenBuffers(1, &mat_triangle_ssbo);

		{
			GLuint compShader = loadShader("./render/intersection.comp", GL_COMPUTE_SHADER);
			intersectionProgram = glCreateProgram();
			glAttachShader(intersectionProgram, compShader);
			glBindFragDataLocation(intersectionProgram, 0, "outColor");
			glLinkProgram(intersectionProgram);
			glUseProgram(intersectionProgram);
		}

		{
			GLuint compShader = loadShader("./voxelizer/fix.comp", GL_COMPUTE_SHADER);
			voxelizerFixProgram = glCreateProgram();
			glAttachShader(voxelizerFixProgram, compShader);
			glBindFragDataLocation(voxelizerFixProgram, 0, "outColor");
			glLinkProgram(voxelizerFixProgram);
			glUseProgram(voxelizerFixProgram);
		}


		{
			GLuint compShader = loadShader("./voxelizer/minmax.comp", GL_COMPUTE_SHADER);
			voxelizerMinmaxProgram = glCreateProgram();
			glAttachShader(voxelizerMinmaxProgram, compShader);
			glBindFragDataLocation(voxelizerMinmaxProgram, 0, "outColor");
			glLinkProgram(voxelizerMinmaxProgram);
			glUseProgram(voxelizerMinmaxProgram);
		}


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

		glUseProgram(voxelizerProgram);
		glEnableVertexAttribArray(glGetAttribLocation(voxelizerProgram, "position"));

		glGenFramebuffers(1, &frameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

		glGenTextures(1, &tempBuffer);
		glBindTexture(GL_TEXTURE_2D, tempBuffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tempBuffer, 0);

		glGenRenderbuffers(1, &rboDepthStencil);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepthStencil);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, tspace);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, tsize, nullptr, GL_DYNAMIC_STORAGE_BIT);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vspace);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, vsize, nullptr, GL_DYNAMIC_STORAGE_BIT);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, subgrid);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, subgridc, nullptr, GL_DYNAMIC_STORAGE_BIT);
	}

public:
	void calcMinmax() {
		GLuint mcounter;
		GLuint minmaxBuf;

		glGenBuffers(1, &mcounter);
		glGenBuffers(1, &minmaxBuf);

		{
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, mcounter);
			glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);

			GLuint size = triangleCount;
			GLuint scounter = size;
			GLuint csize = size;
			do {
				size = size / 2 + size % 2;
				scounter += size;
			} while (size > 1);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, minmaxBuf);
			glBufferStorage(GL_SHADER_STORAGE_BUFFER, scounter * sizeof(Minmax), nullptr, GL_DYNAMIC_STORAGE_BIT);
		}

		GLuint count = 0;
		GLuint prev = 0;
		GLuint from = 0;
		GLuint trisize = triangleCount;
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
	}

	void bind() {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, vbo_triangle_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ebo_triangle_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, norm_triangle_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, tex_triangle_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, mat_triangle_ssbo);
	}

	void bindOctree() {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vspace);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, subgrid);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, tspace);
	}

	void intersection(RObject &rays) {
		glUseProgram(intersectionProgram);

		bindOctree();
		bind();

		rays.bind();

		glUniform2f(glGetUniformLocation(intersectionProgram, "sceneRes"), width, height);
		glUniform1ui(glGetUniformLocation(intersectionProgram, "maxDepth"), maxDepth);
		glUniform3fv(glGetUniformLocation(intersectionProgram, "offset"), 1, glm::value_ptr(offset));
		glUniform3fv(glGetUniformLocation(intersectionProgram, "scale"), 1, glm::value_ptr(scale));
		glUniform1ui(glGetUniformLocation(intersectionProgram, "materialID"), materialID);

		glDispatchCompute(tiled(width, 16) / 16, tiled(height, 16) / 16, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void clearOctree() {
		glUseProgram(0);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, vcounter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, scounter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);
		//glBindBuffer(GL_SHADER_STORAGE_BUFFER, subgrid);
		//glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, subgridc, dvoxels_subgrid.data());
		//glBindBuffer(GL_SHADER_STORAGE_BUFFER, vspace);
		//glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Voxel), dvoxels.data());
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void buildOctree() {
		scale = glm::vec3(glm::compMax(bound.mx.xyz - bound.mn.xyz));
		offset = glm::vec3((bound.mx.xyz + bound.mx.xyz) / 2.0f - scale);

		clearOctree();

		GLuint lscounter;
		GLuint lscounter_to;
		GLuint helper;
		GLuint helper_to;
		{
			glGenBuffers(1, &helper);
			glGenBuffers(1, &lscounter);
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, lscounter);
			glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, helper);
			glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_STORAGE_BIT);
		}

		GLuint size = 1;
		for (int i = 0;i < maxDepth;i++) {
			glUseProgram(0);
			glGenBuffers(1, &helper_to);
			glGenBuffers(1, &lscounter_to);

			{
				glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, lscounter_to);
				glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, helper_to);
				glBufferStorage(GL_SHADER_STORAGE_BUFFER, size * sizeof(GLuint), nullptr, GL_DYNAMIC_STORAGE_BIT);
			}

			{
				glUseProgram(voxelizerFixProgram);

				bindOctree();
				bind();

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

				bindOctree();
				bind();

				glUniform1ui(glGetUniformLocation(voxelizerProgram, "maxDepth"), maxDepth);
				glUniform1ui(glGetUniformLocation(voxelizerProgram, "currentDepth"), i);
				glUniform3fv(glGetUniformLocation(voxelizerProgram, "offset"), 1, glm::value_ptr(offset));
				glUniform3fv(glGetUniformLocation(voxelizerProgram, "scale"), 1, glm::value_ptr(scale));

				glViewport(0, 0, size_r, size_r);
				glDrawArrays(GL_TRIANGLES, 0, triangleCount * 3);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);

				GLuint tsz = 0;
				glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, vcounter);
				glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &tsz);

				GLuint vsz = 0;
				glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, scounter);
				glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &vsz);
			}

			glDeleteBuffers(1, &lscounter);
			glDeleteBuffers(1, &helper);
			lscounter = lscounter_to;
			helper = helper_to;
		}

		glDeleteBuffers(1, &lscounter);
		glDeleteBuffers(1, &helper);
	}
};


class TestMat {
private:
	GLuint matProgram;
	GLuint materialID = 0;
	GLuint tex;
	GLuint btex;
	GLuint stex;

	void init() {
		GLuint compShader = loadShader("./render/testmat.comp", GL_COMPUTE_SHADER);
		matProgram = glCreateProgram();
		glAttachShader(matProgram, compShader);
		glBindFragDataLocation(matProgram, 0, "outColor");
		glLinkProgram(matProgram);
		glUseProgram(matProgram);
	}
public:
	TestMat() {
		init();
	}
	void setMaterialID(GLuint id) {
		materialID = id;
	}
	void setTexture(GLuint tx) {
		tex = tx;
	}
	void setBump(GLuint tx) {
		btex = tx;
	}
	void setSpecular(GLuint tx) {
		stex = tx;
	}
	void shade(RObject &rays, GLfloat reflectionRate) {
		glUseProgram(matProgram);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);
		glUniform1i(glGetUniformLocation(matProgram, "tex"), 0);
		
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, btex);
		glUniform1i(glGetUniformLocation(matProgram, "bump"), 1);
		
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, stex);
		glUniform1i(glGetUniformLocation(matProgram, "spec"), 2);

		rays.bind();
		glUniform1ui(glGetUniformLocation(matProgram, "materialID"), materialID);
		glUniform1f(glGetUniformLocation(matProgram, "reflectionRate"), reflectionRate);
		glUniform2f(glGetUniformLocation(matProgram, "sceneRes"), width, height);

		glDispatchCompute(tiled(width, 16) / 16, tiled(height, 16) / 16, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
};



class Camera {
public:
	glm::vec3 eye = glm::vec3(1.0, 20.0, 1.0);
	glm::vec3 view = glm::vec3(0.0, 20.0, 0.0);

	sf::Vector2i mposition;

	glm::mat4 project() {
		return glm::lookAt(eye, view, glm::vec3(0.0, 1.0, 0.0));
	}

	void work(float diff) {
		glm::mat4 viewm = project();
		glm::mat4 unviewm = glm::inverse(viewm);
		glm::vec3 ca = glm::vec3(0.0f);
		glm::vec3 vi = (viewm * glm::vec4(view, 1.0)).xyz;

		sf::Vector2i position = sf::Mouse::getPosition();
		if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
		{
			sf::Vector2i mpos = position - mposition;
			float diffX = mpos.x * diff;
			float diffY = mpos.y * diff;
			this->rotateX(vi, diffX);
			this->rotateY(vi, diffY);
		} 

		mposition = position;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		{
			forwardBackward(ca, vi, diff);
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		{
			forwardBackward(ca, vi, -diff);
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		{
			leftRight(ca, vi, diff);
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		{
			leftRight(ca, vi, -diff);
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q) || sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
		{
			topBottom(ca, vi, diff);
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::E) || sf::Keyboard::isKeyPressed(sf::Keyboard::C))
		{
			topBottom(ca, vi, -diff);
		}

		eye = (unviewm * glm::vec4(ca, 1.0)).xyz;
		view = (unviewm * glm::vec4(vi, 1.0)).xyz;
	}

	void leftRight(glm::vec3 &ca, glm::vec3 &vi, float diff) {
		ca.x -= diff / 1000.0f;
		vi.x -= diff / 1000.0f;
	}
	void topBottom(glm::vec3 &ca, glm::vec3 &vi, float diff) {
		ca.y += diff / 1000.0f;
		vi.y += diff / 1000.0f;
	}
	void forwardBackward(glm::vec3 &ca, glm::vec3 &vi, float diff) {
		ca.z -= diff / 1000.0f;
		vi.z -= diff / 1000.0f;
	}
	void rotateY(glm::vec3 &vi, float diff) {
		vi = glm::rotateX(vi, -diff / 2000000.0f);
	}
	void rotateX(glm::vec3 &vi, float diff) {
		vi = glm::rotateY(vi, -diff / 2000000.0f);
	}
};

std::string cubefaces[6] = {
	"./cubemap/c0.jpg",
	"./cubemap/c1.jpg",
	"./cubemap/c2.jpg",
	"./cubemap/c3.jpg",
	"./cubemap/c4.jpg",
	"./cubemap/c5.jpg"
};

GLuint initCubeMap()
{
	GLbyte *pBytes;
	GLint eFormat, iComponents;
	GLint width, height;
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	for (int i = 0; i<6; i++)
	{
		sf::Image img_data;
		std::string tex = cubefaces[i];
		if (!img_data.loadFromFile(tex))
		{
			std::cout << "Could not load '" << tex << "'" << std::endl;
			//return false;
		}
		if (tex != "") {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, img_data.getSize().x, img_data.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data.getPixelsPtr());
		}
	}
	return tex;
}


int main()
{
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	sf::Window window(sf::VideoMode(width, height, 32), "OpenGL", sf::Style::Titlebar | sf::Style::Close, settings);

	glewExperimental = GL_TRUE;
	glewInit();

	GLuint cubeTex = initCubeMap();

	std::string inputfile = "sponza.obj";
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	bool ret = tinyobj::LoadObj(shapes, materials, err, inputfile.c_str());
	std::vector<TestMat> msponza(materials.size());
	for (int i = 0;i < msponza.size();i++) {
		sf::Image img_data;
		std::string tex = materials[i].diffuse_texname;

		if (tex != "") {
			if (!img_data.loadFromFile(tex))
			{
				std::cout << "Could not load '" << tex << "'" << std::endl;
			}
			GLuint texture_handle;
			glGenTextures(1, &texture_handle);
			glBindTexture(GL_TEXTURE_2D, texture_handle);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_data.getSize().x, img_data.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data.getPixelsPtr());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			msponza[i].setTexture(texture_handle);
		}


		tex = materials[i].bump_texname;
		if(tex != ""){
			if (!img_data.loadFromFile(tex))
			{
				std::cout << "Could not load '" << tex << "'" << std::endl;
			}
			GLuint texture_handle;
			glGenTextures(1, &texture_handle);
			glBindTexture(GL_TEXTURE_2D, texture_handle);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_data.getSize().x, img_data.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data.getPixelsPtr());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			msponza[i].setBump(texture_handle);
		}

		tex = materials[i].specular_texname;
		if (tex != "") {
			if (!img_data.loadFromFile(tex))
			{
				std::cout << "Could not load '" << tex << "'" << std::endl;
			}
			GLuint texture_handle;
			glGenTextures(1, &texture_handle);
			glBindTexture(GL_TEXTURE_2D, texture_handle);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_data.getSize().x, img_data.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data.getPixelsPtr());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			msponza[i].setSpecular(texture_handle);
		}
		msponza[i].setMaterialID(i);
	}

	TObject sponza;
	sponza.loadMesh(shapes);
	sponza.calcMinmax();
	sponza.buildOctree();

	RObject rays;
	rays.includeCubemap(cubeTex);
	clock_t t = clock();

	Camera cam;

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
		/*
		glm::vec3 eye = glm::vec3(1.0, 100.0, 1.0);
		glm::vec3 view = glm::vec3(0.0, 100.0, 0.0);
		//glm::vec3 eye = glm::vec3(2.0, 0.0, 2.0);
		//glm::vec3 view = glm::vec3(0.0, 0.0, 0.0);
		eye -= view;
		eye = rotate(eye, ((float)c) / 5000.0f, glm::vec3(0.0, 1.0, 0.0));
		eye += view;
		view = eye + normalize(view - eye);
		*/

		cam.work(c);

		//sponza.calcMinmax();
		//sponza.buildOctree();
		rays.camera(cam.eye, cam.view);
		for (int i = 0;i < 4;i++) {
			rays.begin();
			sponza.intersection(rays);
			for (int i = 0;i < msponza.size();i++) {
				msponza[i].shade(rays, 0.2);
			}
			rays.close();
		}

		rays.render();

		window.display();
	}

	window.close();

	return 0;
}
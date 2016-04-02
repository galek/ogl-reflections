// Link statically with GLEW
#define GLEW_STATIC
#define GLM_SWIZZLE
#define GLM_SWIZZLE_XYZW
#define _USE_MATH_DEFINES
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
#include <chrono>

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
	GLuint parent = 0xFFFFFFFF;
	GLuint last = 0xFFFFFFFF;
	GLuint count = 0;
	GLuint coordX = 0;
	GLuint coordY = 0;
	GLuint coordZ = 0;
	//glm::vec4 _space0;
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
const GLuint _zero = 0;

struct Ray {
	GLuint previous;
	GLuint hit;
	GLuint shadow;
	GLint actived;
	glm::vec4 origin;
	glm::vec4 direct;
	glm::vec4 color;
	glm::vec4 final;
	glm::vec4 params;
	glm::vec4 params0;
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

struct Texel {
	GLuint last;
	GLuint count;
};









struct VoxelRaw {
	GLuint coordX;
	GLuint coordY;
	GLuint coordZ;
	GLuint triangle;
};


class TObject {
public:
	TObject() { init(1024, 4); }
	TObject(GLuint count) {init(count, 4);}
	TObject(GLuint count, GLuint d) { init(count, d); }

private:
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
	GLuint voxelizerFillerProgram;
	GLuint voxelizerProgram;
	GLuint moverProgram;

	GLuint vrspace;
	GLuint vspace;
	GLuint tspace;
	GLuint subgrid;
	GLuint vcounter;
	GLuint scounter;
	GLuint dcounter;

	GLuint frameBuffer;
	GLuint tempBuffer;
	GLuint rboDepthStencil;

	Minmax bound;
	GLuint verticeCount = 0;
	

	GLuint intersectionProgram;
	

	

	GLuint vsize_g = sizeof(Voxel) * 256 * 256 * 256;
	GLuint vrsize_g = sizeof(VoxelRaw) * 512 * 512 * 32;
	GLuint tsize_g = sizeof(Thash) * 512 * 512 * 64;
	GLuint subgridc_g = sizeof(GLuint) * 256 * 256 * 256 * 8;

public:

	GLuint materialID = 0;
	glm::vec3 offset;
	glm::vec3 scale;
	GLuint maxDepth = 4;

	GLuint triangleCount = 0;

	void setDepth(GLuint count, GLuint d) {
		maxDepth = d;

		unsigned size_r = pow(2, maxDepth - 1);

		GLuint vrsize = sizeof(VoxelRaw) * count;
		GLuint vsize = sizeof(Voxel) * size_r * size_r * 64;
		GLuint tsize = sizeof(Thash) * count;
		GLuint subgridc = sizeof(GLuint) * size_r * size_r * 64 * 8;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vspace);
		glBufferPageCommitmentARB(GL_SHADER_STORAGE_BUFFER, 0, vsize_g, false);
		glBufferPageCommitmentARB(GL_SHADER_STORAGE_BUFFER, 0, vsize, true);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vrspace);
		glBufferPageCommitmentARB(GL_SHADER_STORAGE_BUFFER, 0, vrsize_g, false);
		glBufferPageCommitmentARB(GL_SHADER_STORAGE_BUFFER, 0, vrsize, true);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, tspace);
		glBufferPageCommitmentARB(GL_SHADER_STORAGE_BUFFER, 0, tsize_g, false);
		glBufferPageCommitmentARB(GL_SHADER_STORAGE_BUFFER, 0, tsize, true);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, subgrid);
		glBufferPageCommitmentARB(GL_SHADER_STORAGE_BUFFER, 0, subgridc_g, false);
		glBufferPageCommitmentARB(GL_SHADER_STORAGE_BUFFER, 0, subgridc, true);
	}

	void setMaterialID(GLuint id) {
		materialID = id;
	}

	void loadMesh(tinyobj::shape_t &shape) {
		std::vector<float> &vertices = shape.mesh.positions;
		std::vector<unsigned> &indices = shape.mesh.indices;
		std::vector<float> &normals = shape.mesh.normals;
		std::vector<float> &texcoords = shape.mesh.texcoords;
		std::vector<int> &material_ids = shape.mesh.material_ids;

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
		verticeCount = vertices.size();
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
		verticeCount = vertices.size();
	}

private:
	void init(GLuint count, GLuint depth) {
		glGenBuffers(1, &vspace);
		glGenBuffers(1, &vrspace);
		glGenBuffers(1, &tspace);
		glGenBuffers(1, &subgrid);
		glGenBuffers(1, &vcounter);
		glGenBuffers(1, &scounter);
		glGenBuffers(1, &dcounter);
		glGenBuffers(1, &vbo_triangle_ssbo);
		glGenBuffers(1, &ebo_triangle_ssbo);
		glGenBuffers(1, &norm_triangle_ssbo);
		glGenBuffers(1, &tex_triangle_ssbo);
		glGenBuffers(1, &mat_triangle_ssbo);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, tspace);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, tsize_g, nullptr, GL_SPARSE_STORAGE_BIT_ARB);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vspace);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, vsize_g, nullptr, GL_SPARSE_STORAGE_BIT_ARB);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vrspace);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, vrsize_g, nullptr, GL_SPARSE_STORAGE_BIT_ARB);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, subgrid);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, subgridc_g, nullptr, GL_SPARSE_STORAGE_BIT_ARB);

		setDepth(count, depth);

		{
			GLuint compShader = loadShader("./voxelizer/fix.comp", GL_COMPUTE_SHADER);
			voxelizerFixProgram = glCreateProgram();
			glAttachShader(voxelizerFixProgram, compShader);
			glBindFragDataLocation(voxelizerFixProgram, 0, "outColor");
			glLinkProgram(voxelizerFixProgram);
			glUseProgram(voxelizerFixProgram);
		}

		{
			GLuint compShader = loadShader("./voxelizer/filler.comp", GL_COMPUTE_SHADER);
			//GLuint compShader = loadShader("./voxelizer/voxelizer_filler_combined_2.comp", GL_COMPUTE_SHADER);
			voxelizerFillerProgram = glCreateProgram();
			glAttachShader(voxelizerFillerProgram, compShader);
			glBindFragDataLocation(voxelizerFillerProgram, 0, "outColor");
			glLinkProgram(voxelizerFillerProgram);
			glUseProgram(voxelizerFillerProgram);
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
			GLuint compShader = loadShader("./voxelizer/manipulator.comp", GL_COMPUTE_SHADER);
			moverProgram = glCreateProgram();
			glAttachShader(moverProgram, compShader);
			glBindFragDataLocation(moverProgram, 0, "outColor");
			glLinkProgram(moverProgram);
			glUseProgram(moverProgram);
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
	}

public:
	void move(glm::vec3 offset) {
		glUseProgram(moverProgram);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_triangle_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ebo_triangle_ssbo);
		glUniform1ui(glGetUniformLocation(moverProgram, "count"), verticeCount * 3);
		glUniform3fv(glGetUniformLocation(moverProgram, "offset"), 1, glm::value_ptr(offset));

		GLuint dsize = tiled(verticeCount, 1024);
		glDispatchCompute((dsize < 1024 ? 1024 : dsize) / 1024, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

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

			GLuint dsize = tiled(count, 1024);
			glDispatchCompute((dsize < 1024 ? 1024 : dsize) / 1024, 1, 1);
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
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, vrspace);
	}

	void clearOctree() {
		glUseProgram(0);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, vcounter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, scounter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, dcounter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);
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

		GLuint dsize = 0;
		{
			unsigned size_r = pow(2, maxDepth-1);

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
			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, dcounter);

			bindOctree();
			bind();

			glUniform1ui(glGetUniformLocation(voxelizerProgram, "maxDepth"), maxDepth);
			glUniform1ui(glGetUniformLocation(voxelizerProgram, "currentDepth"), maxDepth-1);
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

			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, dcounter);
			glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &dsize);
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

			GLuint lsiz = 0;
			{
				glUseProgram(voxelizerFixProgram);

				bindOctree();

				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, helper_to);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, helper);

				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, scounter);
				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, lscounter_to);
				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 3, lscounter);

				glUniform1ui(glGetUniformLocation(voxelizerFixProgram, "maxDepth"), maxDepth);
				glUniform1ui(glGetUniformLocation(voxelizerFixProgram, "currentDepth"), i);
				glUniform3fv(glGetUniformLocation(voxelizerFixProgram, "offset"), 1, glm::value_ptr(offset));
				glUniform3fv(glGetUniformLocation(voxelizerFixProgram, "scale"), 1, glm::value_ptr(scale));

				GLuint tsize = tiled(size, 1024);
				glDispatchCompute((tsize < 1024 ? 1024 : tsize) / 1024, 1, 1);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);

				GLuint osize = size;
				glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, lscounter_to);
				glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &size);
				lsiz = size;
				size *= 8;
			}

			{
				glUseProgram(voxelizerFillerProgram);

				bindOctree();
				bind();

				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, helper_to);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, helper);

				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, vcounter);
				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, dcounter);
				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, lscounter_to);
				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 3, lscounter);
				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 4, scounter);

				glUniform1ui(glGetUniformLocation(voxelizerFillerProgram, "triangleCount"), triangleCount);
				glUniform1ui(glGetUniformLocation(voxelizerFillerProgram, "maxDepth"), maxDepth);
				glUniform1ui(glGetUniformLocation(voxelizerFillerProgram, "currentDepth"), i);
				glUniform3fv(glGetUniformLocation(voxelizerFillerProgram, "offset"), 1, glm::value_ptr(offset));
				glUniform3fv(glGetUniformLocation(voxelizerFillerProgram, "scale"), 1, glm::value_ptr(scale));

				GLuint tsize = tiled(dsize, 1024);
				glDispatchCompute((tsize < 1024 ? 1024 : tsize) / 1024, 1, 1);
				//glDispatchCompute(triangleCount, 1, 1);
				//glDispatchCompute(lsiz, 1, 1);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
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




class RObject {
private:
	GLuint beginProgram;
	GLuint cameraProgram;
	GLuint renderProgram;
	GLuint closeProgram;
	GLuint clearProgram;
	GLuint samplerProgram;
	GLuint intersectionProgram;

	GLuint rays;
	GLuint hits;
	GLuint samples;
	GLuint texels;

	GLuint vbo;
	GLuint ebo;

	GLuint cubeTex;

	GLuint hcounter;
	GLuint rcounter;

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

		glGenBuffers(1, &texels);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, texels);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, rsize * sizeof(Texel), nullptr, GL_DYNAMIC_STORAGE_BIT);

		glGenBuffers(1, &rays);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, rays);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, 1024 * 1024 * 8 * sizeof(Ray), nullptr, GL_DYNAMIC_STORAGE_BIT);

		glGenBuffers(1, &hits);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, hits);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, 1024 * 1024 * 8 * sizeof(Hit), nullptr, GL_DYNAMIC_STORAGE_BIT);

		glGenBuffers(1, &rcounter);
		glGenBuffers(1, &hcounter);

		glGenBuffers(1, &samples);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, samples);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, rsize * sizeof(glm::vec4), nullptr, GL_DYNAMIC_STORAGE_BIT);

		{
			GLuint compShader = loadShader("./render/intersection.comp", GL_COMPUTE_SHADER);
			intersectionProgram = glCreateProgram();
			glAttachShader(intersectionProgram, compShader);
			glLinkProgram(intersectionProgram);
			glUseProgram(intersectionProgram);
		}

		{
			GLuint compShader = loadShader("./render/begin.comp", GL_COMPUTE_SHADER);
			beginProgram = glCreateProgram();
			glAttachShader(beginProgram, compShader);
			glLinkProgram(beginProgram);
			glUseProgram(beginProgram);
		}

		{
			GLuint compShader = loadShader("./render/close.comp", GL_COMPUTE_SHADER);
			closeProgram = glCreateProgram();
			glAttachShader(closeProgram, compShader);
			glLinkProgram(closeProgram);
			glUseProgram(closeProgram);
		}

		{
			GLuint compShader = loadShader("./render/camera.comp", GL_COMPUTE_SHADER);
			cameraProgram = glCreateProgram();
			glAttachShader(cameraProgram, compShader);
			glLinkProgram(cameraProgram);
			glUseProgram(cameraProgram);
		}

		{
			GLuint compShader = loadShader("./render/clear.comp", GL_COMPUTE_SHADER);
			clearProgram = glCreateProgram();
			glAttachShader(clearProgram, compShader);
			glLinkProgram(clearProgram);
			glUseProgram(clearProgram);
		}

		{
			GLuint compShader = loadShader("./render/sampler.comp", GL_COMPUTE_SHADER);
			samplerProgram = glCreateProgram();
			glAttachShader(samplerProgram, compShader);
			glLinkProgram(samplerProgram);
			glUseProgram(samplerProgram);
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

		GLuint _zero = 0;
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, rcounter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, hcounter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);

		bind();

		glUniformMatrix4fv(glGetUniformLocation(cameraProgram, "projInv"), 1, false, glm::value_ptr(glm::inverse(glm::perspective(3.14f / 3.0f, float(width) / float(height), 0.1f, 10000.0f))));
		glUniformMatrix4fv(glGetUniformLocation(cameraProgram, "camInv"), 1, false, glm::value_ptr(glm::inverse(glm::lookAt(eye, view, glm::vec3(0.0, 1.0, 0.0)))));
		glUniform2f(glGetUniformLocation(cameraProgram, "sceneRes"), width, height);
		glUniform1f(glGetUniformLocation(cameraProgram, "time"), clock());

		glDispatchCompute(tiled(width, 32) / 32, tiled(height, 32) / 32, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void begin() {
		glUseProgram(beginProgram);
		bind();

		GLuint rsize = getRayCount();
		glUniform1ui(glGetUniformLocation(beginProgram, "rayCount"), rsize);
		glDispatchCompute(tiled(rsize, 1024) / 1024, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void sample() {
		glUseProgram(samplerProgram);
		bind();

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, samples);
		glUniform2f(glGetUniformLocation(samplerProgram, "sceneRes"), width, height);
		glDispatchCompute(tiled(width, 32) / 32, tiled(height, 32) / 32, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void clear() {
		glUseProgram(clearProgram);
		bind();

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, samples);
		glUniform2f(glGetUniformLocation(clearProgram, "sceneRes"), width, height);
		glDispatchCompute(tiled(width, 32) / 32, tiled(height, 32) / 32, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void close() {
		glUseProgram(closeProgram);
		bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cubeTex);
		glUniform1i(glGetUniformLocation(closeProgram, "cubeTex"), 0);

		GLuint rsize = getRayCount();
		glUniform1ui(glGetUniformLocation(closeProgram, "rayCount"), rsize);
		glDispatchCompute(tiled(rsize, 1024) / 1024, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void bind() {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, rays);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, hits);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, texels);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, rcounter);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, hcounter);
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

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, samples);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glUniform2f(glGetUniformLocation(renderProgram, "sceneRes"), width, height);
		glVertexAttribPointer(glGetAttribLocation(renderProgram, "position"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glViewport(0, 0, width, height);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void intersection(TObject &obj, glm::mat4 trans) {
		glUseProgram(intersectionProgram);

		obj.bindOctree();
		obj.bind();

		bind();

		glUniform2f(glGetUniformLocation(intersectionProgram, "sceneRes"), width, height);
		glUniform1ui(glGetUniformLocation(intersectionProgram, "maxDepth"), obj.maxDepth);
		glUniform3fv(glGetUniformLocation(intersectionProgram, "offset"), 1, glm::value_ptr(obj.offset));
		glUniform3fv(glGetUniformLocation(intersectionProgram, "scale"), 1, glm::value_ptr(obj.scale));
		glUniform1ui(glGetUniformLocation(intersectionProgram, "materialID"), obj.materialID);

		glUniformMatrix4fv(glGetUniformLocation(intersectionProgram, "transform"), 1, false, glm::value_ptr(trans));
		glUniformMatrix4fv(glGetUniformLocation(intersectionProgram, "transformInv"), 1, false, glm::value_ptr(glm::inverse(trans)));

		//glDispatchCompute(tiled(width, 32) / 32, tiled(height, 32) / 32, 1);

		GLuint rsize = getRayCount();
		glUniform1ui(glGetUniformLocation(intersectionProgram, "rayCount"), rsize);
		glDispatchCompute(tiled(rsize, 1024) / 1024, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	GLuint getRayCount() {
		GLuint rsize = 0;
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, rcounter);
		glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &rsize);
		return rsize;
	}
};


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


class TestMat {
private:
	GLuint matProgram;
	GLuint materialID = 0;
	GLuint tex;
	GLuint btex;
	GLuint stex;
	GLuint itex;
	GLfloat illumPow;
	GLfloat reflectivity;
	GLfloat dissolve;
	GLfloat transmission;

	void init() {
		GLuint compShader = loadShader("./render/testmat.comp", GL_COMPUTE_SHADER);
		matProgram = glCreateProgram();
		glAttachShader(matProgram, compShader);
		glBindFragDataLocation(matProgram, 0, "outColor");
		glLinkProgram(matProgram);
		glUseProgram(matProgram);

		itex = loadWithDefault("", glm::vec4(glm::vec3(1.0f), 0.0f));
		illumPow = 1.0f;
		reflectivity = 0.0f;
		dissolve = 0.0f;
		transmission = 1.0f;
	}
public:
	TestMat() {
		init();
	}
	void setTransmission(GLfloat a) {
		transmission = a;
	}
	void setDissolve(GLfloat a) {
		dissolve = a;
	}
	void setReflectivity(GLfloat a) {
		reflectivity = a;
	}
	void setIllumPower(GLfloat a) {
		illumPow = a;
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
	void setIllumination(GLuint tx) {
		itex = tx;
	}
	void shade(RObject &rays) {
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

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, itex);
		glUniform1i(glGetUniformLocation(matProgram, "illum"), 3);

		rays.bind();
		glUniform1ui(glGetUniformLocation(matProgram, "materialID"), materialID);
		glUniform3fv(glGetUniformLocation(matProgram, "light"), 1, glm::value_ptr(glm::vec3(9.0f, 200.0f, 9.0f)));
		glUniform1f(glGetUniformLocation(matProgram, "time"), clock());
		glUniform1f(glGetUniformLocation(matProgram, "illumPower"), illumPow);
		glUniform1f(glGetUniformLocation(matProgram, "reflectivity"), reflectivity);
		glUniform1f(glGetUniformLocation(matProgram, "dissolve"), dissolve);
		glUniform1f(glGetUniformLocation(matProgram, "transmission"), transmission);

		GLuint rsize = rays.getRayCount();
		glUniform1ui(glGetUniformLocation(matProgram, "rayCount"), rsize);
		glDispatchCompute(tiled(rsize, 1024) / 1024, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
};

class Camera {
public:
	glm::vec3 eye = glm::vec3(300.0, 100.0, 0.0);
	glm::vec3 view = glm::vec3(0.0, 100.0, 0.0);

	sf::Vector2i mposition;

	RObject * raysp;

	glm::mat4 project() {
		return glm::lookAt(eye, view, glm::vec3(0.0, 1.0, 0.0));
	}

	void setRays(RObject &rays) {
		raysp = &rays;
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
			raysp->clear();
		}

		mposition = position;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		{
			forwardBackward(ca, vi, diff);
			raysp->clear();
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		{
			forwardBackward(ca, vi, -diff);
			raysp->clear();
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		{
			leftRight(ca, vi, diff);
			raysp->clear();
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		{
			leftRight(ca, vi, -diff);
			raysp->clear();
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q) || sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
		{
			topBottom(ca, vi, diff);
			raysp->clear();
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::E) || sf::Keyboard::isKeyPressed(sf::Keyboard::C))
		{
			topBottom(ca, vi, -diff);
			raysp->clear();
		}

		eye = (unviewm * glm::vec4(ca, 1.0)).xyz;
		view = (unviewm * glm::vec4(vi, 1.0)).xyz;
	}

	void leftRight(glm::vec3 &ca, glm::vec3 &vi, float diff) {
		ca.x -= diff / 10.0f;
		vi.x -= diff / 10.0f;
	}
	void topBottom(glm::vec3 &ca, glm::vec3 &vi, float diff) {
		ca.y += diff / 10.0f;
		vi.y += diff / 10.0f;
	}
	void forwardBackward(glm::vec3 &ca, glm::vec3 &vi, float diff) {
		ca.z -= diff / 10.0f;
		vi.z -= diff / 10.0f;
	}
	void rotateY(glm::vec3 &vi, float diff) {
		vi = glm::rotateX(vi, -diff / float(height) / 10.0f);
	}
	void rotateX(glm::vec3 &vi, float diff) {
		vi = glm::rotateY(vi, -diff / float(height) / 10.0f);
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

double milliseconds() {
	auto duration = std::chrono::high_resolution_clock::now();
	double millis = std::chrono::duration_cast<std::chrono::nanoseconds>(duration.time_since_epoch()).count();
	return millis / 1000000.0;
}

int main()
{
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	sf::Window window(sf::VideoMode(width, height, 32), "OpenGL", sf::Style::Titlebar | sf::Style::Close, settings);

	glewExperimental = GL_TRUE;
	glewInit();

	std::vector<TObject> sponza(1);
	std::vector<TestMat> msponza;
	{
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;
		bool ret = tinyobj::LoadObj(shapes, materials, err, "sponza.obj");
		
		sponza[0].setDepth(128 * 256 * 256, 9);
		sponza[0].setMaterialID(0);
		sponza[0].loadMesh(shapes);
		sponza[0].move(glm::vec3(0.0f, 0.0f, 0.0f));
		sponza[0].calcMinmax();
		sponza[0].buildOctree();
		
		msponza.resize(materials.size());
		for (int i = 0;i < msponza.size();i++) {
			msponza[i].setTransmission(materials[i].transmittance[0]);
			msponza[i].setReflectivity(materials[i].shininess);
			msponza[i].setDissolve(materials[i].dissolve);
			msponza[i].setBump(loadBump(materials[i].bump_texname));
			//mteapot[i].setSpecular(loadWithDefault(materials[i].specular_texname, glm::vec4(glm::vec3(0.0f), 1.0f)));
			//mteapot[i].setTexture(loadWithDefault(materials[i].diffuse_texname, glm::vec4(glm::vec3(1.0f), 1.0f)));
			msponza[i].setSpecular(loadWithDefault(materials[i].specular_texname, glm::vec4(glm::vec3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]), 1.0f)));
			msponza[i].setTexture(loadWithDefault(materials[i].diffuse_texname, glm::vec4(glm::vec3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]), 1.0f)));
			msponza[i].setMaterialID(0 + i);
		}
	}

	std::vector<TObject> teapot(1);
	std::vector<TestMat> mteapot;
	{
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;
		bool ret = tinyobj::LoadObj(shapes, materials, err, "material-test.obj");

		teapot[0].setDepth(128 * 64 * 64, 8);
		teapot[0].setMaterialID(msponza.size());
		teapot[0].loadMesh(shapes);
		teapot[0].move(glm::vec3(0.0f, 0.0f, 0.0f));
		teapot[0].calcMinmax();
		teapot[0].buildOctree();

		mteapot.resize(materials.size());
		for (int i = 0;i < mteapot.size();i++) {
			//mteapot[i].setIllumination(loadWithDefault("", glm::vec4(glm::vec3(1.0f, 1.0f, 1.0f), 1.0f)));
			//mteapot[i].setIllumPower(40.0f);
			mteapot[i].setTransmission(materials[i].transmittance[0]);
			mteapot[i].setReflectivity(materials[i].shininess);
			mteapot[i].setDissolve(-materials[i].dissolve);
			mteapot[i].setBump(loadBump(materials[i].bump_texname));
			//mteapot[i].setSpecular(loadWithDefault(materials[i].specular_texname, glm::vec4(glm::vec3(0.0f), 1.0f)));
			//mteapot[i].setTexture(loadWithDefault(materials[i].diffuse_texname, glm::vec4(glm::vec3(1.0f), 1.0f)));
			mteapot[i].setSpecular(loadWithDefault(materials[i].specular_texname, glm::vec4(glm::vec3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]), 1.0f)));
			mteapot[i].setTexture(loadWithDefault(materials[i].diffuse_texname, glm::vec4(glm::vec3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]), 1.00f)));
			mteapot[i].setMaterialID(msponza.size() + i);
		}
	}


	RObject rays;
	rays.includeCubemap(initCubeMap());
	double t = milliseconds();//time(0) * 1000.0;

	Camera cam;
	cam.setRays(rays);

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

		double tt = milliseconds();
		double c = tt - t;
		t = tt;

		//teapot[0].calcMinmax();
		//teapot[0].buildOctree();
		//sponza[0].calcMinmax();
		//sponza[0].buildOctree();

		cam.work(c);
		rays.camera(cam.eye, cam.view);
		for (int j = 0;j < 4;j++) {
			rays.begin();
			glm::mat4 trans;

			//trans = glm::translate(trans, glm::vec3(0.0f, 2000.0f, 300.0f));
			//trans = glm::scale(trans, glm::vec3(250.0f, 250.0f, 250.0f));
			//trans = glm::rotate(trans, 3.14f / 2.0f, glm::vec3(-1.0f, 0.0f, 0.0f));


			for (int i = 0;i < sponza.size();i++) {
				rays.intersection(sponza[i], glm::mat4());
				//rays.intersection(sponza[i], trans);
			}
			/*for (int i = 0;i < teapot.size();i++) {
				rays.intersection(teapot[i], trans);
			}*/

			for (int i = 0;i < msponza.size();i++) {
				msponza[i].shade(rays);
			}
			/*for (int i = 0;i < mteapot.size();i++) {
				mteapot[i].shade(rays);
			}*/
			rays.close();
		}

		rays.sample();
		rays.render();

		window.display();
	}

	window.close();

	return 0;
}
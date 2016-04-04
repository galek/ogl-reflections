#pragma once

#include "includes.hpp"
#include "utils.hpp"

struct VoxelRaw {
	GLuint coordX;
	GLuint coordY;
	GLuint coordZ;
	GLuint triangle;
};

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
};

struct Minmax {
	glm::vec4 mn;
	glm::vec4 mx;
};


class TObject {
public:
	TObject() { init(1024, 4); }
	TObject(GLuint count) { init(count, 4); }
	TObject(GLuint count, GLuint d) { init(count, d); }

private:
	GLuint ebo_triangle_ssbo;
	GLuint vbo_triangle_ssbo;
	GLuint norm_triangle_ssbo;
	GLuint tex_triangle_ssbo;
	GLuint mat_triangle_ssbo;

	GLuint voxelizerFixProgram;
	GLuint voxelizerMinmaxProgram;
	GLuint voxelizerFillerProgram;
	GLuint voxelizerProgram;
	GLuint intersectionProgram;

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

	GLuint vsize_g = sizeof(Voxel) * 256 * 256 * 256;
	GLuint vrsize_g = sizeof(VoxelRaw) * 512 * 512 * 32;
	GLuint tsize_g = sizeof(Thash) * 512 * 512 * 64;
	GLuint subgridc_g = sizeof(GLuint) * 256 * 256 * 256 * 8;

private:
	void initShaders() {
		includeShader("./voxelizer/include/octree.glsl", "/octree");
		includeShader("./voxelizer/include/octree_raw.glsl", "/octree_raw");
		includeShader("./voxelizer/include/octree_helper.glsl", "/octree_helper");
		includeShader("./voxelizer/include/vertex_attributes.glsl", "/vertex_attributes");
		includeShader("./render/include/constants.glsl", "/constants");
		includeShader("./render/include/structs.glsl", "/structs");
		includeShader("./render/include/uniforms.glsl", "/uniforms");
		includeShader("./render/include/fastmath.glsl", "/fastmath");
		includeShader("./render/include/random.glsl", "/random");

		{
			GLuint compShader = loadShader("./voxelizer/fix.comp", GL_COMPUTE_SHADER);
			voxelizerFixProgram = glCreateProgram();
			glAttachShader(voxelizerFixProgram, compShader);
			glLinkProgram(voxelizerFixProgram);
			glUseProgram(voxelizerFixProgram);
		}

		{
			GLuint compShader = loadShader("./voxelizer/filler.comp", GL_COMPUTE_SHADER);
			voxelizerFillerProgram = glCreateProgram();
			glAttachShader(voxelizerFillerProgram, compShader);
			glLinkProgram(voxelizerFillerProgram);
			glUseProgram(voxelizerFillerProgram);
		}

		{
			GLuint compShader = loadShader("./voxelizer/minmax.comp", GL_COMPUTE_SHADER);
			voxelizerMinmaxProgram = glCreateProgram();
			glAttachShader(voxelizerMinmaxProgram, compShader);
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
			glEnableVertexAttribArray(glGetAttribLocation(voxelizerProgram, "position"));
		}
	}

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

		initShaders();
		setDepth(count, depth);

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
	glm::vec3 offset;
	glm::vec3 scale;
	GLuint materialID = 0;
	GLuint maxDepth = 4;
	GLuint triangleCount = 0;

	void setMaterialID(GLuint id) {
		materialID = id;
	}

	void bind() {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, vbo_triangle_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ebo_triangle_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, norm_triangle_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, tex_triangle_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, mat_triangle_ssbo);
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
			unsigned size_r = pow(2, maxDepth - 1);

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
			glUniform1ui(glGetUniformLocation(voxelizerProgram, "currentDepth"), maxDepth - 1);
			glUniform3fv(glGetUniformLocation(voxelizerProgram, "offset"), 1, glm::value_ptr(offset));
			glUniform3fv(glGetUniformLocation(voxelizerProgram, "scale"), 1, glm::value_ptr(scale));

			glViewport(0, 0, size_r, size_r);
			glDrawArrays(GL_TRIANGLES, 0, triangleCount * 3);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			GLuint tsz = 0;
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, vcounter);
			glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &tsz);

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

				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, scounter);
				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, lscounter_to);
				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 3, lscounter);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, helper_to);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, helper);

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

				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, dcounter);
				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, vcounter);

				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, lscounter_to);
				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 3, lscounter);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, helper_to);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, helper);

				glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 4, scounter);

				glUniform1ui(glGetUniformLocation(voxelizerFillerProgram, "triangleCount"), triangleCount);
				glUniform1ui(glGetUniformLocation(voxelizerFillerProgram, "maxDepth"), maxDepth);
				glUniform1ui(glGetUniformLocation(voxelizerFillerProgram, "currentDepth"), i);
				glUniform3fv(glGetUniformLocation(voxelizerFillerProgram, "offset"), 1, glm::value_ptr(offset));
				glUniform3fv(glGetUniformLocation(voxelizerFillerProgram, "scale"), 1, glm::value_ptr(scale));

				GLuint tsize = tiled(dsize, 1024);
				glDispatchCompute((tsize < 1024 ? 1024 : tsize) / 1024, 1, 1);
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
};
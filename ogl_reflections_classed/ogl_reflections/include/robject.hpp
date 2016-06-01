#pragma once

#include "includes.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include "tobject.hpp"

struct Ray {
	glm::uint texel;
	glm::uint previous;
	glm::uint hit;
	glm::uint shadow;
	int actived;
	glm::vec4 origin;
	glm::vec4 direct;
	glm::vec4 color;
	glm::vec4 final;
	glm::vec4 params;
	glm::vec4 params0;
};

struct Hit {
	float dist;
	glm::uint triangle;
	glm::uint materialID;
	glm::vec4 normal;
	glm::vec4 tangent;
	glm::vec4 texcoord;
	glm::vec4 params;
};

struct Texel {
	glm::uint last;
	glm::uint count;
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
	GLuint fastIntersectionProgram;

	GLuint preloadProgram;
	GLuint postloadProgram;

	GLuint rays;
	GLuint hits;
	GLuint samples;
	GLuint texels;
	GLuint hcounter;
	GLuint rcounter;

	GLuint vbo;
	GLuint ebo;

	GLuint cubeTex;
	
	GLuint frameBuffer;
	GLuint tempBuffer;
	GLuint rboBuffer;

	std::vector<GLuint> finalHit;
	GLuint finalDepth;

public:
	RObject() {
		init();
	}

private:

	void initShaders() {
		includeShader("./voxelizer/include/octree.glsl", "/octree");
		includeShader("./voxelizer/include/octree_raw.glsl", "/octree_raw");
		includeShader("./voxelizer/include/vertex_attributes.glsl", "/vertex_attributes");
		includeShader("./render/include/constants.glsl", "/constants");
		includeShader("./render/include/structs.glsl", "/structs");
		includeShader("./render/include/uniforms.glsl", "/uniforms");
		includeShader("./render/include/fastmath.glsl", "/fastmath");
		includeShader("./render/include/random.glsl", "/random");
		includeShader("./render/include/fimages.glsl", "/fimages");

		{
			GLuint compShader = loadShader("./render/intersection.cs.glsl", GL_COMPUTE_SHADER);
			intersectionProgram = glCreateProgram();
			glAttachShader(intersectionProgram, compShader);
			glLinkProgram(intersectionProgram);
			glUseProgram(intersectionProgram);
		}

		{
			GLuint compShader = loadShader("./render/begin.cs.glsl", GL_COMPUTE_SHADER);
			beginProgram = glCreateProgram();
			glAttachShader(beginProgram, compShader);
			glLinkProgram(beginProgram);
			glUseProgram(beginProgram);
		}

		{
			GLuint compShader = loadShader("./render/close.cs.glsl", GL_COMPUTE_SHADER);
			closeProgram = glCreateProgram();
			glAttachShader(closeProgram, compShader);
			glLinkProgram(closeProgram);
			glUseProgram(closeProgram);
		}

		{
			GLuint compShader = loadShader("./render/postload.cs.glsl", GL_COMPUTE_SHADER);
			postloadProgram = glCreateProgram();
			glAttachShader(postloadProgram, compShader);
			glLinkProgram(postloadProgram);
			glUseProgram(postloadProgram);
		}

		{
			GLuint compShader = loadShader("./render/preload.cs.glsl", GL_COMPUTE_SHADER);
			preloadProgram = glCreateProgram();
			glAttachShader(preloadProgram, compShader);
			glLinkProgram(preloadProgram);
			glUseProgram(preloadProgram);
		}

		{
			GLuint compShader = loadShader("./render/camera.cs.glsl", GL_COMPUTE_SHADER);
			cameraProgram = glCreateProgram();
			glAttachShader(cameraProgram, compShader);
			glLinkProgram(cameraProgram);
			glUseProgram(cameraProgram);
		}

		{
			GLuint compShader = loadShader("./render/clear.cs.glsl", GL_COMPUTE_SHADER);
			clearProgram = glCreateProgram();
			glAttachShader(clearProgram, compShader);
			glLinkProgram(clearProgram);
			glUseProgram(clearProgram);
		}

		{
			GLuint compShader = loadShader("./render/sampler.cs.glsl", GL_COMPUTE_SHADER);
			samplerProgram = glCreateProgram();
			glAttachShader(samplerProgram, compShader);
			glLinkProgram(samplerProgram);
			glUseProgram(samplerProgram);
		}

		{
			GLuint vertexShader = loadShader("./render/render.vs.glsl", GL_VERTEX_SHADER);
			GLuint fragmentShader = loadShader("./render/render.fs.glsl", GL_FRAGMENT_SHADER);
			renderProgram = glCreateProgram();
			glAttachShader(renderProgram, vertexShader);
			glAttachShader(renderProgram, fragmentShader);
			glBindFragDataLocation(renderProgram, 0, "outColor");
			glLinkProgram(renderProgram);
			glUseProgram(renderProgram);
			glEnableVertexAttribArray(glGetAttribLocation(renderProgram, "position"));
		}

		{
			GLuint vertexShader = loadShader("./render/fast/intersection.vs.glsl", GL_VERTEX_SHADER);
			GLuint geometryShader = loadShader("./render/fast/intersection.gs.glsl", GL_GEOMETRY_SHADER);
			GLuint fragmentShader = loadShader("./render/fast/intersection.fs.glsl", GL_FRAGMENT_SHADER);

			fastIntersectionProgram = glCreateProgram();
			glAttachShader(fastIntersectionProgram, vertexShader);
			glAttachShader(fastIntersectionProgram, geometryShader);
			glAttachShader(fastIntersectionProgram, fragmentShader);
			glBindFragDataLocation(fastIntersectionProgram, 0, "outColor");
			glLinkProgram(fastIntersectionProgram);
			glUseProgram(fastIntersectionProgram);
			glEnableVertexAttribArray(glGetAttribLocation(fastIntersectionProgram, "position"));
			
			GLint isCompiled = GL_FALSE;
			glGetProgramiv(fastIntersectionProgram, GL_LINK_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetProgramiv(fastIntersectionProgram, GL_INFO_LOG_LENGTH, &maxLength);
				std::vector<GLchar> errorLog(maxLength);
				glGetProgramInfoLog(fastIntersectionProgram, maxLength, &maxLength, &errorLog[0]);
				errorLog.resize(maxLength);
				glDeleteShader(fastIntersectionProgram);
				std::string err(errorLog.begin(), errorLog.end());
				std::cout << err << std::endl;
				system("pause");
				exit(0);
			}
		}

		//Frame buffer
		glGenFramebuffers(1, &frameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

		//Render buffer
		glGenRenderbuffers(1, &rboBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, rboBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1024, 1024);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboBuffer);

		//Draw buffers
		GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5 };
		glDrawBuffers(6, drawBuffers);

		//Hit buffers
		finalHit.resize(6);
		for (int i = 0;i < 6;i++) {
			glGenTextures(1, &finalHit[i]);
			glBindTexture(GL_TEXTURE_2D, finalHit[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1024, 1024, 0, GL_RGBA, GL_FLOAT, NULL);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, finalHit[i], 0);
		}

		//Depth
		glGenTextures(1, &finalDepth);
		glBindTexture(GL_TEXTURE_2D, finalDepth);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, finalDepth, 0);
	}
	
	

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

		glGenBuffers(1, &texels);
		glGenBuffers(1, &rays);
		glGenBuffers(1, &hits);
		glGenBuffers(1, &rcounter);
		glGenBuffers(1, &hcounter);
		glGenBuffers(1, &samples);

		GLuint rsize = width * height * 2;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, texels);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, rsize * sizeof(Texel), nullptr, GL_DYNAMIC_STORAGE_BIT);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, rays);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, 1024 * 1024 * 8 * sizeof(Ray), nullptr, GL_DYNAMIC_STORAGE_BIT);
		
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, hits);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, 1024 * 1024 * 8 * sizeof(Hit), nullptr, GL_DYNAMIC_STORAGE_BIT);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, samples);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, rsize * sizeof(glm::vec4), nullptr, GL_DYNAMIC_STORAGE_BIT);

		initShaders();
	}

public:
	void includeCubemap(GLuint cube) {
		cubeTex = cube;
	}

	void clearRays() {
		GLuint _zero = 0;
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, rcounter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, hcounter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &_zero, GL_DYNAMIC_DRAW);
	}

	void camera(glm::vec3 eye, glm::vec3 view) {
		glUseProgram(cameraProgram);

		clearRays();
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

		GLuint rsize = getRayCount();
		glUniform1ui(glGetUniformLocation(samplerProgram, "rayCount"), rsize);
		glUniform2f(glGetUniformLocation(samplerProgram, "sceneRes"), width, height);
		glDispatchCompute(tiled(width, 32) / 32, tiled(height, 32) / 32, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void clear() {
		glUseProgram(clearProgram);
		bind();

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
	
	void bindImages() {
		for (int i = 0;i < 6;i++) {
			glBindImageTexture(0 + i, finalHit[i], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		}
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, finalDepth);
		glUniform1i(4, 4);
		//glBindImageTexture(4, finalDepth, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	}

	void bind() {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, rays);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, hits);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, texels);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, samples);
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

		bind();
		glUniform2f(glGetUniformLocation(renderProgram, "sceneRes"), width, height);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
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

		glUniform2f(glGetUniformLocation(intersectionProgram, "sceneRes"), 1024, 1024);
		glUniform1ui(glGetUniformLocation(intersectionProgram, "maxDepth"), obj.maxDepth);
		glUniform3fv(glGetUniformLocation(intersectionProgram, "offset"), 1, glm::value_ptr(obj.offset));
		glUniform3fv(glGetUniformLocation(intersectionProgram, "scale"), 1, glm::value_ptr(obj.scale));
		glUniform1ui(glGetUniformLocation(intersectionProgram, "materialID"), obj.materialID);

		glUniformMatrix4fv(glGetUniformLocation(intersectionProgram, "transform"), 1, false, glm::value_ptr(trans));
		glUniformMatrix4fv(glGetUniformLocation(intersectionProgram, "transformInv"), 1, false, glm::value_ptr(glm::inverse(trans)));

		GLuint rsize = getRayCount();
		glUniform1ui(glGetUniformLocation(intersectionProgram, "rayCount"), rsize);
		glDispatchCompute(tiled(rsize, 1024) / 1024, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void fastIntersection(TObject &obj, glm::mat4 trans) {
		glUseProgram(preloadProgram);
		bind();
		bindImages();
		GLuint rsize = getRayCount();
		glUniform1ui(glGetUniformLocation(preloadProgram, "rayCount"), rsize);
		glDispatchCompute(tiled(rsize, 1024) / 1024, 1, 1);

		glUseProgram(fastIntersectionProgram);
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClearDepth(1.0f);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDisable(GL_BLEND);
		glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glCullFace(GL_FRONT_AND_BACK);

		bindImages();
		bind();
		obj.bind();

		glUniform2f(glGetUniformLocation(fastIntersectionProgram, "sceneRes"), width, height);
		glUniform2f(glGetUniformLocation(fastIntersectionProgram, "resolution"), 1024.0f, 1024.0f);
		glUniform1ui(glGetUniformLocation(fastIntersectionProgram, "materialID"), obj.materialID);
		glUniformMatrix4fv(glGetUniformLocation(fastIntersectionProgram, "transform"), 1, false, glm::value_ptr(trans));
		glUniformMatrix4fv(glGetUniformLocation(fastIntersectionProgram, "transformInv"), 1, false, glm::value_ptr(glm::inverse(trans)));
		glUniform1ui(glGetUniformLocation(fastIntersectionProgram, "rayCount"), rsize);
		glViewport(0, 0, 1024, 1024);
		glDrawArrays(GL_TRIANGLES, 0, obj.triangleCount * 3);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		glUseProgram(postloadProgram);
		bind();
		bindImages();
		glUniform1ui(glGetUniformLocation(postloadProgram, "rayCount"), rsize);
		glDispatchCompute(tiled(rsize, 1024) / 1024, 1, 1);
		/*
		std::vector<float> pixels(1024 * 1024 * 4);
		glBindTexture(GL_TEXTURE_2D, finalHit[1]);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.data());

		int test = 0;
		int test2 = 0;
		test += test2;*/
	}

	GLuint getRayCount() {
		GLuint rsize = 0;
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, rcounter);
		glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &rsize);
		return rsize;
	}
};
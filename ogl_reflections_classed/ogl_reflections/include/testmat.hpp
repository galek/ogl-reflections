#pragma once

#include "includes.hpp"
#include "constants.hpp"
#include "robject.hpp"

class TestMat {
private:
	GLuint matProgram;
	GLuint materialID = 0;
	GLuint tex;
	GLuint btex;
	GLuint stex;
	GLuint itex;
	GLuint ttex;

	GLfloat illumPow;
	GLfloat reflectivity;
	GLfloat dissolve;
	GLfloat ior;

	void init() {
		includeShader("./voxelizer/include/octree.glsl", "/octree");
		includeShader("./voxelizer/include/octree_raw.glsl", "/octree_raw");
		includeShader("./voxelizer/include/vertex_attributes.glsl", "/vertex_attributes");
		includeShader("./render/include/constants.glsl", "/constants");
		includeShader("./render/include/structs.glsl", "/structs");
		includeShader("./render/include/uniforms.glsl", "/uniforms");
		includeShader("./render/include/fastmath.glsl", "/fastmath");
		includeShader("./render/include/random.glsl", "/random");

		GLuint compShader = loadShader("./render/testmat.cs.glsl", GL_COMPUTE_SHADER);
		matProgram = glCreateProgram();
		glAttachShader(matProgram, compShader);
		glBindFragDataLocation(matProgram, 0, "outColor");
		glLinkProgram(matProgram);
		glUseProgram(matProgram);

		itex = loadWithDefault("", glm::vec4(glm::vec3(1.0f), 0.0f));
		illumPow = 0.0f;
		reflectivity = 0.0f;
		dissolve = 0.0f;
		//transmission = 1.0f;
		ior = 1.0f;
	}
public:
	TestMat() {
		init();
	}
	void setTransmission(GLfloat a) {
		ttex = a;
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
	void setIOR(GLuint io) {
		ior = io;
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

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, ttex);
		glUniform1i(glGetUniformLocation(matProgram, "trans"), 4);

		rays.bind();
		glUniform1ui(glGetUniformLocation(matProgram, "materialID"), materialID);
		glUniform3fv(glGetUniformLocation(matProgram, "light"), 1, glm::value_ptr(glm::vec3(9.0f, 200.0f, 9.0f)));
		glUniform1f(glGetUniformLocation(matProgram, "time"), clock());
		glUniform1f(glGetUniformLocation(matProgram, "illumPower"), illumPow);
		glUniform1f(glGetUniformLocation(matProgram, "reflectivity"), reflectivity);
		glUniform1f(glGetUniformLocation(matProgram, "dissolve"), dissolve);
		glUniform1f(glGetUniformLocation(matProgram, "ior"), ior);

		GLuint rsize = rays.getRayCount();
		glUniform1ui(glGetUniformLocation(matProgram, "rayCount"), rsize);
		glDispatchCompute(tiled(rsize, 1024) / 1024, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
};
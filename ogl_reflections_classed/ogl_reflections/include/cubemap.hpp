#pragma once
#include "includes.hpp"

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
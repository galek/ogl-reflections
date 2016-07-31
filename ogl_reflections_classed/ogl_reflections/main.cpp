#include "include/includes.hpp"
#include "include/constants.hpp"
#include "include/utils.hpp"
#include "include/camera.hpp"
#include "include/robject.hpp"
#include "include/tobject.hpp"
#include "include/cubemap.hpp"
#include "include/testmat.hpp"

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

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	bool ret = tinyobj::LoadObj(shapes, materials, err, "chessboard.obj");

	sponza[0].setDepth(128 * 128 * 128, 10);
	sponza[0].setMaterialID(0);
	sponza[0].loadMesh(shapes);
	sponza[0].calcMinmax();
	sponza[0].buildOctree();

	msponza.resize(materials.size());
	for (int i = 0;i < materials.size();i++) {
		msponza[i].setIOR(materials[i].ior);
		msponza[i].setReflectivity(materials[i].shininess);
		msponza[i].setDissolve(materials[i].dissolve);
		msponza[i].setBump(loadBump(""));
		msponza[i].setIllumination(loadWithDefault(materials[i].ambient_texname, glm::vec4(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2], 1.0f)));
		msponza[i].setSpecular(loadWithDefault(materials[i].specular_texname, glm::vec4(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2], 1.0f)));
		msponza[i].setTexture(loadWithDefault(materials[i].diffuse_texname, glm::vec4(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2], 1.0f)));
		msponza[i].setTransmission(loadWithDefault("", glm::vec4(materials[i].transmittance[0], materials[i].transmittance[1], materials[i].transmittance[2], 1.0f)));
		msponza[i].setIllumPower(0.0f);
		msponza[i].setMaterialID(0+i);
	}


	RObject rays;
	rays.includeCubemap(initCubeMap());
	double t = milliseconds();

	Camera cam;
	cam.setRays(rays);

	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<> dist(0, 1);

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

		cam.work(c);
		rays.camera(cam.eye, cam.view);
		//sponza[0].calcMinmax();
		//sponza[0].buildOctree();

		glm::mat4 matrix = glm::mat4();
		matrix = glm::scale(matrix, glm::vec3(100.0f));
		matrix *= glm::rotate(float(M_PI) / 2.0f, glm::vec3(-1.0f, 0.0f, 0.0f));

		for (int j = 0;j < 3;j++) {
			rays.begin();
			for (int i = 0;i < /*objects.size()*/1;i++) {
				rays.intersection(sponza[0], matrix);
			}
			for (int i = 0;i < msponza.size();i++) {
				msponza[i].shade(rays);
			}
			rays.close();
		}

		rays.sample();
		rays.render();
		window.display();
	}

	window.close();

	return 0;
}
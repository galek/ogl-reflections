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
	{
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;
		bool ret = tinyobj::LoadObj(shapes, materials, err, "sponza.obj");
		
		sponza[0].setDepth(128 * 256 * 256, 9);
		sponza[0].setMaterialID(0);
		sponza[0].loadMesh(shapes);
		sponza[0].calcMinmax();
		sponza[0].buildOctree();
		
		msponza.resize(materials.size());
		for (int i = 0;i < msponza.size();i++) {
			msponza[i].setIOR(materials[i].ior);
			msponza[i].setReflectivity(materials[i].shininess);
			msponza[i].setDissolve(materials[i].dissolve);
			msponza[i].setBump(loadBump(materials[i].bump_texname));
			msponza[i].setSpecular(loadWithDefault(materials[i].specular_texname, glm::vec4(glm::vec3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]), 1.0f)));
			msponza[i].setTexture(loadWithDefault(materials[i].diffuse_texname, glm::vec4(glm::vec3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]), 1.0f)));
			msponza[i].setMaterialID(0 + i);
		}
	}



	std::vector<TObject> sphere(1);
	std::vector<TestMat> msphere;
	{
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;
		bool ret = tinyobj::LoadObj(shapes, materials, err, "models/sphere.obj");

		sphere[0].setDepth(128 * 128 * 128, 6);
		sphere[0].setMaterialID(msponza.size());
		sphere[0].loadMesh(shapes);
		sphere[0].calcMinmax();
		sphere[0].buildOctree();

		msphere.resize(materials.size());
		for (int i = 0;i < msphere.size();i++) {
			msphere[i].setIOR(materials[i].ior);
			msphere[i].setReflectivity(1.0f);
			msphere[i].setDissolve(1.0f);
			msphere[i].setBump(loadBump(""));
			msphere[i].setSpecular(loadWithDefault("", glm::vec4(glm::vec3(1.0f), 1.0f)));
			msphere[i].setTexture(loadWithDefault("", glm::vec4(glm::vec3(1.0f), 1.0f)));
			msphere[i].setIllumPower(1.0f);
			msphere[i].setIllumination(loadWithDefault("", glm::vec4(glm::vec3(1.0f) / pow(400.0f, 2.0f) * 1600000.0f, 1.0f)));
			msphere[i].setMaterialID(msponza.size() + i);
		}
	}



	RObject rays;
	rays.includeCubemap(initCubeMap());
	double t = milliseconds();

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

		cam.work(c);
		rays.camera(cam.eye, cam.view);
		for (int j = 0;j < 4;j++) {
			
			rays.begin();

			glm::mat4 trans;
			trans = translate(trans, glm::vec3(0.0f, 2000.0f, 300.0f));
			trans = scale(trans, glm::vec3(400.0f));
			for (int i = 0;i < sphere.size();i++) {
				rays.intersection(sphere[i], trans);
			}

			for (int i = 0;i < sponza.size();i++) {
				rays.intersection(sponza[i], glm::mat4());
			}
			for (int i = 0;i < msponza.size();i++) {
				msphere[i].shade(rays);
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
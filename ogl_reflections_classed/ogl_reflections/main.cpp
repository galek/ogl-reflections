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

	std::vector<TObject> cube(6);
	std::vector<TestMat> mcube(6);

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	bool ret = tinyobj::LoadObj(shapes, materials, err, "cube.obj");

	std::array<glm::vec3, 6> colors { {
		glm::vec3(59.0 / 255.0, 89.0 / 255.0, 106.0 / 255.0),
		glm::vec3(66.0 / 255.0, 118.0 / 255.0, 118.0 / 255.0),
		glm::vec3(63.0 / 255.0, 154.0 / 255.0, 130.0 / 255.0),
		glm::vec3(161.0 / 255.0, 205.0 / 255.0, 115.0 / 255.0),
		glm::vec3(236.0 / 255.0, 219.0 / 255.0, 96.0 / 255.0),
		glm::vec3(236.0 / 255.0, 96.0 / 255.0, 96.0 / 255.0)
	} };

	for (int i = 0;i < 6;i++) {
		cube[i].setDepth(64 * 64 * 64, 5);
		cube[i].setMaterialID(i);
		cube[i].loadMesh(shapes);
		cube[i].calcMinmax();
		cube[i].buildOctree();

		mcube[i].setIOR(1.5f);
		mcube[i].setReflectivity(cos(20.0f / 180.0f * 3.14f));
		mcube[i].setDissolve(1.0f);
		mcube[i].setBump(loadBump(""));
		mcube[i].setIllumination(loadWithDefault("", glm::vec4(glm::vec3(0.0f), 1.0f)));
		mcube[i].setSpecular(loadWithDefault("", glm::vec4(glm::vec3(1.0f), 1.0f)));
		mcube[i].setTexture(loadWithDefault("", glm::vec4(glm::vec3(colors[i]), 1.0f)));
		mcube[i].setTransmission(loadWithDefault("", glm::vec4(glm::vec3(1.0f), 1.0f)));
		mcube[i].setIllumPower(0.0f);
		mcube[i].setMaterialID(i);
	}

	std::vector<TObject> sphere(1);
	std::vector<TestMat> msphere;
	{
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;
		bool ret = tinyobj::LoadObj(shapes, materials, err, "models/sphere.obj");

		sphere[0].setDepth(64 * 64 * 64, 6);
		sphere[0].setMaterialID(mcube.size());
		sphere[0].loadMesh(shapes);
		sphere[0].calcMinmax();
		sphere[0].buildOctree();
		
		msphere.resize(materials.size());
		for (int i = 0;i < msphere.size();i++) {
			msphere[i].setIOR(1.0f);
			msphere[i].setReflectivity(1.0f);
			msphere[i].setDissolve(1.0f);
			msphere[i].setBump(loadBump(""));
			msphere[i].setIllumination(loadWithDefault("", glm::vec4(glm::vec3(10.0f), 1.0f)));
			msphere[i].setSpecular(loadWithDefault("", glm::vec4(glm::vec3(1.0f), 1.0f)));
			msphere[i].setTexture(loadWithDefault("", glm::vec4(glm::vec3(1.0f), 1.0f)));
			msphere[i].setTransmission(loadWithDefault("", glm::vec4(glm::vec3(1.0f), 1.0f)));
			msphere[i].setIllumPower(1.0f);

			msphere[i].setMaterialID(msphere.size() + i);
		}
	}



	RObject rays;
	rays.includeCubemap(initCubeMap());
	double t = milliseconds();

	Camera cam;
	cam.setRays(rays);

	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<> dist(0, 1);

	std::vector<int> objects;
	std::vector<glm::mat4> transf;
	for (int x = -2;x < 2;x++) {
		for (int z = -3;z < 3;z++) {
			objects.push_back((int)((float)dist(mt) * 5.0f));
			objects.push_back((int)((float)dist(mt) * 5.0f));

			float fx = (float)x;
			float fy = (float)dist(mt);
			float fz = (float)z;

			{
				glm::mat4 trans;
				trans = translate(trans, glm::vec3(fx * 10.0f, fy * 10.0f, fz * 10.0f));
				transf.push_back(trans);
			}
			{
				glm::mat4 trans;
				trans = translate(trans, glm::vec3(fx * 10.0f, (fy + 1.0f) * 10.0f, fz * 10.0f));
				transf.push_back(trans);
			}
		}
	}

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
			for (int i = 0;i < objects.size();i++) {
				rays.intersection(cube[objects[i]], transf[i]);
			}
			for (int i = 0;i < mcube.size();i++) {
				mcube[i].shade(rays);
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
#pragma once
#include "includes.hpp"
#include "robject.hpp"

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
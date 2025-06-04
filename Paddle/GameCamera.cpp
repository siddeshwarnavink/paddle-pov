#include "GameCamera.hpp"

namespace Paddle {
	GameCamera::GameCamera() : position(5.0f, 0.0f, 0.0f), target(0.0f, 0.0f, 0.0f) {}

	void GameCamera::MoveLeft(float amount) {
		glm::vec3 direction = glm::normalize(target - position);
		glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 0.0f, 1.0f)));
		position -= right * amount;
		target -= right * amount;
	}

	void GameCamera::MoveRight(float amount) {
		glm::vec3 direction = glm::normalize(target - position);
		glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 0.0f, 1.0f)));
		position += right * amount;
		target += right * amount;
	}

	glm::vec3 GameCamera::GetPosition() const {
		return position;
	}

	glm::vec3 GameCamera::GetTarget() const {
		return target;
	}
}

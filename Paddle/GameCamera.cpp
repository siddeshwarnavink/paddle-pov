#include "GameCamera.hpp"

namespace Paddle {
	const auto DEFAULT_POSITION = glm::vec3(6.0f, 0.0f, 0.0f);
	const auto DEFAULT_TARGET = glm::vec3(0.0f, 0.0f, 0.0f);

	GameCamera::GameCamera() {
		Reset();
	}

	void GameCamera::Reset() {
		position = DEFAULT_POSITION;
		target = DEFAULT_TARGET;
	}

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
}

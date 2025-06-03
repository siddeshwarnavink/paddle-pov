#include "GameCamera.hpp"

namespace Paddle {
	GameCamera::GameCamera() : position(0.0f, 0.0f, 0.0f) {}

	void GameCamera::MoveLeft(float amount) {
		position.x -= amount;
	}

	void GameCamera::MoveRight(float amount) {
		position.x += amount;
	}

	glm::vec3 GameCamera::GetPosition() const {
		return position;
	}
}

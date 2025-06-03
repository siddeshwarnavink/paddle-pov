#pragma once
#include <glm/glm.hpp>

namespace Paddle {
	class GameCamera {
	public:
		GameCamera();
		void MoveLeft(float amount);
		void MoveRight(float amount);
		glm::vec3 GetPosition() const;
	private:
		glm::vec3 position;
	};
}

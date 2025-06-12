#pragma once
#include <glm/glm.hpp>

namespace Paddle {
	struct CameraUbo {
		glm::mat4 view;
		glm::mat4 proj;
	};

	class GameCamera {
	public:
		GameCamera();
		void MoveLeft(float amount);
		void MoveRight(float amount);
		void SetPosition(const glm::vec3& pos) { position = pos; }
		glm::vec3 GetPosition() const;
		glm::vec3 GetTarget() const;
	private:
		glm::vec3 position;
		glm::vec3 target;
	};
}

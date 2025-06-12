#include "PlayerPaddle.hpp"
#include "VkDevice.hpp"
#include <cstring>

namespace Paddle {
	PlayerPaddle::PlayerPaddle(Vk::Device& device, float x, float y, float z)
		: GameEntity(device)  {
		verticesInstance = {
			// Front face
			{{-0.001f, -0.25f,  0.075f}, {0,0,0}, {0,0,1}, {0,0}},
			{{ 0.001f, -0.25f,  0.075f}, {0,0,0}, {0,0,1}, {1,0}},
			{{ 0.001f,  0.25f,  0.075f}, {0,0,0}, {0,0,1}, {1,1}},
			{{-0.001f,  0.25f,  0.075f}, {0,0,0}, {0,0,1}, {0,1}},
			// Back face
			{{-0.001f, -0.25f, -0.075f}, {0,0,0}, {0,0,-1}, {1,0}},
			{{ 0.001f, -0.25f, -0.075f}, {0,0,0}, {0,0,-1}, {0,0}},
			{{ 0.001f,  0.25f, -0.075f}, {0,0,0}, {0,0,-1}, {0,1}},
			{{-0.001f,  0.25f, -0.075f}, {0,0,0}, {0,0,-1}, {1,1}},
		};
		indicesInstance = {
			0, 1, 2, 2, 3, 0,  // Front face
			1, 5, 6, 6, 2, 1,  // Right face
			5, 4, 7, 7, 6, 5,  // Back face
			4, 0, 3, 3, 7, 4,  // Left face
			3, 2, 6, 6, 7, 3,  // Top face
			4, 5, 1, 1, 0, 4   // Bottom face
		};
		SetPosition(glm::vec3(x, y, z));
		InitialiseEntity();
	}

	bool PlayerPaddle::CheckCollision(GameEntity* other) {
		if (auto* boundedEntity = dynamic_cast<IBounded*>(other)) {
			auto* entity = dynamic_cast<GameEntity*>(other);
			glm::vec3 otherMin = entity->GetPosition() - boundedEntity->GetHalfExtents();
			glm::vec3 otherMax = entity->GetPosition() + boundedEntity->GetHalfExtents();
			glm::vec3 thisMin = this->GetPosition() - this->GetHalfExtents();
			glm::vec3 thisMax = this->GetPosition() + this->GetHalfExtents();

			return (thisMin.x <= otherMax.x && thisMax.x >= otherMin.x) &&
				(thisMin.y <= otherMax.y && thisMax.y >= otherMin.y) &&
				(thisMin.z <= otherMax.z && thisMax.z >= otherMin.z);
		}
		return false;
	}

	glm::vec3 PlayerPaddle::GetHalfExtents() const {
		return glm::vec3(0.25);
	}
}

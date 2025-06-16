#include "Bullet.hpp"

#include "VkDevice.hpp"
#include <cstring>

namespace Paddle {
	Bullet::Bullet(GameContext& context, float x, float y, float z)
		: GameEntity(context)  {
		verticesInstance = {
			// Front face
			{{-0.01f, -0.01f,  0.01f}, {0,0,1}, {0,0,1}, {0,0}},
			{{ 0.01f, -0.01f,  0.01f}, {0,0,1}, {0,0,1}, {1,0}},
			{{ 0.01f,  0.01f,  0.01f}, {0,0,1}, {0,0,1}, {1,1}},
			{{-0.01f,  0.01f,  0.01f}, {0,0,1}, {0,0,1}, {0,1}},
			// Back face
			{{-0.01f, -0.01f, -0.01f}, {0,0,1}, {0,0,-1}, {1,0}},
			{{ 0.01f, -0.01f, -0.01f}, {0,0,1}, {0,0,-1}, {0,0}},
			{{ 0.01f,  0.01f, -0.01f}, {0,0,1}, {0,0,-1}, {0,1}},
			{{-0.01f,  0.01f, -0.01f}, {0,0,1}, {0,0,-1}, {1,1}},
		};
		indicesInstance = {
			0, 1, 2, 2, 3, 0,  // Front face
			1, 5, 6, 6, 2, 1,  // Right face
			5, 4, 7, 7, 6, 5,  // Back face
			4, 0, 3, 3, 7, 4,  // Left face
			3, 2, 6, 6, 7, 3,  // Top face
			4, 5, 1, 1, 0, 4   // Bottom face
		};

		velocity = glm::vec3(0.25f, 0.0f, 0.0f);
		SetPosition(glm::vec3(x, y, z));

		InitialiseEntity();
	}

	bool Bullet::CheckCollision(GameEntity* other) {
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

	void Bullet::Update() {
		glm::vec3 pos = GetPosition();
		const float speedDelta = -0.5f;

		pos += velocity * speedDelta;
		SetPosition(pos);
	}

	glm::vec3 Bullet::GetHalfExtents() const {
		return glm::vec3(0.15f);
	}
}

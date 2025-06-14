#include "Wall.hpp"
#include "VkDevice.hpp"
#include <cstring>

namespace Paddle {
	Wall::Wall(GameContext& context, float x, float y, float z, glm::vec3 halfExtents)
		: GameEntity(context), halfExtents(halfExtents) {
		verticesInstance = {
			// Front face
			{{-10.0f, -0.1f,  1.0f}, {0,0,0}, {0,0,1}, {0,0}},
			{{ 10.0f, -0.1f,  1.0f}, {0,0,0}, {0,0,1}, {1,0}},
			{{ 10.0f,  0.1f,  1.0f}, {0,0,0}, {0,0,1}, {1,1}},
			{{-10.0f,  0.1f,  1.0f}, {0,0,0}, {0,0,1}, {0,1}},
			// Back face
			{{-10.0f, -0.1f, -1.0f}, {0,0,0}, {0,0,-1}, {1,0}},
			{{ 10.0f, -0.1f, -1.0f}, {0,0,0}, {0,0,-1}, {0,0}},
			{{ 10.0f,  0.1f, -1.0f}, {0,0,0}, {0,0,-1}, {0,1}},
			{{-10.0f,  0.1f, -1.0f}, {0,0,0}, {0,0,-1}, {1,1}},
		};
		indicesInstance = {
			0, 1, 2, 2, 3, 0,  // Front face
			1, 5, 6, 6, 2, 1,  // Right face
			5, 4, 7, 7, 6, 5,  // Back face
			4, 0, 3, 3, 7, 4,  // Left face
			3, 2, 6, 6, 7, 3,  // Top face
			4, 5, 1, 1, 0, 4   // Bottom face
		};
		initPosition = glm::vec3(x, y, z);
		SetPosition(initPosition);
		InitialiseEntity();
	}

	glm::vec3 Wall::GetHalfExtents() const {
		return halfExtents;
	}

	void Wall::Reset() {
		SetPosition(initPosition);
	}
}

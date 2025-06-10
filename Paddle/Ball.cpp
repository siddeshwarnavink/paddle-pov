#include "Ball.hpp"
#include "VkDevice.hpp"
#include <cstring>
#include <glm/gtc/constants.hpp>

namespace Paddle {
	const uint32_t slices = 64;
	const uint32_t stacks = 32;

	Ball::Ball(Vk::Device& device, float x, float y, float z)
		: GameEntity(device) {
		verticesInstance = GenerateVertices();
		indicesInstance = GenerateIndices();
		SetPosition(glm::vec3(x, y, z));
		SetVelocity(glm::vec3(-1.0f, 0.5f, 0.0f));
		InitialiseEntity();
	}

	void Ball::Update(UpdateArgs args) {
		const int score = args.i;
		glm::vec3 pos = GetPosition();

		float speedDelta = 0.02f;

		if (score >= 20) {
			speedDelta = 0.04f;
		}
		else if (score >= 50) {
			speedDelta = 0.06f;
		}
		else if (score >= 100) {
			speedDelta = 0.08f;
		}
		else if (score >= 200) {
			speedDelta = 0.1f;
		}

		pos += velocity * speedDelta;
		SetPosition(pos);
	}

	std::vector<Vertex> Ball::GenerateVertices() {
		//
		// Ref: https://en.wikipedia.org/wiki/Spherical_coordinate_system
		//

		std::vector<Vertex> vertices;
		for (int i = 0; i <= stacks; ++i) {
			float phi = glm::pi<float>() * i / stacks;
			for (int j = 0; j <= slices; ++j) {
				float theta = 2 * glm::pi<float>() * j / slices;

				float x = radius * sin(phi) * cos(theta);
				float y = radius * cos(phi);
				float z = radius * sin(phi) * sin(theta);

				glm::vec3 pos = glm::vec3(x, y, z);
				glm::vec3 color = glm::vec3(194.0f / 255.f, 64.0f / 255.0f, 62.0f / 255.0f); // #c2403e
				glm::vec3 normal = glm::normalize(pos);
				glm::vec2 uv = glm::vec2((float)j / slices, (float)i / stacks);

				vertices.push_back({ pos, color, normal, uv });
			}
		}
		return vertices;
	}

	std::vector<uint32_t> Ball::GenerateIndices() {
		std::vector<uint32_t> indices;
		for (int i = 0; i < stacks; ++i) {
			for (int j = 0; j < slices; ++j) {
				int first = i * (slices + 1) + j;
				int second = first + slices + 1;

				indices.push_back(first);
				indices.push_back(second);
				indices.push_back(first + 1);

				indices.push_back(second);
				indices.push_back(second + 1);
				indices.push_back(first + 1);
			}
		}
		return indices;
	}
}

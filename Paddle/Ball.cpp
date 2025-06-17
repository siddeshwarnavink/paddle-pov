#include "Ball.hpp"
#include "VkDevice.hpp"
#include "Bullet.hpp"

#include <glm/gtc/constants.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

#include <cstring>

namespace Paddle {
	const auto DEFAULT_POSITION = glm::vec3(3.0f, 0.0f, 0.0f);
	const auto DEFAULT_VELOCITY = glm::vec3(-1.0f, 0.5f, 0.0f);

	const uint32_t BALL_SLICES = 64;
	const uint32_t BALL_STACKS = 32;

	Ball::Ball(GameContext& context) : GameEntity(context) {
		verticesInstance = GenerateVertices();
		indicesInstance = GenerateIndices();
		Reset();
		InitialiseEntity();
	}

	void Ball::Reset() {
		SetPosition(DEFAULT_POSITION);
		SetVelocity(DEFAULT_VELOCITY);
	}

	bool CheckAABBSphereCollision(const glm::vec3& boxMin, const glm::vec3& boxMax, const glm::vec3& sphereCenter, float sphereRadius) {
		glm::vec3 closestPoint = glm::clamp(sphereCenter, boxMin, boxMax);
		float distanceSquared = glm::distance2(closestPoint, sphereCenter);
		return distanceSquared < (sphereRadius * sphereRadius);
	}

	bool Ball::CheckCollision(GameEntity* other) {
		if (auto* boundedEntity = dynamic_cast<IBounded*>(other)) {
			auto* entity = dynamic_cast<GameEntity*>(other);
			glm::vec3 blockMin = entity->GetPosition() - boundedEntity->GetHalfExtents();
			glm::vec3 blockMax = entity->GetPosition() + boundedEntity->GetHalfExtents();
			return CheckAABBSphereCollision(blockMin, blockMax, this->GetPosition(), this->GetRadius());
		}
		return false;
	}

	void Ball::OnCollision(GameEntity* other) {
                IBounded* boundedEntity = dynamic_cast<IBounded*>(other);
                Bullet* bulletEntity    = dynamic_cast<Bullet*>(other);

		if (boundedEntity && !bulletEntity) {
			auto* entity = dynamic_cast<GameEntity*>(other);
			glm::vec3 blockMin = entity->GetPosition() - boundedEntity->GetHalfExtents();
			glm::vec3 blockMax = entity->GetPosition() + boundedEntity->GetHalfExtents();
			glm::vec3 sphereCenter = this->GetPosition();
			float sphereRadius = this->GetRadius();
			glm::vec3 closestPoint = glm::clamp(sphereCenter, blockMin, blockMax);
			glm::vec3 normal = sphereCenter - closestPoint;

			// Ball center is inside the box, so pick the axis of minimum penetration
			if (glm::length2(normal) < 1e-6f) {
				glm::vec3 distances = glm::min(blockMax - sphereCenter, sphereCenter - blockMin);
				int axis = 0;
				if (distances.y < distances.x) axis = 1;
				if (distances.z < distances[axis]) axis = 2;
				normal = glm::vec3(0.0f);
				normal[axis] = (sphereCenter[axis] > (blockMin[axis] + blockMax[axis]) * 0.5f) ? 1.0f : -1.0f;
			} else {
				normal = glm::normalize(normal);
			}

			velocity = glm::reflect(velocity, normal);

			// Push ball out of the paddle to prevent sticking
			SetPosition(closestPoint + normal * (sphereRadius + 1e-3f));
		}
	}

	void Ball::Update() {
		glm::vec3 pos = GetPosition();

		float speedDelta = 0.02f;

		if (context.score >= 20) {
			speedDelta = 0.04f;
		}
		else if (context.score >= 50) {
			speedDelta = 0.06f;
		}
		else if (context.score >= 100) {
			speedDelta = 0.08f;
		}
		else if (context.score >= 200) {
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
		for (int i = 0; i <= BALL_STACKS; ++i) {
			float phi = glm::pi<float>() * i / BALL_STACKS;
			for (int j = 0; j <= BALL_SLICES; ++j) {
				float theta = 2 * glm::pi<float>() * j / BALL_SLICES;

				float x = radius * sin(phi) * cos(theta);
				float y = radius * cos(phi);
				float z = radius * sin(phi) * sin(theta);

				glm::vec3 pos = glm::vec3(x, y, z);
				glm::vec3 color = glm::vec3(194.0f / 255.f, 64.0f / 255.0f, 62.0f / 255.0f); // #c2403e
				glm::vec3 normal = glm::normalize(pos);
				glm::vec2 uv = glm::vec2((float)j / BALL_SLICES, (float)i / BALL_STACKS);

				vertices.push_back({ pos, color, normal, uv });
			}
		}
		return vertices;
	}

	std::vector<uint32_t> Ball::GenerateIndices() {
		std::vector<uint32_t> indices;
		for (int i = 0; i < BALL_STACKS; ++i) {
			for (int j = 0; j < BALL_SLICES; ++j) {
				int first = i * (BALL_SLICES + 1) + j;
				int second = first + BALL_SLICES + 1;

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

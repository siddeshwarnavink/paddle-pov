#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "VkDevice.hpp"
#include "GameEntity.hpp"

namespace Paddle {
	class Ball : public GameEntity {
	public:
		Ball(GameContext& context);

		Ball(const Ball&) = delete;
		Ball& operator=(const Ball&) = delete;

		void Reset();
		bool CheckCollision(GameEntity* other) override;
		void OnCollision(GameEntity* other);
		void Update() override;

		glm::vec3 GetVelocity() { return velocity; }
		void SetVelocity(glm::vec3 updatedVelocity) { velocity = updatedVelocity; }

		float GetRadius() { return radius; }

	private:
		float radius = 0.25;
		glm::vec3 velocity;

		std::vector<Vertex> GenerateVertices();
		std::vector<uint32_t> GenerateIndices();
	};
}

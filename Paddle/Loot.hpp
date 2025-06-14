#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "GameContext.hpp"
#include "GameEntity.hpp"

namespace Paddle {
	class Loot : public GameEntity, public IBounded {
	public:
		Loot(GameContext& context, float x, float y, float z);

		Loot(const Loot&) = delete;
		Loot& operator=(const Loot&) = delete;

		glm::vec3 GetHalfExtents() const override;
		bool CheckCollision(GameEntity* other) override;
		void Update() override;

		glm::vec3 GetVelocity() { return velocity; }
		void SetVelocity(glm::vec3 updatedVelocity) { velocity = updatedVelocity; }

		void OnCollision();

	private:
		glm::vec3 velocity;
	};
}

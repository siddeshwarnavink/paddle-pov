#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "GameContext.hpp"
#include "GameEntity.hpp"

namespace Paddle {
	class Bullet : public GameEntity, public IBounded {
	public:
		Bullet(GameContext& context, float x, float y, float z);

		Bullet(const Bullet&) = delete;
		Bullet& operator=(const Bullet&) = delete;

		glm::vec3 GetHalfExtents() const override;
		bool CheckCollision(GameEntity* other) override;
		void Update() override;

		glm::vec3 GetVelocity() { return velocity; }
		void SetVelocity(glm::vec3 updatedVelocity) { velocity = updatedVelocity; }

	private:
		glm::vec3 velocity;
	};
}

#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "VkDevice.hpp"
#include "GameEntity.hpp"

namespace Paddle {
	class PlayerPaddle : public GameEntity, public IBounded {
	public:
		PlayerPaddle(Vk::Device& device);

		PlayerPaddle(const PlayerPaddle&) = delete;
		PlayerPaddle& operator=(const PlayerPaddle&) = delete;

		void Reset();
		bool CheckCollision(GameEntity* other) override;
		glm::vec3 GetHalfExtents() const override;
	};
}

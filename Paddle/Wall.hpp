#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "VkDevice.hpp"
#include "GameEntity.hpp"

namespace Paddle {
	class Wall : public GameEntity {
	public:
		Wall(Vk::Device& device, float x, float y, float z);

		Wall(const Wall&) = delete;
		Wall& operator=(const Wall&) = delete;
	};
}

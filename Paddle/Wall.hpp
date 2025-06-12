#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "VkDevice.hpp"
#include "GameEntity.hpp"

namespace Paddle {
	class Wall : public GameEntity, public IBounded {
	public:
		Wall(Vk::Device& device, float x, float y, float z, glm::vec3 halfExtents);

		Wall(const Wall&) = delete;
		Wall& operator=(const Wall&) = delete;

		glm::vec3 GetHalfExtents() const override;

	private:
		glm::vec3 halfExtents;
	};
}

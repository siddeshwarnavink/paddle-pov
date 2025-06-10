#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <vector>

#include "VkDevice.hpp"
#include "GameEntity.hpp"

namespace Paddle {
	struct CubePiece {
		glm::vec3 position;
		glm::vec3 velocity;
		glm::vec3 rotationAxis;
		float rotationSpeed;
		float currentAngle = 0.0f;
		float scale = 1.0f;
	};

	class Block : public GameEntity {
	public:
		Block(Vk::Device& device, float x, float y, float z, const glm::vec3& color);

		Block(const Block&) = delete;
		Block& operator=(const Block&) = delete;

		void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet) override;
		void Update(UpdateArgs args = UpdateArgs{}) override;

		void InitExplosion();
		bool IsExploded() { return isExploded; }
		bool IsExplosionInitiated() const { return isExplosionInitiated; }

	private:
		bool isExploded;
		bool isExplosionInitiated;
		std::vector<CubePiece> explodedPieces;
	};
}

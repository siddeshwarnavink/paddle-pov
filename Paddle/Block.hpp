#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <vector>
#include <memory>

#include "VkDevice.hpp"
#include "GameEntity.hpp"
#include "Block.hpp"

namespace Paddle {
	struct CubePiece {
		glm::vec3 position;
		glm::vec3 velocity;
		glm::vec3 rotationAxis;
		float rotationSpeed;
		float currentAngle = 0.0f;
		float scale = 1.0f;
		bool hasSubExploded = false;
	};

	class Block : public GameEntity, public IBounded {
	public:
		Block(Vk::Device& device, float x, float y, float z, const glm::vec3& color);

		Block(const Block&) = delete;
		Block& operator=(const Block&) = delete;

		void SetAllBlocksRef(std::vector<std::unique_ptr<Block>>* ref) { allBlocksRef = ref; } 
		static void CreateBlocks(Vk::Device& device, std::vector<std::unique_ptr<Block>>& blocks);

		glm::vec3 GetHalfExtents() const override;
		void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet) override;
		void Update(UpdateArgs args = UpdateArgs{}) override;

		void InitExplosion();
		bool IsExploded() { return isExploded; }
		bool IsExplosionInitiated() const { return isExplosionInitiated; }

	private:
		std::vector<std::unique_ptr<Block>>* allBlocksRef = nullptr;
		bool isTNTBlock;
		bool isRainbowBlock;
		bool isExploded;
		bool isExplosionInitiated;
		std::vector<CubePiece> explodedPieces;
	};
}

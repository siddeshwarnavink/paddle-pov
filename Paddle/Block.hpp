#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <vector>
#include <memory>

#include "GameEntity.hpp"
#include "Block.hpp"
#include "Loot.hpp"

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
		Block(GameContext &context, float x, float y, float z, const glm::vec3& color);

		Block(const Block&) = delete;
		Block& operator=(const Block&) = delete;

		void SetAllBlocksRef(std::vector<Block*>* ref) { allBlocksRef = ref; } 
		void SetAllLootsRef(std::vector<Loot*>* ref) { allLootsRef = ref; }
		static void CreateBlocks(GameContext& context, std::vector<Block*>& blocks, std::vector<Loot*>& loots);

		glm::vec3 GetHalfExtents() const override;
		void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet) override;
		void Update() override;

		void InitExplosion();
		bool IsExploded() { return isExploded; }
		bool IsExplosionInitiated() const { return isExplosionInitiated; }

	private:
		std::vector<Block*>* allBlocksRef = nullptr;
		std::vector<Loot*>*allLootsRef = nullptr;
		bool isTNTBlock;
		bool isRainbowBlock;
		bool isExploded;
		bool isExplosionInitiated;
		std::vector<CubePiece> explodedPieces;
	};
}

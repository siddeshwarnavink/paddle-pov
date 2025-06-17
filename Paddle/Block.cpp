#include "Block.hpp"
#include "VkDevice.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/epsilon.hpp>

#include <cstring>

#include "Utils.hpp"

using Utils::RandomChance;
using Utils::RandomNumber;
using Utils::DebugLog;

namespace Paddle {
	static constexpr int   BLOCK_ROWS = 3;
	static constexpr int   BLOCK_COLS = 8;
	static constexpr float BLOCK_SPACING = 1.15f;

	static constexpr float LOOT_PROB = 0.15f;
	static constexpr float TNT_PROB = 0.2f;
	static constexpr float RAINBOW_PROB = 0.05f;

	static const std::vector<glm::vec3> colors = {
		{15.0f / 255.0f, 30.0f / 255.0f, 63.0f / 255.0f},    // Dark Blue - #0f1e3f
		{213.0f / 255.0f, 21.0f / 255.0f, 24.0f / 255.0f},   // Bright Red - #d51518
		{251.0f / 255.0f, 229.0f / 255.0f, 74.0f / 255.0f},  // Bright Yellow - #fbe54a
		{38.0f / 255.0f, 109.0f / 255.0f, 45.0f / 255.0f},   // Dark Green - #266d2d
		{242.0f / 255.0f, 242.0f / 255.0f, 242.0f / 255.0f}, // White - #f2f2f2
	};

	Block::Block(GameContext& context, float x, float y, float z, const glm::vec3& color) : GameEntity(context) {
		allBlocksRef = nullptr;
		allLootsRef  = nullptr;
		
		tintColor = glm::vec4(color, 1);
		
		isExploded           = false;
		isExplosionInitiated = false;

		isTNTBlock = RandomChance(TNT_PROB);
		if(isTNTBlock) tintColor = glm::vec4(0.0f);

		isRainbowBlock = RandomChance(RAINBOW_PROB);
		if(isRainbowBlock) {
			lastColoChangeTime = time(NULL);
			isTNTBlock = false;
		}

		SetScale(glm::vec3(0.2f));
		SetRotation(glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f));
		SetPosition(glm::vec3(x, y, z));

		LoadModel("Shader\\Brick.obj");
		InitialiseEntity();
	}

	void Block::CreateBlocks(GameContext& context, std::vector<Block*>& blocks, std::vector<Loot*>& loots) {
		const float startX = -BLOCK_SPACING * 2;
		const float startY = -BLOCK_SPACING * 2;

		blocks.reserve(BLOCK_ROWS * BLOCK_COLS);
		for (size_t i = 0; i < BLOCK_ROWS; ++i) {
			for (size_t j = 0; j < BLOCK_COLS; ++j) {
				const int random = RandomNumber(0, static_cast<int>(colors.size()) - 1);
				const float x = startX + i * BLOCK_SPACING;
				const float y = startY + j * BLOCK_SPACING;
				glm::vec3 color = colors[random];
				blocks.emplace_back(new Block(context, x, y, 0.0f, color));
			}
		}

		for (auto& block : blocks) {
			block->SetAllBlocksRef(&blocks);
			block->SetAllLootsRef(&loots);
		}
	}

	void Block::InitExplosion() {
		if (isExplosionInitiated) return;
		isExplosionInitiated = true;
		explodedPieces.clear();

		//
		// Spawn Loot
		//
		if (RandomChance(LOOT_PROB) && allLootsRef) {
			const auto lootPosition = this->GetPosition();
			auto loot = new Loot(context, lootPosition.x, lootPosition.y, lootPosition.z);
			allLootsRef->emplace_back(loot);
		}

		//
		// TNT Block
		//
		if (isTNTBlock && allBlocksRef) {
			for (auto& otherBlockPtr : *allBlocksRef) {
				if (otherBlockPtr == this) continue;

				glm::vec3 otherPos = otherBlockPtr->GetPosition();
				glm::vec3 thisPos = this->GetPosition();

				glm::vec3 diff = glm::abs(otherPos - thisPos);

				int adjacentAxes = 0;
				int sameAxes = 0;
				for (int i = 0; i < 3; ++i) {
					if (glm::epsilonEqual(diff[i], BLOCK_SPACING, 0.01f)) adjacentAxes++;
					else if (glm::epsilonEqual(diff[i], 0.0f, 0.01f)) sameAxes++;
				}

				if (adjacentAxes == 1 && sameAxes == 2) {
					otherBlockPtr->InitExplosion();
					context.score += 10;
				}
			}
		}

		//
		// Rainbow block
		//
		if (isRainbowBlock && allBlocksRef) {
			for (auto& otherBlockPtr : *allBlocksRef) {
				if (otherBlockPtr == this) continue;
				otherBlockPtr->InitExplosion();
				context.score += 10;
			}
		}

		for (int x = -1; x <= 1; x += 2) {
			for (int y = -1; y <= 1; y += 2) {
				for (int z = -1; z <= 1; z += 2) {
					CubePiece piece;
					piece.position = glm::vec3(x, y, z) * 0.125f;

					glm::vec3 baseDir = glm::normalize(piece.position);
					glm::vec3 randomDir = glm::sphericalRand(0.5f);
					glm::vec3 velocityDir = glm::normalize(baseDir + randomDir);
					float speed = glm::linearRand(0.8f, 2.5f);

					piece.velocity = velocityDir * speed;
					piece.rotationAxis = glm::sphericalRand(1.0f);
					piece.rotationSpeed = glm::linearRand(1.0f, 3.0f);
					piece.scale = 1.0f;
					explodedPieces.push_back(piece);
				}
			}
		}
	}

	void Block::Update() {
		if(isRainbowBlock) {
			if(difftime(time(NULL), lastColoChangeTime) > 0.05f) {
				lastColoChangeTime = time(NULL);
				const int random = RandomNumber(0, static_cast<int>(colors.size()) - 1);
				tintColor = glm::vec4(colors[random], 1.0f);
				UpdateVertexColors(tintColor);
			}
		}
		if(!isExplosionInitiated || isExploded) return;

		const float deltaTime = 0.01f;
		bool allGone = true;
		std::vector<CubePiece> newSubPieces;

		for (auto& piece : explodedPieces) {
			piece.position += piece.velocity * deltaTime;
			piece.currentAngle += piece.rotationSpeed * deltaTime;
			piece.scale -= deltaTime * 0.5f;
			piece.scale = std::max(0.0f, piece.scale);

			if (!piece.hasSubExploded && piece.scale < 0.5f && piece.scale > 0.0f) {
				piece.hasSubExploded = true;
				for (int i = 0; i < 4; ++i) {
					CubePiece subPiece;
					subPiece.position = piece.position + glm::sphericalRand(0.03f);
					glm::vec3 randomDir = glm::sphericalRand(1.0f);
					subPiece.velocity = glm::normalize(randomDir) * glm::linearRand(0.5f, 1.5f);
					subPiece.rotationAxis = glm::sphericalRand(1.0f);
					subPiece.rotationSpeed = glm::linearRand(1.0f, 3.0f);
					subPiece.scale = piece.scale * 0.5f;
					subPiece.hasSubExploded = true;
					newSubPieces.push_back(subPiece);
				}
			}

			if (piece.scale > 0.0f) allGone = false;
		}

		explodedPieces.insert(explodedPieces.end(), newSubPieces.begin(), newSubPieces.end());

		if (allGone) {
			isExploded = true;
		}
	}

	glm::vec3 Block::GetHalfExtents() const {
		return glm::vec3(0.25f);
	}

	void Block::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet) {
		if (isExplosionInitiated) {
			for (const auto& piece : explodedPieces) {
				if (piece.scale <= 0.0f) continue;
				glm::mat4 model = glm::translate(glm::mat4(1.0f), position + piece.position);
				model = glm::rotate(model, piece.currentAngle, piece.rotationAxis);
				model = glm::scale(model, glm::vec3(piece.scale * 0.5f));

				vkCmdPushConstants(
					commandBuffer,
					pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT,
					0,
					sizeof(glm::mat4),
					&model
				);

				VkBuffer vertexBuffers[] = { vertexBuffer };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indicesInstance.size()), 1, 0, 0, 0);
			}
		}
		else {
			glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
			model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
			model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
			model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
			model = glm::scale(model, glm::vec3(1.0f));

			vkCmdPushConstants(
				commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT,
				0,
				sizeof(glm::mat4),
				&model
			);

			VkBuffer vertexBuffers[] = { vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indicesInstance.size()), 1, 0, 0, 0);
		}
	}
}
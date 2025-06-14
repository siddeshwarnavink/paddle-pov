#include "Block.hpp"
#include "VkDevice.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/epsilon.hpp>

#include <cstring>
#include <random>
#include <algorithm>

namespace Paddle {
	static constexpr float BLOCK_SPACING = 0.65f;

	static constexpr float TNT_PROB     = 0.2f;
	static constexpr float RAINBOW_PROB = 0.05f;

	Block::Block(GameContext& context, float x, float y, float z, const glm::vec3& color) : GameEntity(context) {
		allBlocksRef = nullptr;

		verticesInstance = {
			// Front face
			{{-0.25f, -0.25f,  0.25f}, {0,0,0}, {0,0,1}, {0,0}},
			{{ 0.25f, -0.25f,  0.25f}, {0,0,0}, {0,0,1}, {1,0}},
			{{ 0.25f,  0.25f,  0.25f}, {0,0,0}, {0,0,1}, {1,1}},
			{{-0.25f,  0.25f,  0.25f}, {0,0,0}, {0,0,1}, {0,1}},
			// Back face
			{{-0.25f, -0.25f, -0.25f}, {0,0,0}, {0,0,-1}, {1,0}},
			{{ 0.25f, -0.25f, -0.25f}, {0,0,0}, {0,0,-1}, {0,0}},
			{{ 0.25f,  0.25f, -0.25f}, {0,0,0}, {0,0,-1}, {0,1}},
			{{-0.25f,  0.25f, -0.25f}, {0,0,0}, {0,0,-1}, {1,1}},
		};
		indicesInstance = {
			0, 1, 2, 2, 3, 0,  // Front face
			1, 5, 6, 6, 2, 1,  // Right face
			5, 4, 7, 7, 6, 5,  // Back face
			4, 0, 3, 3, 7, 4,  // Left face
			3, 2, 6, 6, 7, 3,  // Top face
			4, 5, 1, 1, 0, 4   // Bottom face
		};

		glm::vec3 applyColor = color;

		std::random_device rd;
		std::mt19937 gen(rd());
		std::bernoulli_distribution tnt_dis(TNT_PROB);
		std::bernoulli_distribution rainbow_dis(RAINBOW_PROB);

		isTNTBlock = tnt_dis(gen);
		if(isTNTBlock)
			applyColor = glm::vec3(0.0f);

		isRainbowBlock = rainbow_dis(gen);
		if (isRainbowBlock) {
			applyColor = glm::vec3(1.0f, 0.0f, 1.0f);
			isTNTBlock = false;
		}

		for (auto& v : verticesInstance) v.color = applyColor;

		SetPosition(glm::vec3(x, y, z));
		InitialiseEntity();

		isExploded = false;
		isExplosionInitiated = false;
	}

	void Block::CreateBlocks(GameContext& context, std::vector<std::unique_ptr<Block>>& blocks) {
		const float startX = -BLOCK_SPACING * 2;
		const float startY = -BLOCK_SPACING * 2;

		const std::vector<glm::vec3> colors = {
			{124.0f / 255.0f, 156.0f / 255.0f, 228.0f / 255.0f}, // #7c9ce4
			{100.0f / 255.0f, 230.0f / 255.0f, 215.0f / 255.0f}, // #64e6d7
			{231.0f / 255.0f, 235.0f / 255.0f, 185.0f / 255.0f}, // #e7ebb9
			{179.0f / 255.0f, 240.0f / 255.0f, 255.0f / 255.0f}, // #b3f0ff
			{30.0f / 255.0f, 151.0f / 255.0f, 158.0f / 255.0f},  // #1e979e
		};
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, static_cast<int>(colors.size()) - 1);

		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 8; ++j) {
				float x = startX + i * BLOCK_SPACING;
				float y = startY + j * BLOCK_SPACING;
				glm::vec3 color = colors[dis(gen)];
				blocks.emplace_back(std::make_unique<Block>(context, x, y, 0.0f, color));
			}
		}

		for (auto& block : blocks) block->SetAllBlocksRef(&blocks);
	}

	void Block::InitExplosion() {
		if (isExplosionInitiated) return;
		isExplosionInitiated = true;
		explodedPieces.clear();

		//
		// TNT Block
		//
		if (isTNTBlock && allBlocksRef) {
			for (auto& otherBlockPtr : *allBlocksRef) {
				if (!otherBlockPtr || otherBlockPtr.get() == this) continue;

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
				if (!otherBlockPtr || otherBlockPtr.get() == this) continue;
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
		if (!isExplosionInitiated || isExploded) return;

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

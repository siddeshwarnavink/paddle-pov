#include "Block.hpp"
#include "VkDevice.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

#include <cstring>

namespace Paddle {
	Block::Block(Vk::Device& device, float x, float y, float z, const glm::vec3& color) : GameEntity(device) {
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
		for (auto& v : verticesInstance) v.color = color;
		SetPosition(glm::vec3(x, y, z));
		InitialiseEntity();

		isExploded = false;
		isExplosionInitiated = false;
	}

	void Block::InitExplosion() {
		if (isExplosionInitiated) return;
		explodedPieces.clear();
		isExplosionInitiated = true;

		for (int x = -1; x <= 1; x += 2) {
			for (int y = -1; y <= 1; y += 2) {
				for (int z = -1; z <= 1; z += 2) {
					CubePiece piece;
					piece.position = glm::vec3(x, y, z) * 0.125f;           // center of mini cube
					piece.velocity = glm::normalize(piece.position) * 1.0f; // outward
					piece.rotationAxis = glm::sphericalRand(1.0f);          // random axis
					piece.rotationSpeed = glm::linearRand(1.0f, 3.0f);
					piece.scale = 1.0f;
					explodedPieces.push_back(piece);
				}
			}
		}
	}

	void Block::Update(UpdateArgs args) {
		if (!isExplosionInitiated || isExploded) return;

		const float deltaTime = 0.01f;
		bool allGone = true;
		for (auto& piece : explodedPieces) {
			piece.position += piece.velocity * deltaTime;
			piece.currentAngle += piece.rotationSpeed * deltaTime;
			piece.scale -= deltaTime * 0.5f;
			piece.scale = std::max(0.0f, piece.scale);
			if (piece.scale > 0.0f) allGone = false;
		}
		if (allGone) {
			isExploded = true;
		}
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
				vkCmdDrawIndexed(commandBuffer, indicesInstance.size(), 1, 0, 0, 0);
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
			vkCmdDrawIndexed(commandBuffer, indicesInstance.size(), 1, 0, 0, 0);
		}
	}
}

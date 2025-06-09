#include "Block.hpp"
#include "VkDevice.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

#include <cstring>

namespace Paddle {
	//
	// NOTE: Vector = { position, color, normal, uv }
	//
	const std::vector<Vertex> Block::vertices = {
		// Front face (z = +0.25)
		{{-0.25f, -0.25f,  0.25f}, {0,0,0}, {0,0,1}, {0,0}},
		{{ 0.25f, -0.25f,  0.25f}, {0,0,0}, {0,0,1}, {1,0}},
		{{ 0.25f,  0.25f,  0.25f}, {0,0,0}, {0,0,1}, {1,1}},
		{{-0.25f,  0.25f,  0.25f}, {0,0,0}, {0,0,1}, {0,1}},
		// Back face (z = -0.25)
		{{-0.25f, -0.25f, -0.25f}, {0,0,0}, {0,0,-1}, {1,0}},
		{{ 0.25f, -0.25f, -0.25f}, {0,0,0}, {0,0,-1}, {0,0}},
		{{ 0.25f,  0.25f, -0.25f}, {0,0,0}, {0,0,-1}, {0,1}},
		{{-0.25f,  0.25f, -0.25f}, {0,0,0}, {0,0,-1}, {1,1}},
	};

	const std::vector<uint16_t> Block::indices = {
		0, 1, 2, 2, 3, 0,  // Front face
		1, 5, 6, 6, 2, 1,  // Right face
		5, 4, 7, 7, 6, 5,  // Back face
		4, 0, 3, 3, 7, 4,  // Left face
		3, 2, 6, 6, 7, 3,  // Top face
		4, 5, 1, 1, 0, 4   // Bottom face
	};

	Block::Block(Vk::Device& device, float x, float y, float z, const glm::vec3& color)
		: device(device), color(color) {
		SetPosition(glm::vec3(x, y, z));
		SetRotation(glm::vec3(0.0f));
		verticesInstance = vertices;
		for (auto& v : verticesInstance) v.color = color;
		CreateVertexBuffer();
		CreateIndexBuffer();

		isExploded = false;
		isExplosionInitiated = false;
	}

	Block::~Block() {
		vkDestroyBuffer(device.device(), vertexBuffer, nullptr);
		vkFreeMemory(device.device(), vertexBufferMemory, nullptr);
		vkDestroyBuffer(device.device(), indexBuffer, nullptr);
		vkFreeMemory(device.device(), indexBufferMemory, nullptr);
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

	void Block::Update() {
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

	void Block::CreateVertexBuffer() {
		VkDeviceSize bufferSize = sizeof(verticesInstance[0]) * verticesInstance.size();
		device.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBuffer,
			vertexBufferMemory);
		device.SetObjectName((uint64_t)vertexBuffer, VK_OBJECT_TYPE_BUFFER, "Block Vertex Buffer");
		void* data;
		vkMapMemory(device.device(), vertexBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, verticesInstance.data(), (size_t)bufferSize);
		vkUnmapMemory(device.device(), vertexBufferMemory);
	}

	void Block::CreateIndexBuffer() {
		indexCount = static_cast<uint32_t>(indices.size());
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
		device.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			indexBuffer,
			indexBufferMemory);
		device.SetObjectName((uint64_t)indexBuffer, VK_OBJECT_TYPE_BUFFER, "Block Index Buffer");
		void* data;
		vkMapMemory(device.device(), indexBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(device.device(), indexBufferMemory);
	}

	void Block::Bind(VkCommandBuffer commandBuffer)
	{
		// ...
	}

	void Block::Draw(VkCommandBuffer commandBuffer)
	{
		// ...
	}

	void Block::DrawBlock(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet) {
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
				vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
				vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
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
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
			vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		}
	}


	uint32_t Block::GetIndexCount() const {
		return indexCount;
	}
}

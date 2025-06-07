#include "PlayerPaddle.hpp"
#include "VkDevice.hpp"
#include <cstring>

namespace Paddle {
	//
	// NOTE: Vector = { position, color, normal, uv }
	//
	const std::vector<Vertex> PlayerPaddle::vertices = {
		// Front face (z = +0.075)
		{{-0.001f, -0.25f,  0.075f}, {0,0,0}, {0,0,1}, {0,0}},
		{{ 0.001f, -0.25f,  0.075f}, {0,0,0}, {0,0,1}, {1,0}},
		{{ 0.001f,  0.25f,  0.075f}, {0,0,0}, {0,0,1}, {1,1}},
		{{-0.001f,  0.25f,  0.075f}, {0,0,0}, {0,0,1}, {0,1}},
		// Back face (z = -0.075)
		{{-0.001f, -0.25f, -0.075f}, {0,0,0}, {0,0,-1}, {1,0}},
		{{ 0.001f, -0.25f, -0.075f}, {0,0,0}, {0,0,-1}, {0,0}},
		{{ 0.001f,  0.25f, -0.075f}, {0,0,0}, {0,0,-1}, {0,1}},
		{{-0.001f,  0.25f, -0.075f}, {0,0,0}, {0,0,-1}, {1,1}},
	};

	const std::vector<uint16_t> PlayerPaddle::indices = {
	    0, 1, 2, 2, 3, 0,  // Front face
	    1, 5, 6, 6, 2, 1,  // Right face
	    5, 4, 7, 7, 6, 5,  // Back face
	    4, 0, 3, 3, 7, 4,  // Left face
	    3, 2, 6, 6, 7, 3,  // Top face
	    4, 5, 1, 1, 0, 4   // Bottom face
	};

	PlayerPaddle::PlayerPaddle(Vk::Device& device, float x, float y, float z)
		: device(device) {
		SetPosition(glm::vec3(x, y, z));
		color = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);
		verticesInstance = vertices;
		for (auto& v : verticesInstance) v.color = color;
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	PlayerPaddle::~PlayerPaddle() {
		vkDestroyBuffer(device.device(), vertexBuffer, nullptr);
		vkFreeMemory(device.device(), vertexBufferMemory, nullptr);
		vkDestroyBuffer(device.device(), indexBuffer, nullptr);
		vkFreeMemory(device.device(), indexBufferMemory, nullptr);
	}

	void PlayerPaddle::CreateVertexBuffer() {
		VkDeviceSize bufferSize = sizeof(verticesInstance[0]) * verticesInstance.size();
		device.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBuffer,
			vertexBufferMemory);
		void* data;
		vkMapMemory(device.device(), vertexBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, verticesInstance.data(), (size_t)bufferSize);
		vkUnmapMemory(device.device(), vertexBufferMemory);
	}

	void PlayerPaddle::CreateIndexBuffer() {
		indexCount = static_cast<uint32_t>(indices.size());
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
		device.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			indexBuffer,
			indexBufferMemory);
		void* data;
		vkMapMemory(device.device(), indexBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(device.device(), indexBufferMemory);
	}

	void PlayerPaddle::Bind(VkCommandBuffer commandBuffer) {
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
	}

	void PlayerPaddle::Draw(VkCommandBuffer commandBuffer) {
		vkCmdDrawIndexed(commandBuffer, GetIndexCount(), 1, 0, 0, 0);
	}

	uint32_t PlayerPaddle::GetIndexCount() const {
		return indexCount;
	}
}

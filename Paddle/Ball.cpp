#include "Ball.hpp"
#include "VkDevice.hpp"
#include <cstring>
#include <glm/gtc/constants.hpp>

namespace Paddle {
	const float radius = 0.25f;
	const uint32_t slices = 64;
	const uint32_t stacks = 32;

	Ball::Ball(Vk::Device& device, float x, float y, float z)
		: device(device) {
		SetPosition(glm::vec3(x, y, z));
		SetRotation(glm::vec3(0.0f));
		verticesInstance = GenerateVertices();
		CreateVertexBuffer();
		CreateIndexBuffer();
		velocity = glm::vec3(-1.0f, 0.5f, 0.0f);
	}

	Ball::~Ball() {
		vkDestroyBuffer(device.device(), vertexBuffer, nullptr);
		vkFreeMemory(device.device(), vertexBufferMemory, nullptr);
		vkDestroyBuffer(device.device(), indexBuffer, nullptr);
		vkFreeMemory(device.device(), indexBufferMemory, nullptr);
	}

	void Ball::Update() {
		glm::vec3 pos = GetPosition();
		pos += velocity * 0.01f; // TODO: Later move 0.01 as speed in Game class
								 // and we can increase speed as game goes on.
		SetPosition(pos);
	}

	glm::vec3 Ball::GetVelocity() {
		return velocity;
	}

	void Ball::SetVelocity(glm::vec3 updatedVelocity) {
		velocity = updatedVelocity;
	}

	std::vector<Vertex> Ball::GenerateVertices() {
		//
		// Ref: https://en.wikipedia.org/wiki/Spherical_coordinate_system
		//

		std::vector<Vertex> vertices;
		for (int i = 0; i <= stacks; ++i) {
			float phi = glm::pi<float>() * i / stacks;
			for (int j = 0; j <= slices; ++j) {
				float theta = 2 * glm::pi<float>() * j / slices;

				float x = radius * sin(phi) * cos(theta);
				float y = radius * cos(phi);
				float z = radius * sin(phi) * sin(theta);

				glm::vec3 pos = glm::vec3(x, y, z);
				glm::vec3 color = glm::vec3(194.0f / 255.f, 64.0f / 255.0f, 62.0f / 255.0f); // #c2403e
				glm::vec3 normal = glm::normalize(pos);
				glm::vec2 uv = glm::vec2((float)j / slices, (float)i / stacks);

				vertices.push_back({ pos, color, normal, uv });
			}
		}
		return vertices;
	}

	std::vector<uint32_t> Ball::GenerateIndices() {
		std::vector<uint32_t> indices;
		for (int i = 0; i < stacks; ++i) {
			for (int j = 0; j < slices; ++j) {
				int first = i * (slices + 1) + j;
				int second = first + slices + 1;

				indices.push_back(first);
				indices.push_back(second);
				indices.push_back(first + 1);

				indices.push_back(second);
				indices.push_back(second + 1);
				indices.push_back(first + 1);
			}
		}
		return indices;
	}

	void Ball::CreateVertexBuffer() {
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

	void Ball::CreateIndexBuffer() {
		std::vector<uint32_t> indices = GenerateIndices();
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

	void Ball::Bind(VkCommandBuffer commandBuffer) {
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	}

	void Ball::Draw(VkCommandBuffer commandBuffer) {
		vkCmdDrawIndexed(commandBuffer, GetIndexCount(), 1, 0, 0, 0);
	}

	uint32_t Ball::GetIndexCount() const {
		return indexCount;
	}
}

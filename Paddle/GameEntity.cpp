#include "GameEntity.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace Paddle {
	GameEntity::GameEntity(Vk::Device& device) : device(device) {
		position = glm::vec3(0.0f);
		rotation = glm::vec3(0.0f);
	}

	GameEntity::~GameEntity() {
		vkDestroyBuffer(device.device(), vertexBuffer, nullptr);
		vkFreeMemory(device.device(), vertexBufferMemory, nullptr);
		vkDestroyBuffer(device.device(), indexBuffer, nullptr);
		vkFreeMemory(device.device(), indexBufferMemory, nullptr);
	}

	void GameEntity::InitialiseEntity() {
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	void GameEntity::CreateVertexBuffer() {
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

	void GameEntity::CreateIndexBuffer() {
		VkDeviceSize bufferSize = sizeof(indicesInstance[0]) * indicesInstance.size();
		device.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			indexBuffer,
			indexBufferMemory);
		device.SetObjectName((uint64_t)indexBuffer, VK_OBJECT_TYPE_BUFFER, "Block Index Buffer");
		void* data;
		vkMapMemory(device.device(), indexBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indicesInstance.data(), (size_t)bufferSize);
		vkUnmapMemory(device.device(), indexBufferMemory);
	}

	void GameEntity::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet) {
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, position);
		model = glm::rotate(model, rotation.x, glm::vec3(1, 0, 0));
		model = glm::rotate(model, rotation.y, glm::vec3(0, 1, 0));
		model = glm::rotate(model, rotation.z, glm::vec3(0, 0, 1));

		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);

		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indicesInstance.size()), 1, 0, 0, 0);
	}
}

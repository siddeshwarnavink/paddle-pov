#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <vector>

#include "VkDevice.hpp"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec3 normal;
	glm::vec2 uv;
};

namespace Paddle {
	union UpdateArgs {
		int i;
		float f;
		double d;
		glm::vec2 v2;
		glm::vec3 v3;
		glm::vec4 v4;
	};

	class GameEntity {
	public:
		GameEntity(Vk::Device& device);
		virtual ~GameEntity();

		GameEntity(const GameEntity&) = delete;
		GameEntity& operator=(const GameEntity&) = delete;

		void SetPosition(const glm::vec3& pos) { position = pos; }
		void SetRotation(const glm::vec3& rot) { rotation = rot; }

		glm::vec3 GetPosition() const { return position; }
		glm::vec3 GetRotation() const { return rotation; }

		virtual void Update(UpdateArgs args = UpdateArgs{});
		virtual void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet);

	protected:
		Vk::Device& device;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
		std::vector<Vertex> verticesInstance;
		std::vector<uint32_t> indicesInstance;

		void InitialiseEntity();

		glm::vec3 position;
		glm::vec3 rotation;

	private:
		void CreateVertexBuffer();
		void CreateIndexBuffer();
	};
}

#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "VkDevice.hpp"
#include "GameEntity.hpp"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
};

namespace Paddle {
	class Block : public GameEntity {
	public:
		Block(Vk::Device& device);
		~Block() override;
		
		Block(const Block&) = delete;
		Block& operator=(const Block&) = delete;

		void CreateVertexBuffer();
		void CreateIndexBuffer();
		void Bind(VkCommandBuffer commandBuffer) override;
		void Draw(VkCommandBuffer commandBuffer) override;
		uint32_t GetIndexCount() const;

	private:
		Vk::Device& device;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
		uint32_t indexCount;

		static const std::vector<Vertex> vertices;
		static const std::vector<uint16_t> indices;
	};
}

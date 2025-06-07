#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "VkDevice.hpp"
#include "GameEntity.hpp"

namespace Paddle {
	class Wall : public GameEntity {
	public:
		Wall(Vk::Device& device, float x, float y, float z);
		~Wall() override;

		Wall(const Wall&) = delete;
		Wall& operator=(const Wall&) = delete;

		void CreateVertexBuffer();
		void CreateIndexBuffer();
		void Bind(VkCommandBuffer commandBuffer) override;
		void Draw(VkCommandBuffer commandBuffer) override;
		uint32_t GetIndexCount() const;

	private:
		Vk::Device& device;
		glm::vec3 color;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
		uint32_t indexCount;

		std::vector<Vertex> verticesInstance;

		static const std::vector<Vertex> vertices;
		static const std::vector<uint16_t> indices;
	};
}

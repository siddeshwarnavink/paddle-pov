#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "VkDevice.hpp"
#include "GameEntity.hpp"

namespace Paddle {
	class Ball : public GameEntity {
	public:
		Ball(Vk::Device& device, float x, float y, float z);
		~Ball() override;

		Ball(const Ball&) = delete;
		Ball& operator=(const Ball&) = delete;

		void CreateVertexBuffer();
		void CreateIndexBuffer();
		void Bind(VkCommandBuffer commandBuffer) override;
		void Draw(VkCommandBuffer commandBuffer) override;
		uint32_t GetIndexCount() const;

		void Update(int score);
		glm::vec3 GetVelocity();
		void SetVelocity(glm::vec3 updatedVelocity);

		float GetRadius();

	private:
		Vk::Device& device;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
		uint32_t indexCount;

		std::vector<Vertex> verticesInstance;

		static const std::vector<Vertex> vertices;
		static const std::vector<uint16_t> indices;

		float radius = 0.25;
		glm::vec3 velocity;

		std::vector<Vertex> GenerateVertices();
		std::vector<uint32_t> GenerateIndices();
	};
}

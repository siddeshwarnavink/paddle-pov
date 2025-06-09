#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "VkDevice.hpp"
#include "GameEntity.hpp"

namespace Paddle {
	struct CubePiece {
		glm::vec3 position;
		glm::vec3 velocity;
		glm::vec3 rotationAxis;
		float rotationSpeed;
		float currentAngle = 0.0f;
		float scale = 1.0f;
	};

	class Block : public GameEntity {
	public:
		Block(Vk::Device& device, float x, float y, float z, const glm::vec3& color);
		~Block() override;

		Block(const Block&) = delete;
		Block& operator=(const Block&) = delete;

		void CreateVertexBuffer();
		void CreateIndexBuffer();
		void Bind(VkCommandBuffer commandBuffer) override;
		void Draw(VkCommandBuffer commandBuffer) override;
		void DrawBlock(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet);
		uint32_t GetIndexCount() const;

		void InitExplosion();
		void Update();
		bool IsExploded() { return isExploded; }
		bool IsExplosionInitiated() const { return isExplosionInitiated; }

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

		bool isExploded;
		bool isExplosionInitiated;
		std::vector<CubePiece> explodedPieces;
	};
}

#pragma once

#include "GameVertex.hpp"
#include "GameContext.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <vector>

namespace Paddle {
	class GameEntity {
	public:
		GameEntity(GameContext &context);
		virtual ~GameEntity();

		GameEntity(const GameEntity&) = delete;
		GameEntity& operator=(const GameEntity&) = delete;

		void SetPosition(const glm::vec3& pos) { position = pos; }
		void SetRotation(const glm::vec3& rot) { rotation = rot; }

		glm::vec3 GetPosition() const { return position; }
		glm::vec3 GetRotation() const { return rotation; }

		virtual void Update() {}
		virtual void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet);

		virtual bool CheckCollision(GameEntity* other) { return false; }

	protected:
		GameContext& context;
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


	class IBounded {
	public:
		virtual glm::vec3 GetHalfExtents() const = 0;
		virtual ~IBounded() = default;
	};
}

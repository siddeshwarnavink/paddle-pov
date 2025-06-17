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

		GameEntity* AsEntity() { return this; }

		void SetPosition(const glm::vec3& pos) { position = pos; }
		void SetRotation(const glm::vec3& rot) { rotation = rot; }

		glm::vec3 GetPosition() const { return position; }
		glm::vec3 GetRotation() const { return rotation; }

		void SetScale(const glm::vec3& s) { scale = s; }
		void SetTintColor(const glm::vec4& color) { tintColor = color; }

		virtual void Update() {}
		virtual void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet);

		virtual bool CheckCollision(GameEntity* other) { return false; }

		void MarkForDestruction() { toBeDestroyed = true; }
		bool IsMarkedForDestruction() const { return toBeDestroyed; }

	protected:
		GameContext& context;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
		std::vector<Vertex> verticesInstance;
		std::vector<uint32_t> indicesInstance;

		void InitialiseEntity();
		void LoadModel(std::string path);
		void UpdateVertexColors(const glm::vec4& color);

		glm::vec3 scale = glm::vec3(1.0f);
		glm::vec4 tintColor = glm::vec4(1.0f);
		glm::vec3 position;
		glm::vec3 rotation;

	private:
		bool toBeDestroyed = false;

		void CreateVertexBuffer();
		void CreateIndexBuffer();
	};


	class IBounded {
	public:
		virtual glm::vec3 GetHalfExtents() const = 0;
		virtual ~IBounded() = default;
	};
}

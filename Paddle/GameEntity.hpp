#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
};

namespace Paddle {
    class GameEntity {
    public:
        GameEntity();
        virtual ~GameEntity();

        GameEntity(const GameEntity&) = delete;
		GameEntity& operator=(const GameEntity&) = delete;

        virtual void Bind(VkCommandBuffer commandBuffer) = 0;
        virtual void Draw(VkCommandBuffer commandBuffer) = 0;

        void SetPosition(const glm::vec3& pos);
        void SetRotation(const glm::vec3& rot);
        glm::vec3 GetPosition() const;
        glm::vec3 GetRotation() const;
        glm::mat4 GetModelMatrix() const;

    protected:
        glm::vec3 position;
        glm::vec3 rotation;
    };
}

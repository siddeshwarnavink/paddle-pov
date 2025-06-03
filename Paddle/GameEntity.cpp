#include "GameEntity.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace Paddle {
    GameEntity::GameEntity() : position(0.0f), rotation(0.0f) {}
    GameEntity::~GameEntity() {}

    void GameEntity::SetPosition(const glm::vec3& pos) { position = pos; }
    void GameEntity::SetRotation(const glm::vec3& rot) { rotation = rot; }
    glm::vec3 GameEntity::GetPosition() const { return position; }
    glm::vec3 GameEntity::GetRotation() const { return rotation; }
    glm::mat4 GameEntity::GetModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, rotation.x, glm::vec3(1,0,0));
        model = glm::rotate(model, rotation.y, glm::vec3(0,1,0));
        model = glm::rotate(model, rotation.z, glm::vec3(0,0,1));
        return model;
    }
}

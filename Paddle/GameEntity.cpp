#include "GameEntity.hpp"
#include "Utils.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Vendor\tiny_obj_loader.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

using Utils::DebugLog;

namespace Paddle {
	GameEntity::GameEntity(GameContext& context) : context(context) {
		position = glm::vec3(0.0f);
		rotation = glm::vec3(0.0f);

		// InitialiseEntity() will be called by child classes
	}

	GameEntity::~GameEntity() {
		vkDestroyBuffer(context.device->device(), vertexBuffer, nullptr);
		vkFreeMemory(context.device->device(), vertexBufferMemory, nullptr);
		vkDestroyBuffer(context.device->device(), indexBuffer, nullptr);
		vkFreeMemory(context.device->device(), indexBufferMemory, nullptr);
	}

	void GameEntity::LoadModel(std::string path) {
		DebugLog("Loading model from: " + path);

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials; // Not used for now
		std::string warn, err;

		bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), nullptr, true);
		if(!warn.empty()) DebugLog("Tinyobj warning: " + warn);
		if(!err.empty())  DebugLog("Tinyobj error: " + err);

		if(!ret) throw std::runtime_error("Failed to load OBJ file: " + path);

		std::unordered_map<Vertex, uint32_t, VertexHasher> uniqueVertices{};
		verticesInstance.clear();
		indicesInstance.clear();

		for(const auto& shape : shapes) {
			for(const auto& index : shape.mesh.indices) {
				Vertex vertex{};
				vertex.color = tintColor;

				// Position
				if(index.vertex_index >= 0) {
					vertex.pos = {
							attrib.vertices[3 * index.vertex_index + 0],
							attrib.vertices[3 * index.vertex_index + 1],
							attrib.vertices[3 * index.vertex_index + 2]
					};
					vertex.pos *= scale;
				}

				// Normal
				if(index.normal_index >= 0) {
					vertex.normal = {
							attrib.normals[3 * index.normal_index + 0],
							attrib.normals[3 * index.normal_index + 1],
							attrib.normals[3 * index.normal_index + 2]
					};
				}
				else vertex.normal = glm::vec3(0.0f);

				// Texture Coordinates (UV)
				if(index.texcoord_index >= 0) {
					vertex.uv = {
							attrib.texcoords[2 * index.texcoord_index + 0],
							1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // Flip V
					};
				}
				else vertex.uv = glm::vec2(0.0f);

				// De-duplicate
				if(uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(verticesInstance.size());
					verticesInstance.push_back(vertex);
				}

				indicesInstance.push_back(uniqueVertices[vertex]);
			}
		}

		// Rotation
		glm::mat4 rotMatrix = glm::yawPitchRoll(rotation.y, rotation.x, rotation.z);
		for(auto& v : verticesInstance) {
			glm::vec4 transformedPos = rotMatrix * glm::vec4(v.pos, 1.0f);
			glm::vec4 transformedNormal = rotMatrix * glm::vec4(v.normal, 0.0f);

			v.pos = glm::vec3(transformedPos);
			v.normal = glm::normalize(glm::vec3(transformedNormal));
		}
	}

	void GameEntity::InitialiseEntity() {
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	void GameEntity::CreateVertexBuffer() {
		VkDeviceSize bufferSize = sizeof(verticesInstance[0]) * verticesInstance.size();
		context.device->createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBuffer,
			vertexBufferMemory);
		context.device->SetObjectName((uint64_t)vertexBuffer, VK_OBJECT_TYPE_BUFFER, "Block Vertex Buffer");
		void* data;
		vkMapMemory(context.device->device(), vertexBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, verticesInstance.data(), (size_t)bufferSize);
		vkUnmapMemory(context.device->device(), vertexBufferMemory);
	}

	void GameEntity::CreateIndexBuffer() {
		VkDeviceSize bufferSize = sizeof(indicesInstance[0]) * indicesInstance.size();
		context.device->createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			indexBuffer,
			indexBufferMemory);
		context.device->SetObjectName((uint64_t)indexBuffer, VK_OBJECT_TYPE_BUFFER, "Block Index Buffer");
		void* data;
		vkMapMemory(context.device->device(), indexBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indicesInstance.data(), (size_t)bufferSize);
		vkUnmapMemory(context.device->device(), indexBufferMemory);
	}

	void GameEntity::UpdateVertexColors(const glm::vec4& color) {
		// Update color in all vertices
		for(auto& v : verticesInstance) {
			v.color = color;
		}
		// Re-upload to GPU
		VkDeviceSize bufferSize = sizeof(verticesInstance[0]) * verticesInstance.size();
		void* data;
		vkMapMemory(context.device->device(), vertexBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, verticesInstance.data(), (size_t)bufferSize);
		vkUnmapMemory(context.device->device(), vertexBufferMemory);
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

#include "Game.hpp"
#include "Block.hpp"
#include "Ball.hpp"
#include "GameEntity.hpp"
#include "GameCamera.hpp"

#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <random>

#include <stdexcept>
#include <array>

namespace Paddle
{
	Game::Game()
		: window(WIDTH, HEIGHT, "Paddle Game"),
		device(window),
		swapChain(device, window.getExtent())
	{
		CreateDescriptorSetLayout();
		CreateUniformBuffer();
		CreateDescriptorPool();
		CreateDescriptorSet();
		CreatePipelineLayout();
		CreatePipeline();
		CreateBlocks();
		CreateBall();
		CreateCommandBuffers();
	}

	Game::~Game() {
		vkDestroyBuffer(device.device(), cameraUbo, nullptr);
		vkFreeMemory(device.device(), cameraUboMemory, nullptr);
		//vkDestroyBuffer(device.device(), lightBuffer, nullptr);
		//vkFreeMemory(device.device(), lightBufferMemory, nullptr);
		vkDestroyDescriptorPool(device.device(), descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device.device(), descriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
	}

	void Game::CreateBall() {
		ballEntity = std::make_unique<Ball>(device, 3.0f, 0.0f, 0.0f);
	}

	void Game::CreateBlocks() {
		float spacing = 0.65f;
		float startX = -spacing * 2;
		float startY = -spacing * 2;

		//
		// Random block color
		//
		std::vector<glm::vec3> colors = {
			{1.0f, 0.0f, 0.0f}, // red
			{0.0f, 1.0f, 0.0f}, // green
			{0.0f, 0.0f, 1.0f}, // blue
			{1.0f, 1.0f, 0.0f}  // yellow
		};
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, static_cast<int>(colors.size()) - 1);

		for (int i = 0; i < 5; ++i) {
			for (int j = 0; j < 5; ++j) {
				float x = startX + i * spacing;
				float y = startY + j * spacing;
				glm::vec3 color = colors[dis(gen)];
				entities.emplace_back(std::make_unique<Block>(device, x, y, 0.0f, color));
			}
		}
	}

	bool CheckAABBSphereCollision(const glm::vec3& boxMin, const glm::vec3& boxMax, const glm::vec3& sphereCenter, float sphereRadius) {
		glm::vec3 closestPoint = glm::clamp(sphereCenter, boxMin, boxMax);
		float distanceSquared = glm::distance2(closestPoint, sphereCenter);
		return distanceSquared < (sphereRadius * sphereRadius);
	}

	void Game::run() {
		while (!window.ShouldClose()) {
			window.PollEvents();

			//
			// Update ball
			//
			static_cast<Paddle::Ball*>(ballEntity.get())->Update();

			//
			// Collision detection
			//
			float ballRadius = 0.25f;
			glm::vec3 ballPos = ballEntity->GetPosition();

			for (auto& entity : entities) {
				glm::vec3 blockPos = entity->GetPosition();
				glm::vec3 blockMin = blockPos - glm::vec3(0.25f);
				glm::vec3 blockMax = blockPos + glm::vec3(0.25f);

				if (CheckAABBSphereCollision(blockMin, blockMax, ballPos, ballRadius)) {
					//
					// Deflect the ball
					//
					auto *ball = static_cast<Paddle::Ball*>(ballEntity.get());
					auto vel = ball->GetVelocity();

					glm::vec3 closestPoint = glm::clamp(ballPos, blockMin, blockMax);
					glm::vec3 normal = glm::normalize(ballPos - closestPoint);

					if (glm::length(normal) < 0.0001f) {
						normal = glm::vec3(1, 0, 0);
					}

					vel = vel - 2.0f * glm::dot(vel, normal) * normal;
					ball->SetVelocity(vel);
				}
			}

			//
			// Camera movement logic
			//
			if (window.IsKeyPressed(GLFW_KEY_LEFT) || window.IsKeyPressed(GLFW_KEY_A)) {
				for (auto& entity : entities) {
					auto pos = entity->GetPosition();
					pos.y += 0.05f;
					entity->SetPosition(pos);
				}

				{
					auto pos = ballEntity->GetPosition();
					pos.y += 0.05f;
					ballEntity->SetPosition(pos);
				}
			}
			if (window.IsKeyPressed(GLFW_KEY_RIGHT) || window.IsKeyPressed(GLFW_KEY_D)) {
				for (auto& entity : entities) {
					auto pos = entity->GetPosition();
					pos.y -= 0.05f;
					entity->SetPosition(pos);
				}

				{
					auto pos = ballEntity->GetPosition();
					pos.y -= 0.05f;
					ballEntity->SetPosition(pos);
				}
			}

			CreateCommandBuffers();
			DrawFrame();
		}
		vkDeviceWaitIdle(device.device());
	}

	void Game::CreatePipelineLayout() {
		CreateDescriptorSetLayout();
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(glm::mat4);
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	// Vertex input binding and attribute descriptions for the pipeline
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);
		/*
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, normal);
		*/
		return attributeDescriptions;
	}

	void Game::CreatePipeline() {
		auto pipelineConfig = Vk::Pipeline::DefaultPipelineConfigInfo(swapChain.width(), swapChain.height());
		pipelineConfig.renderPass = swapChain.getRenderPass();
		pipelineConfig.pipelineLayout = pipelineLayout;
		// Set up vertex input
		static auto bindingDescription = getBindingDescription();
		static auto attributeDescriptions = getAttributeDescriptions();
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		pipelineConfig.vertexInputInfo = vertexInputInfo;
		pipeline = std::make_unique<Vk::Pipeline>(
			device,
			"shader.vert.spv",
			"shader.frag.spv",
			pipelineConfig);
	}

	void Game::CreateCommandBuffers() {
		commandBuffers.resize(swapChain.imageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = device.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		for (int i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = swapChain.getRenderPass();
			renderPassInfo.framebuffer = swapChain.getFrameBuffer(i);

			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChain.getSwapChainExtent();

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			pipeline->bind(commandBuffers[i]);


			//
			// Draw all entities
			//
			for (auto& entity : entities) {
				glm::mat4 model = entity->GetModelMatrix();
				vkCmdPushConstants(commandBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
				entity->Bind(commandBuffers[i]);
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);
				entity->Draw(commandBuffers[i]);
			}


			//
			// Draw the ball
			//
			{
				glm::mat4 model = ballEntity->GetModelMatrix();
				vkCmdPushConstants(commandBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
				ballEntity->Bind(commandBuffers[i]);
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);
				ballEntity->Draw(commandBuffers[i]);
			}

			vkCmdEndRenderPass(commandBuffers[i]);
			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}
	}

	void Game::DrawFrame() {
		uint32_t imageIndex;
		auto result = swapChain.acquireNextImage(&imageIndex);
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image");
		}
		UpdateUniformBuffer(imageIndex);
		result = swapChain.submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}
	}

	void Game::CreateUniformBuffer() {
		//
		// Create UBO for camera
		//
		VkDeviceSize bufferSize = sizeof(CameraUbo);
		device.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			cameraUbo,
			cameraUboMemory);

		/*
		VkDeviceSize lightBufferSize = sizeof(LightBufferObject);
		device.createBuffer(
			lightBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			lightBuffer,
			lightBufferMemory);
		*/
	}

	void Game::UpdateUniformBuffer(uint32_t currentImage) {
		//
		// Update camera UBO
		//
		CameraUbo ubo{};
		glm::vec3 camPos = camera.GetPosition();
		glm::vec3 camTarget = camera.GetTarget();
		ubo.view = glm::lookAt(camPos, camTarget, glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), swapChain.extentAspectRatio(), 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;
		void* data;
		vkMapMemory(device.device(), cameraUboMemory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device.device(), cameraUboMemory);

		/*
		LightBufferObject light{};
		light.lightPos = glm::vec3(2.0f, 2.0f, 2.0f);
		light.viewPos = camPos;
		light.lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
		vkMapMemory(device.device(), lightBufferMemory, 0, sizeof(light), 0, &data);
		memcpy(data, &light, sizeof(light));
		vkUnmapMemory(device.device(), lightBufferMemory);
		*/
	}

	void Game::CreateDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		/*
		VkDescriptorSetLayoutBinding lightLayoutBinding{};
		lightLayoutBinding.binding = 1;
		lightLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		lightLayoutBinding.descriptorCount = 1;
		lightLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		lightLayoutBinding.pImmutableSamplers = nullptr;
		*/

		std::array<VkDescriptorSetLayoutBinding, 1> bindings = {
			uboLayoutBinding
			//lightLayoutBinding
		};
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device.device(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	void Game::CreateDescriptorPool() {
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 1;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[1].descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 1;

		if (vkCreateDescriptorPool(device.device(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	void Game::CreateDescriptorSet() {
		//
		// Create descriptor set for camera
		//
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(device.device(), &allocInfo, &cameraDescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor set!");
		}

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = cameraUbo;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(CameraUbo);

		/*
		VkDescriptorBufferInfo lightBufferInfo{};
		lightBufferInfo.buffer = lightBuffer;
		lightBufferInfo.offset = 0;
		lightBufferInfo.range = sizeof(LightBufferObject);

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = cameraDescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = cameraDescriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &lightBufferInfo;

		vkUpdateDescriptorSets(device.device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		*/

		VkWriteDescriptorSet descriptorWrite{};

		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = cameraDescriptorSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(device.device(), 1, &descriptorWrite, 0, nullptr);
	}
}

// TODO: Proper non-flat-color shaders for Block
// TODO: Setup light source

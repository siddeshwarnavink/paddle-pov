#include "Game.hpp"
#include "Block.hpp"
#include "Ball.hpp"
#include "GameEntity.hpp"
#include "GameCamera.hpp"
#include "PlayerPaddle.hpp"

#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <random>

#include <iostream>
#include <stdexcept>
#include <array>

namespace Paddle
{
	Game::Game()
		: window(WIDTH, HEIGHT, "Paddle POV"),
		device(window),
		swapChain(device, window.getExtent())
	{
		score = 0;
		CreateGameSounds();
		CreateDescriptorSetLayout();
		CreateUniformBuffer();
		CreateDescriptorPool();
		CreateFont();
		CreateDescriptorSet();
		CreatePipelineLayout();
		CreatePipeline();
		CreateFontPipelineLayout();
		CreateFontPipeline();
		CreateBlocks();
		CreateBall();
		CreatePaddle();
		CreateWalls();
		CreateCommandBuffers();
	}

	Game::~Game() {
		vkFreeCommandBuffers(device.device(), device.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		vkDestroyBuffer(device.device(), cameraUbo, nullptr);
		vkFreeMemory(device.device(), cameraUboMemory, nullptr);
		vkDestroyDescriptorPool(device.device(), descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device.device(), descriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
		if (fontPipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(device.device(), fontPipelineLayout, nullptr);
			fontPipelineLayout = VK_NULL_HANDLE;
		}
	}

	void Game::CreateGameSounds() {
		gameSounds = std::make_unique<GameSounds>();
		gameSounds->PlayBgm();
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
		// https://www.color-hex.com/color-palette/3811
		//
		std::vector<glm::vec3> colors = {
			{124.0f / 255.0f, 156.0f / 255.0f, 228.0f / 255.0f}, // #7c9ce4
			{100.0f / 255.0f, 230.0f / 255.0f, 215.0f / 255.0f}, // #64e6d7
			{231.0f / 255.0f, 235.0f / 255.0f, 185.0f / 255.0f}, // #e7ebb9
			{179.0f / 255.0f, 240.0f / 255.0f, 255.0f / 255.0f}, // #b3f0ff
			{30.0f / 255.0f, 151.0f / 255.0f, 158.0f / 255.0f},  // #1e979e
		};
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, static_cast<int>(colors.size()) - 1);

		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 8; ++j) {
				float x = startX + i * spacing;
				float y = startY + j * spacing;
				glm::vec3 color = colors[dis(gen)];
				blocks.emplace_back(std::make_unique<Block>(device, x, y, 0.0f, color));
			}
		}
	}

	void Game::CreateWalls() {
		const float sideOffset = 3.0f;

		auto leftWall = std::make_unique<Wall>(device, 6.0f, -sideOffset, 0.0f);
		wallEntities.emplace_back(std::move(leftWall));

		auto rightWall = std::make_unique<Wall>(device, 6.0f, sideOffset + 2.0f, 0.0f);
		wallEntities.emplace_back(std::move(rightWall));

		// Wall behind all the blocks
		auto frontWall = std::make_unique<Wall>(device, -4.0f, sideOffset, 0.0f);
		frontWall->SetRotation(glm::vec3(0, 0, 30));
		wallEntities.emplace_back(std::move(frontWall));
	}

	void Game::CreateFont() {
		font = std::make_unique<GameFont>(device);
		font->AddText("Score: 0");
		font->AddText("Game Over");
		font->CreateVertexBuffer();
	}

	bool CheckAABBSphereCollision(const glm::vec3& boxMin, const glm::vec3& boxMax, const glm::vec3& sphereCenter, float sphereRadius) {
		glm::vec3 closestPoint = glm::clamp(sphereCenter, boxMin, boxMax);
		float distanceSquared = glm::distance2(closestPoint, sphereCenter);
		return distanceSquared < (sphereRadius * sphereRadius);
	}

	void Game::UpdateAllEntitiesPosition(const glm::vec3& delta) {
		for (auto& block : blocks) {
			auto pos = block->GetPosition();
			block->SetPosition(pos + delta);
		}

		for (auto& entity : wallEntities) {
			auto pos = entity->GetPosition();
			entity->SetPosition(pos + delta);
		}

		{
			auto pos = ballEntity->GetPosition();
			ballEntity->SetPosition(pos + delta);
		}
	}

	void Game::ResetGame() {
		ResetBlocks();

		paddleEntity->SetPosition(glm::vec3(5.5f, 0.0f, 0.0f));
		ballEntity->SetPosition(glm::vec3(3.0f, 0.0f, 0.0f));

		score = 0;
		gameOver = false;
	}

	void Game::ResetBlocks() {
		for (auto& block : blocks)
			pendingDeleteBlocks.push_back(std::move(block));
		blocks.clear();
		CreateBlocks();
	}

	void Game::RenderScoreFont(std::string scoreText) {
		font->AddText(scoreText, -(float)swapChain.width() / 2.0f + 10.0f, (float)swapChain.height() / 2.0f - 40.0f, 0.5f, glm::vec3(1.0f));
	}

	void Game::RenderGameOverFont(std::string scoreText) {
		float scaleX = (float)swapChain.width() / WIDTH;
		float scaleY = (float)swapChain.height() / HEIGHT;

		font->AddText("Game Over", -280.0f * scaleX, -100.0f * scaleY, 2.0f, glm::vec3(1.0f, 0.2f, 0.2f));
		font->AddText(scoreText, -80.0f * scaleX, 150.0f * scaleY, 0.75f, glm::vec3(1.0f));
	}

	void Game::run() {
		bool prevF12Pressed = false;
		bool prevGameOver = false;
		int prevScore = -1;

		while (!window.ShouldClose()) {
			window.PollEvents();

			//
			// Toggle debug
			//
			bool f12Pressed = window.IsKeyPressed(GLFW_KEY_F12);
			if (f12Pressed && !prevF12Pressed)
				showDebug = !showDebug;
			prevF12Pressed = f12Pressed;

			//
			// Update ball
			//
			if (!gameOver)
				static_cast<Paddle::Ball*>(ballEntity.get())->Update(score);
			//
			// Block Collision
			//
			auto* ball = static_cast<Paddle::Ball*>(ballEntity.get());
			auto vel = ball->GetVelocity();
			const float ballRadius = ball->GetRadius();
			const glm::vec3 ballPos = ballEntity->GetPosition();

			if (ballPos.x > 6.0f)
				gameOver = true;

			for (auto it = blocks.begin(); it != blocks.end(); ) {
				(*it)->Update();
				glm::vec3 blockPos = (*it)->GetPosition();
				glm::vec3 blockMin = blockPos - glm::vec3(0.25f);
				glm::vec3 blockMax = blockPos + glm::vec3(0.25f);

				if ((*it)->IsExploded()) {
					pendingDeleteBlocks.push_back(std::move(*it));
					it = blocks.erase(it);
					continue;
				}
				else if (!(*it)->IsExplosionInitiated() && CheckAABBSphereCollision(blockMin, blockMax, ballPos, ballRadius)) {
					glm::vec3 closestPoint = glm::clamp(ballPos, blockMin, blockMax);
					glm::vec3 normal = glm::normalize(ballPos - closestPoint);

					if (glm::length(normal) < 0.0001f) {
						normal = glm::vec3(1, 0, 0);
					}

					vel = vel - 2.0f * glm::dot(vel, normal) * normal;
					ball->SetVelocity(vel);

					(*it)->InitExplosion();
					gameSounds->PlaySfx(SFX_BLOCK_EXPLOSION);

					score += 10;
				}
				else {
					++it;
				}
			}

			std::string scoreText = "Score: " + std::to_string(score);
			//
			// Reset Game
			//
			if (window.IsKeyPressed(GLFW_KEY_F11)) {
				ResetGame();
				font->ClearText();
				RenderScoreFont(scoreText);
				font->CreateVertexBuffer();
				prevScore = score;
				prevGameOver = gameOver;
				gameSounds->PlaySfx(SFX_BLOCKS_RESET);
				continue;
			}

			//
			// Paddle Collision
			//
			{
				glm::vec3 blockPos = paddleEntity->GetPosition();
				glm::vec3 blockMin = blockPos - glm::vec3(0.25f);
				glm::vec3 blockMax = blockPos + glm::vec3(0.25f);

				if (CheckAABBSphereCollision(blockMin, blockMax, ballPos, ballRadius)) {
					glm::vec3 closestPoint = glm::clamp(ballPos, blockMin, blockMax);
					glm::vec3 normal = glm::normalize(ballPos - closestPoint);

					if (glm::length(normal) < 0.0001f) {
						normal = glm::vec3(1, 0, 0);
					}

					vel = vel - 2.0f * glm::dot(vel, normal) * normal;
					ball->SetVelocity(vel);

					gameSounds->PlaySfx(SFX_PADDLE_BOUNCE);
				}

			}

			//
			// Wall collision
			//
			{
				for (size_t i = 0; i < wallEntities.size(); ++i) {
					auto& entity = wallEntities[i];
					glm::vec3 blockPos = entity->GetPosition();

					glm::vec3 halfExtents;
					const float wallLength = 10.0f;
					if (i < 2) // Left & Right wall
						halfExtents = glm::vec3(wallLength, 1.0f, 0.1f);
					else // Front wall
						halfExtents = glm::vec3(0.1f, wallLength, 1.0f);


					glm::vec3 blockMin = blockPos - halfExtents;
					glm::vec3 blockMax = blockPos + halfExtents;

					if (CheckAABBSphereCollision(blockMin, blockMax, ballPos, ballRadius)) {
						glm::vec3 closestPoint = glm::clamp(ballPos, blockMin, blockMax);
						glm::vec3 normal = glm::normalize(ballPos - closestPoint);

						if (glm::length(normal) < 0.0001f) {
							normal = glm::vec3(1, 0, 0);
						}

						vel = vel - 2.0f * glm::dot(vel, normal) * normal;
						ball->SetVelocity(vel);

						gameSounds->PlaySfx(SFX_WALL_BOUNCE);
					}
				}
			}

			//
			// Camera movement logic
			//
			if (!gameOver || showDebug) {
				if (showDebug) {
					if (window.IsKeyPressed(GLFW_KEY_UP) || window.IsKeyPressed(GLFW_KEY_W))
						UpdateAllEntitiesPosition(glm::vec3(0.05f, 0.0f, 0.0f));
					if (window.IsKeyPressed(GLFW_KEY_DOWN) || window.IsKeyPressed(GLFW_KEY_S))
						UpdateAllEntitiesPosition(glm::vec3(-0.05f, 0.0f, 0.0f));
				}

				if (window.IsKeyPressed(GLFW_KEY_LEFT) || window.IsKeyPressed(GLFW_KEY_A))
					UpdateAllEntitiesPosition(glm::vec3(0.0f, 0.05f, 0.0f));
				if (window.IsKeyPressed(GLFW_KEY_RIGHT) || window.IsKeyPressed(GLFW_KEY_D))
					UpdateAllEntitiesPosition(glm::vec3(0.0f, -0.05f, 0.0f));
			}

			//
			// Font rendering
			//
			font->ClearText();

			if (gameOver) {
				RenderGameOverFont(scoreText);

				//
				// Game over keybindings
				//
				if (window.IsKeyPressed(GLFW_KEY_SPACE)) {
					gameSounds->PlayBgm();
					gameSounds->PlaySfx(SFX_BLOCKS_RESET);

					ResetGame();
					font->ClearText();
					RenderScoreFont(scoreText);
					font->CreateVertexBuffer();
					prevScore = score;
					prevGameOver = gameOver;
					continue;
				}

			}
			else {
				RenderScoreFont(scoreText);
			}

			if (prevScore != score || prevGameOver != gameOver) {
				font->CreateVertexBuffer();
				if (prevGameOver != gameOver) {
					gameSounds->PauseBgm();
					gameSounds->PlaySfx(SFX_GAME_OVER);
				}
			}
			prevScore = score;
			prevGameOver = gameOver;

			CreateCommandBuffers();
			DrawFrame();

			if (blocks.empty())
				ResetBlocks();
		}
		vkDeviceWaitIdle(device.device());
		pendingDeleteBlocks.clear();
	}

	void Game::CreatePipelineLayout() {
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

	void Game::CreateFontPipelineLayout() {
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
		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &fontPipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create font pipeline layout!");
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

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, normal);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, uv);

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

	void Game::CreateFontPipeline() {
		auto pipelineConfig = Vk::Pipeline::DefaultPipelineConfigInfo(swapChain.width(), swapChain.height());
		pipelineConfig.renderPass = swapChain.getRenderPass();
		pipelineConfig.pipelineLayout = fontPipelineLayout;
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
		// Disable depth test/write for font
		pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
		pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
		// Enable alpha blending for font
		pipelineConfig.colorBlendAttachment.blendEnable = VK_TRUE;
		pipelineConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		pipelineConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		pipelineConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		pipelineConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		pipelineConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		pipelineConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		fontPipeline = std::make_unique<Vk::Pipeline>(
			device,
			"font.vert.spv",
			"font.frag.spv",
			pipelineConfig);
	}

	void Game::CreatePaddle() {
		paddleEntity = std::make_unique<PlayerPaddle>(device, 5.5f, 0.0f, 0.0f);
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

			// Draw all blocks
			for (auto& block : blocks)
				block->DrawBlock(commandBuffers[i], pipelineLayout, cameraDescriptorSet);

			//
			// Draw walls
			//
			if (showDebug) {
				for (auto& entity : wallEntities) {
					glm::mat4 model = entity->GetModelMatrix();
					vkCmdPushConstants(commandBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
					entity->Bind(commandBuffers[i]);
					vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);
					entity->Draw(commandBuffers[i]);
				}
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

			//
			// Font rendering
			//
			fontPipeline->bind(commandBuffers[i]);
			font->Bind(commandBuffers[i]);
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, fontPipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);
			glm::mat4 ortho = glm::ortho(
				-float(swapChain.width()) / 2.0f, float(swapChain.width()) / 2.0f,
				-float(swapChain.height()) / 2.0f, float(swapChain.height()) / 2.0f,
				-1.0f, 1.0f
			);
			vkCmdPushConstants(commandBuffers[i], fontPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &ortho);
			font->Draw(commandBuffers[i]);

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
	}

	void Game::CreateDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBinding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
			uboLayoutBinding, samplerLayoutBinding
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
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
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
		// Create descriptor set for camera and font sampler
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

		VkWriteDescriptorSet descriptorWriteUBO{};
		descriptorWriteUBO.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWriteUBO.dstSet = cameraDescriptorSet;
		descriptorWriteUBO.dstBinding = 0;
		descriptorWriteUBO.dstArrayElement = 0;
		descriptorWriteUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWriteUBO.descriptorCount = 1;
		descriptorWriteUBO.pBufferInfo = &bufferInfo;

		// Font image and sampler
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = font->GetFontImageView();
		imageInfo.sampler = font->GetFontSampler();

		VkWriteDescriptorSet descriptorWriteImage{};
		descriptorWriteImage.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWriteImage.dstSet = cameraDescriptorSet;
		descriptorWriteImage.dstBinding = 1;
		descriptorWriteImage.dstArrayElement = 0;
		descriptorWriteImage.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWriteImage.descriptorCount = 1;
		descriptorWriteImage.pImageInfo = &imageInfo;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = { descriptorWriteUBO, descriptorWriteImage };
		vkUpdateDescriptorSets(device.device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

// TODO: Double, Triple score bonus.
// TODO: Support full screen.
// TODO: More complex block explosion animation (small cube break into sub-pieces, particle effect, ...)
// TODO: Ball bounce particle effect.
// TODO: Cook up something for background (day-night cycle maybe?)
// TODO: Adding power-ups (extra life, paddle machine gun?)

// BUG: Sometimes ball passing through paddle and getting disappeared

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
	constexpr int DESIGNED_WIDTH = 1920;
	constexpr int DESIGNED_HEIGHT = 1080;
	constexpr float DESIGNED_ASPECT = float(DESIGNED_WIDTH) / float(DESIGNED_HEIGHT);

	void SetAspectViewport(VkCommandBuffer cmd, int fbWidth, int fbHeight) {
		float fbAspect = float(fbWidth) / float(fbHeight);
		int vpWidth, vpHeight, vpX, vpY;
		if (fbAspect > DESIGNED_ASPECT) {
			vpHeight = fbHeight;
			vpWidth = int(DESIGNED_ASPECT * fbHeight);
			vpX = (fbWidth - vpWidth) / 2;
			vpY = 0;
		}
		else {
			vpWidth = fbWidth;
			vpHeight = int(fbWidth / DESIGNED_ASPECT);
			vpX = 0;
			vpY = (fbHeight - vpHeight) / 2;
		}

		VkViewport viewport{};
		viewport.x = float(vpX);
		viewport.y = float(vpY);
		viewport.width = float(vpWidth);
		viewport.height = float(vpHeight);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);
	}

	Game::Game()
		: window(WIDTH, HEIGHT, "Paddle POV"),
		device(window),
		swapChain(device, window.getExtent())
	{
		CreateGameSounds();
		CreateDescriptorSetLayout();
		CreateUniformBuffer();
		CreateDescriptorPool();
		CreateDescriptorSet();
		CreatePipelineLayout();
		CreatePipeline();
		CreateGameFont();
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

		//for (int i = 0; i < 3; ++i) {
		for (int i = 0; i < 2; ++i) {
			//for (int j = 0; j < 8; ++j) {
			for (int j = 0; j < 2; ++j) {
				float x = startX + i * spacing;
				float y = startY + j * spacing;
				glm::vec3 color = colors[dis(gen)];
				blocks.emplace_back(std::make_unique<Block>(device, x, y, 0.0f, color));
			}
		}
	}

	void Game::CreateWalls() {
		const float wallLength = 10.0f;
		const float sideOffset = 3.0f;

		auto leftWall = std::make_unique<Wall>(device, 6.0f, -sideOffset, 0.0f, glm::vec3(wallLength, 1.0f, 0.1f));
		wallEntities.emplace_back(std::move(leftWall));

		auto rightWall = std::make_unique<Wall>(device, 6.0f, sideOffset + 2.0f, 0.0f, glm::vec3(wallLength, 1.0f, 0.1f));
		wallEntities.emplace_back(std::move(rightWall));

		// Wall behind all the blocks
		auto frontWall = std::make_unique<Wall>(device, -4.0f, sideOffset, 0.0f, glm::vec3(0.1f, wallLength, 1.0f));
		wallEntities.emplace_back(std::move(frontWall));
	}

	void Game::CreateGameFont() {
		font = std::make_unique<GameFont>(device, descriptorPool, descriptorSetLayout, cameraUbo, swapChain);
		font->AddText(FontFamily::FONT_FAMILY_TITLE, "Score: 0");
		font->AddText(FontFamily::FONT_FAMILY_TITLE, "Game Over");
		font->AddText(FontFamily::FONT_FAMILY_BODY, "Press Space to restart. Esc to exit.");
		font->CreateVertexBuffer();
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
		ResetEntities();
		score = 0;
		gameOver = false;
	}

	void Game::ResetEntities() {
		camera.SetPosition(glm::vec3(6.0f, 0.0f, 0.0f));

		paddleEntity->SetPosition(glm::vec3(5.5f, 0.0f, 0.0f));

		//
		// Rest ball
		auto* ball = static_cast<Paddle::Ball*>(ballEntity.get());
		ball->SetPosition(glm::vec3(3.0f, 0.0f, 0.0f));
		ball->SetVelocity(glm::vec3(-1.0f, 0.5f, 0.0f));

		//
		// Reset blocks
		//
		for (auto& block : blocks)
			pendingDeleteBlocks.push_back(std::move(block));
		blocks.clear();
		CreateBlocks();
	}

	void Game::RenderScoreFont(std::string scoreText) {
		float width = static_cast<float>(swapChain.width());
		float height = static_cast<float>(swapChain.height());

		float paddingX = 20.0f;
		float paddingY = isFullscreen ? 60.0f : 40.0f;

		float x = (-width / 2.0f) + paddingX;
		float y = (-height / 2.0f) + paddingY;

		const float fontSize = isFullscreen ? 1.0f : 0.5f;

		font->AddText(FontFamily::FONT_FAMILY_TITLE, scoreText, x, y, fontSize, glm::vec3(1.0f));
	}

	void Game::RenderGameOverFont(std::string scoreText) {
		float width = static_cast<float>(swapChain.width());
		float height = static_cast<float>(swapChain.height());

		float scaleX = width / 1080;
		float scaleY = height / 720;

		font->AddText(FontFamily::FONT_FAMILY_TITLE, "Game Over", -280.0f * scaleX, -100.0f * scaleY, 2.0f * scaleX, glm::vec3(1.0f, 0.2f, 0.2f));
		font->AddText(FontFamily::FONT_FAMILY_TITLE, scoreText, -80.0f * scaleX, 50.0f * scaleY, 0.75f * scaleX, glm::vec3(1.0f));
		font->AddText(FontFamily::FONT_FAMILY_BODY, "Press Space to restart. Esc to exit.", -153.0f * scaleX, 150.0f * scaleY, 0.25f * scaleX, glm::vec3(112.0f / 255.0f));
	}

	void Game::run() {
		bool prevF12Pressed = false;
		bool prevF10Pressed = false;
		bool prevGameOver = false;
		int prevScore = -1;

		glm::vec3 paddleMoveDelta = glm::vec3(0.0f);

		score = 0;
		isFullscreen = true;

		int windowedX = 100, windowedY = 100, windowedW = WIDTH, windowedH = HEIGHT;
		GLFWwindow* glfwWin = window.GetWindow();
		GLFWmonitor* mon = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(mon);

		glfwSetWindowMonitor(glfwWin, mon, 0, 0, mode->width, mode->height, mode->refreshRate);
		window.UpdateFramebufferSize();
		swapChain.recreate();
		CreatePipeline();
		font->CreatePipeline();
		CreateCommandBuffers();

		while (!window.ShouldClose()) {
			window.PollEvents();
			//
			// Fullscreen/windowed toggle
			//
			bool f10Pressed = window.IsKeyPressed(GLFW_KEY_F10);
			if (f10Pressed && !prevF10Pressed) {
				if (isFullscreen) {
					windowedW = WIDTH;
					windowedH = HEIGHT;
					windowedX = 100;
					windowedY = 100;
					glfwSetWindowMonitor(glfwWin, nullptr, windowedX, windowedY, windowedW, windowedH, 0);
					isFullscreen = false;
				}
				else {
					mon = glfwGetPrimaryMonitor();
					mode = glfwGetVideoMode(mon);
					glfwSetWindowMonitor(glfwWin, mon, 0, 0, mode->width, mode->height, mode->refreshRate);
					isFullscreen = true;
				}
				window.UpdateFramebufferSize();
				swapChain.recreate();
				CreatePipeline();
				font->CreatePipeline();
				CreateCommandBuffers();
			}

			//
			// Reset blocks
			//
			if (blocks.empty()) {
				ResetEntities();
				gameSounds->PlaySfx(SFX_BLOCKS_RESET);
			}

			//
			// Ball when behind paddle
			//
			if (ballEntity->GetPosition().x > 6.0f)
				gameOver = true;

			if (!gameOver)
				ballEntity->Update({ score });

			//
			// Block Collision
			//
			auto* ball = static_cast<Paddle::Ball*>(ballEntity.get());
			auto vel = ball->GetVelocity();
			const float ballRadius = ball->GetRadius();
			const glm::vec3 ballPos = ballEntity->GetPosition();

			for (auto it = blocks.begin(); it != blocks.end(); ) {
				auto block = (*it).get();

				block->Update();

				if (block->IsExploded()) {
					pendingDeleteBlocks.push_back(std::move(*it));
					it = blocks.erase(it);
				}
				else if (!block->IsExplosionInitiated() && ball->CheckCollision(block)) {
					ball->OnCollision(block);
					block->InitExplosion();
					gameSounds->PlaySfx(SFX_BLOCK_EXPLOSION);
					
					score += 10;
				}
				else ++it;
			}


			std::string scoreText = "Score: " + std::to_string(score);

			//
			// Paddle Collision
			//
			GameEntity* paddle = paddleEntity.get();
			if (ball->CheckCollision(paddle)) {
				ball->OnCollision(paddle);
				gameSounds->PlaySfx(SFX_PADDLE_BOUNCE);
			}

			//
			// Wall collision
			//
			for (size_t i = 0; i < wallEntities.size(); ++i) {
				auto& entity = wallEntities[i];

				if (ball->CheckCollision(entity.get())) {
					ball->OnCollision(entity.get());
					gameSounds->PlaySfx(SFX_WALL_BOUNCE);
				}
			}

			//
			// Camera movement logic
			//
			if (!gameOver) {
				glm::vec3 paddleDelta = glm::vec3(0.0f);

				if (window.IsKeyPressed(GLFW_KEY_LEFT) || window.IsKeyPressed(GLFW_KEY_A))
					paddleDelta = glm::vec3(0.0f, 0.05f, 0.0f);
				if (window.IsKeyPressed(GLFW_KEY_RIGHT) || window.IsKeyPressed(GLFW_KEY_D))
					paddleDelta = glm::vec3(0.0f, -0.05f, 0.0f);

				if (paddleDelta != glm::vec3(0.0f)) {
					UpdateAllEntitiesPosition(paddleDelta);

					bool willCollide = false;
					for (auto& entity : wallEntities) {
						if (paddleEntity->CheckCollision(entity.get())) {
							willCollide = true;
							break;
						}
					}

					if (willCollide) UpdateAllEntitiesPosition(-paddleDelta);
				}
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
				else if (window.IsKeyPressed(GLFW_KEY_ESCAPE)) {
					window.Close(); // TODO: Redirect to main menu once implemented.
				}
			}
			else {
				RenderScoreFont(scoreText);
			}

			font->CreateVertexBuffer();

			if (prevScore != score || prevGameOver != gameOver || prevF10Pressed != f10Pressed) {
				if (prevGameOver != gameOver) {
					gameSounds->PauseBgm();
					gameSounds->PlaySfx(SFX_GAME_OVER);
				}
			}
			prevScore = score;
			prevGameOver = gameOver;
			prevF10Pressed = f10Pressed;

			CreateCommandBuffers();
			DrawFrame();
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
			"Shader\\shader.vert.spv",
			"Shader\\shader.frag.spv",
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

			SetAspectViewport(commandBuffers[i], swapChain.width(), swapChain.height());
			pipeline->bind(commandBuffers[i]);

			// Draw all blocks
			for (auto& block : blocks)
				block->Draw(commandBuffers[i], pipelineLayout, cameraDescriptorSet);

			//
			// Draw the ball
			//
			ballEntity->Draw(commandBuffers[i], pipelineLayout, cameraDescriptorSet);

			//
			// Font rendering
			//
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
		{
			VkDeviceSize bufferSize = sizeof(CameraUbo);
			device.createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				cameraUbo,
				cameraUboMemory);
		}
	}

	void Game::UpdateUniformBuffer(uint32_t currentImage) {
		//
		// Update camera UBO
		//
		CameraUbo ubo{};
		glm::vec3 camPos = camera.GetPosition();
		glm::vec3 camTarget = camera.GetTarget();
		ubo.view = glm::lookAt(camPos, camTarget, glm::vec3(0.0f, 0.0f, 1.0f));
		float aspect = float(swapChain.width()) / float(swapChain.height());
		ubo.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;
		void* data;
		vkMapMemory(device.device(), cameraUboMemory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device.device(), cameraUboMemory);
	}

	void Game::CreateDescriptorSet() {
		//
		// Camera UBO descriptor set
		//
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(device.device(), &allocInfo, &cameraDescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate camera descriptor set!");
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

		vkUpdateDescriptorSets(device.device(), 1, &descriptorWriteUBO, 0, nullptr);
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
		const size_t numFonts = 2; // TODO: Need to find better way to get this count.
		const size_t numSets = 1 + numFonts;

		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(numSets);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(numSets);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(numSets);

		if (vkCreateDescriptorPool(device.device(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool");
		}
	}
}

// TODO: Double, Triple score bonus.
// TODO: More complex block explosion animation (small cube break into sub-pieces, particle effect, ...)
// TODO: Ball bounce particle effect.
// TODO: Cook up something for background (day-night cycle maybe?)
// TODO: Adding power-ups (extra life, paddle machine gun?)
// TODO: Blinking font effect support.

// BUG: Sometimes ball passing through paddle and getting disappeared

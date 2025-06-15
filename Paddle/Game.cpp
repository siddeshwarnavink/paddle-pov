#include "Game.hpp"
#include "GameSounds.hpp"
#include "GameCamera.hpp"
#include "GameFont.hpp"
#include "Utils.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

#include <random>
#include <stdexcept>
#include <array>
#include <ctime>
#include <cstdlib>

using Utils::DebugLog;
using Utils::DestroyPtrs;
using Utils::DestroyPtr;

namespace Paddle
{
	static constexpr int WIDTH  = 1080;
	static constexpr int HEIGHT = 720;

	static constexpr float PADDLE_SPEED = 0.05f;

	static constexpr float WALL_LENGTH      = 10.0f;
	static constexpr float WALL_SIDE_OFFSET = 3.0f;

	Game::Game() : window(WIDTH, HEIGHT, "Paddle POV")
	{
		device    = new Vk::Device(window);
		swapChain = new Vk::SwapChain(*device, window.getExtent());

		CreateDescriptorSetLayout();
		CreateUniformBuffer();
		CreateDescriptorPool();

		context = new GameContext(
			device,
			new GameSounds(),
			new GameFont(*device, descriptorPool, *swapChain),
			new GameCamera());

		CreateDescriptorSet();
		CreatePipelineLayout();
		CreatePipeline();
		CreateGameEntities();
		CreateGameFont();
	}

	Game::~Game() {
		DebugLog("Destroying game entities.");
		DestroyPtrs<Block> (blocks);
		DestroyPtrs<Loot>  (loots);
		DestroyPtrs<Wall>  (walls);

		DestroyPtr<Ball>         (ball);
		DestroyPtr<PlayerPaddle> (paddle);

		DebugLog("Destroying Vulkan resources.");
		vkFreeCommandBuffers(device->device(), device->getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		vkDestroyBuffer(device->device(), cameraUbo, nullptr);
		vkFreeMemory(device->device(), cameraUboMemory, nullptr);
		vkDestroyDescriptorPool(device->device(), descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device->device(), descriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(device->device(), pipelineLayout, nullptr);

		DebugLog("Destroying game resources.");
		DestroyPtr<GameSounds>  (context->gameSounds);
		DestroyPtr<GameCamera>  (context->camera);
		DestroyPtr<GameFont>    (context->font);
		DestroyPtr<GameContext> (context);

		DebugLog("Destroying Vulkan objects.");
		DestroyPtr<Vk::Pipeline>  (pipeline);
		DestroyPtr<Vk::SwapChain> (swapChain);
		DestroyPtr<Vk::Device>    (device);
	}

	void Game::CreateGameEntities() {
		Block::CreateBlocks(*context, blocks, loots);
		ball   = new Ball(*context);
		paddle = new PlayerPaddle(*context);

		walls[0] = new Wall(*context, 6.0f, -WALL_SIDE_OFFSET, 0.0f, glm::vec3(WALL_LENGTH, 1.0f, 0.1f));
		walls[1] = new Wall(*context, 6.0f, WALL_SIDE_OFFSET + 2.0f, 0.0f, glm::vec3(WALL_LENGTH, 1.0f, 0.1f));
		walls[2] = new Wall(*context, -4.0f, WALL_SIDE_OFFSET, 0.0f, glm::vec3(0.1f, WALL_LENGTH, 1.0f));
	}

	void Game::CreateGameFont() {
		context->font->AddText(FontFamily::FONT_FAMILY_TITLE, "Score: 0");
		context->font->AddText(FontFamily::FONT_FAMILY_TITLE, "Lives: 1");
		context->font->AddText(FontFamily::FONT_FAMILY_TITLE, "Game Over");
		context->font->AddText(FontFamily::FONT_FAMILY_BODY,  "Press Space to restart. Esc to exit.");
		context->font->AddText(FontFamily::FONT_FAMILY_BODY,  "Bonus +69");
		context->font->CreateVertexBuffer();
	}

	void EntityAddDeltaPos(GameEntity *entity, const glm::vec3& delta) {
		const auto pos = entity->GetPosition();
		entity->SetPosition(pos + delta);
	}

	void Game::UpdateAllEntitiesPosition(const glm::vec3& delta) {
		for(auto& block : blocks) EntityAddDeltaPos(block->AsEntity(), delta);
		for(auto& wall : walls)   EntityAddDeltaPos(wall->AsEntity(), delta);
		for(auto& loot : loots)   EntityAddDeltaPos(loot->AsEntity(), delta);
		EntityAddDeltaPos(ball->AsEntity(), delta);
	}

	void Game::ResetGame(uint64_t currentFrame) {
		DebugLog("Resetting game state.");

		ResetEntities(currentFrame);
		context->score = 0;
		context->gameOver = false;
	}

	template <typename T>
	void Game::UpdateDestructionQueue(std::vector<T*>& entities, uint64_t currentFrame, bool deleteAll) {
		static_assert(std::is_base_of<GameEntity, T>::value, "T must be a GameEntity");

		for(auto it = entities.begin(); it != entities.end(); ) {
			GameEntity* entity = *it;

			bool shouldDelete = entity->IsMarkedForDestruction();
			if(deleteAll) shouldDelete = true;

			if(shouldDelete) {
				DebugLog(std::string(typeid(*entity).name()) + " marked for destruction");
				PendingDestroyEntity pd{ entity->AsEntity(), currentFrame + MAX_FRAMES_IN_FLIGHT};
				destructionQueue.push_back(pd);
				it = entities.erase(it);
			}
			else ++it;
		}
	}

	void Game::ResetEntities(uint64_t currentFrame) {
		DebugLog("Resetting game entities.");

		context->camera->Reset();
		paddle->Reset();
		ball->Reset();
		for(auto& wall : walls) wall->Reset();

		UpdateDestructionQueue<Block>(blocks, currentFrame, true);
		UpdateDestructionQueue<Loot> (loots, currentFrame, true);

		Block::CreateBlocks(*context, blocks, loots);
	}

	void Game::RenderScoreFont(std::string scoreText,std::string livesText, std::string bonusText) {
		const float width  = static_cast<float>(swapChain->width());
		const float height = static_cast<float>(swapChain->height());

		{
			const float paddingX = 20.0f;
			const float paddingY = window.IsFullscreen() ? 60.0f : 40.0f;

			const float x = (-width / 2.0f)  + paddingX;
			const float y = (-height / 2.0f) + paddingY;
			const float fontSize = window.IsFullscreen() ? 1.0f : 0.5f;

			context->font->AddText(FontFamily::FONT_FAMILY_TITLE, scoreText, x, y, fontSize, glm::vec3(1.0f));
		}

		{
			const float paddingX = 20.0f;
			const float paddingY = window.IsFullscreen() ? 60.0f : 40.0f;

			const float x = (-width / 2.0f)  + paddingX;
			const float y = (-height / 2.3f) + paddingY;
			const float fontSize = window.IsFullscreen() ? 1.0f : 0.5f;

			context->font->AddText(FontFamily::FONT_FAMILY_TITLE, livesText, x, y, fontSize, glm::vec3(1.0f));
		}

		if(bonusText.length() > 0) {
			const float scaleX = width  / 1080;
			const float scaleY = height / 720;

			context->font->AddText(FontFamily::FONT_FAMILY_BODY, bonusText, -50.0f * scaleX, -200.0f * scaleY, 0.25f * scaleX, glm::vec3(1.0f));
		}
	}

	void Game::RenderGameOverFont(std::string scoreText) {
		const float width  = static_cast<float>(swapChain->width());
		const float height = static_cast<float>(swapChain->height());

		const float scaleX = width  / 1080;
		const float scaleY = height / 720;

		context->font->AddText(FontFamily::FONT_FAMILY_TITLE, "Game Over", -280.0f * scaleX, -100.0f * scaleY, 2.0f * scaleX, glm::vec3(1.0f, 0.2f, 0.2f));
		context->font->AddText(FontFamily::FONT_FAMILY_TITLE, scoreText, -80.0f * scaleX, 50.0f * scaleY, 0.75f * scaleX, glm::vec3(1.0f));
		context->font->AddText(FontFamily::FONT_FAMILY_BODY, "Press Space to restart. Esc to exit.", -153.0f * scaleX, 150.0f * scaleY, 0.25f * scaleX, glm::vec3(112.0f / 255.0f));
	}

	void Game::run() {
		srand(static_cast<unsigned int>(time(0)));
		uint64_t currentFrame = 0, absoluteFrameNumber = 0;

		context->gameSounds->PlayBgm();

		bool prevPlayPaddleCollisionSound = false;
		bool prevF10Pressed               = false;
		bool prevGameOver                 = false;
		int prevScore                     = -1;
		glm::vec3 paddleMoveDelta         = glm::vec3(0.0f);

		context->score = 0;

		bool waitingForBlockReset = false;
		time_t blockResetTime     = 0;

		time_t lastCollision    = time(NULL);
		time_t lastBonusMessage = time(NULL);
		uint32_t streak_count   = 0;
		std::string bonusText;
		std::string livesText;

		swapChain->recreate();
		CreatePipeline();
		context->font->CreatePipeline();
		CreateCommandBuffers();

		while (!window.ShouldClose()) {
			window.PollEvents();

			//
			// Cleanup pending destroys
			//
			currentFrame = absoluteFrameNumber % MAX_FRAMES_IN_FLIGHT;

			for(auto it = destructionQueue.begin(); it != destructionQueue.end(); ) {
				if(it->frameNumber <= absoluteFrameNumber) {
					DebugLog("Destroying entity: " + std::string(typeid(*it->entity).name()));
					delete it->entity;
					it = destructionQueue.erase(it);
				}
				else ++it;
			}

			++absoluteFrameNumber;

			// I know this takes 2 billion years to happen, but my
			// game is soo good that this edge case should be handled.
			if(absoluteFrameNumber > UINT64_MAX - 69420) {
				std::cerr << "You played for too long! Thank you for playing!" << std::endl;
				std::abort();
			}

			//
			// Fullscreen/windowed toggle
			//
			bool f10Pressed = window.IsKeyPressed(GLFW_KEY_F10);
			if (f10Pressed && !prevF10Pressed) {
				window.ToggleFullscreen();
				swapChain->recreate();
				CreatePipeline();
				context->font->CreatePipeline();
				CreateCommandBuffers();
			}

			if (waitingForBlockReset) {
				if (difftime(time(NULL), blockResetTime) >= 1) {
					ResetEntities(currentFrame);
					waitingForBlockReset = false;
				}
			}

			if (difftime(time(NULL), lastBonusMessage) > 3)
				bonusText = "";

			livesText = "Lives: " + std::to_string(context->lives);
			if(context->lives <= 1) livesText = "";

			//
			// Destroy uncollected loot
			//
			for (auto& loot : loots)
				if (loot->GetPosition().x > 6.0f) loot->MarkForDestruction();

			//
			// Game over logic
			//
			if (ball->GetPosition().x > 6.0f) {
				if(--context->lives >= 1) {
					ball->Reset();
					context->gameSounds->PlaySfx(SFX_BLOCKS_RESET);
				} else context->gameOver = true;
			}

			if (!context->gameOver) {
				ball->Update();
				for (auto& loot : loots) loot->Update();
			}

			//
			// Block Collision
			//
			bool didBlockCollide = false;
			for (auto& block : blocks) {
				block->Update();

				if (block->IsExploded()) {
					block->MarkForDestruction();

					//
					// Reset blocks
					//
					bool isAllBlocksBroken = true;
					for(auto& b : blocks) {
						if(!b->IsMarkedForDestruction() || !b->IsExplosionInitiated()) {
							isAllBlocksBroken = false;
							break;
						}
					}

					if(isAllBlocksBroken && !waitingForBlockReset) {
						waitingForBlockReset = true;
						blockResetTime = time(NULL);
						context->gameSounds->PlaySfx(SFX_BLOCKS_RESET);
					}
				}
				else if (!block->IsExplosionInitiated() && ball->CheckCollision(block)) {
					didBlockCollide = true;
					ball->OnCollision(block);
					block->InitExplosion();

					if (difftime(time(NULL), lastCollision) < 2) {
						++streak_count;
						DebugLog("Streak bonus: " + std::to_string(streak_count));
					}

					time(&lastCollision);

					context->gameSounds->PlaySfx(SFX_BLOCK_EXPLOSION);
					context->score += 10;
				}
			}

			//
			// Streak bonus
			//
			if(!didBlockCollide &&  streak_count > 0) {
				const int bonusScore = ++streak_count * 10;
				context->score += bonusScore;
				streak_count = 0;
				time(&lastBonusMessage);

				bonusText = "Bonus +" + std::to_string(bonusScore);
				context->gameSounds->PlaySfx(SFX_BONUS);
			}

			std::string scoreText = "Score: " + std::to_string(context->score);

			//
			// Paddle Collision
			//
			bool playPaddleCollisionSound = false;
			if (ball->CheckCollision(paddle->AsEntity())) {
				DebugLog("Ball collision with paddle detected.");
				ball->OnCollision(paddle->AsEntity());

				if(!prevPlayPaddleCollisionSound)
					context->gameSounds->PlaySfx(SFX_PADDLE_BOUNCE);
				playPaddleCollisionSound = true;
			}

			//
			// Loot Collision
			//
			for(auto& loot : loots) {
				if(loot->CheckCollision(paddle->AsEntity())) {
					DebugLog("Loot collision with paddle detected.");
					loot->OnCollision();
					loot->MarkForDestruction();
				}
			}

			//
			// Wall collision
			//
			for(auto& wall : walls) {
				if (ball->CheckCollision(wall)) {
					DebugLog("Ball collision with wall detected.");
					ball->OnCollision(wall);
					context->gameSounds->PlaySfx(SFX_WALL_BOUNCE);
				}
			}

			//
			// Camera movement logic
			//
			if (!context->gameOver) {
				glm::vec3 paddleDelta = glm::vec3(0.0f);

				if (window.IsKeyPressed(GLFW_KEY_LEFT) || window.IsKeyPressed(GLFW_KEY_A))
					paddleDelta = glm::vec3(0.0f, PADDLE_SPEED, 0.0f);
				if (window.IsKeyPressed(GLFW_KEY_RIGHT) || window.IsKeyPressed(GLFW_KEY_D))
					paddleDelta = glm::vec3(0.0f, -PADDLE_SPEED, 0.0f);

				if (paddleDelta != glm::vec3(0.0f)) {
					UpdateAllEntitiesPosition(paddleDelta);

					bool willCollide = false;
					for(auto& wall : walls) {
						if(paddle->CheckCollision(wall)) {
							DebugLog("Paddle collision with wall detected.");
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
			context->font->ClearText();
			if (context->gameOver) {
				RenderGameOverFont(scoreText);

				//
				// Game over keybindings
				//
				if (window.IsKeyPressed(GLFW_KEY_SPACE)) {
					context->gameSounds->PlayBgm();
					context->gameSounds->PlaySfx(SFX_BLOCKS_RESET);

					ResetGame(currentFrame);
					context->font->ClearText();
					RenderScoreFont(scoreText, livesText, bonusText);
					context->font->CreateVertexBuffer();
					prevScore = context->score;
					prevGameOver = context->gameOver;
					continue;
				}
				else if (window.IsKeyPressed(GLFW_KEY_ESCAPE)) {
					window.Close(); // TODO: Redirect to main menu once implemented.
				}
			}
			else {
				RenderScoreFont(scoreText, livesText, bonusText);
			}

			context->font->CreateVertexBuffer();

			if (prevScore != context->score || prevGameOver != context->gameOver || prevF10Pressed != f10Pressed) {
				if (prevGameOver != context->gameOver) {
					context->gameSounds->PauseBgm();
					context->gameSounds->PlaySfx(SFX_GAME_OVER);
				}
			}
			prevScore = context->score;
			prevGameOver = context->gameOver;
			prevF10Pressed = f10Pressed;
			prevPlayPaddleCollisionSound = playPaddleCollisionSound;

			//
			// Move entites to destruction queue
			//
			UpdateDestructionQueue<Block>(blocks, currentFrame);
			UpdateDestructionQueue<Loot> (loots,  currentFrame);

			CreateCommandBuffers();
			DrawFrame();
		}
		vkDeviceWaitIdle(device->device());
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

		if (vkCreatePipelineLayout(device->device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
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
		DestroyPtr<Vk::Pipeline>(pipeline);

		auto pipelineConfig = Vk::Pipeline::DefaultPipelineConfigInfo(swapChain->width(), swapChain->height());
		pipelineConfig.renderPass = swapChain->getRenderPass();
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
		pipeline = new Vk::Pipeline(
			*device,
			"Shader\\shader.vert.spv",
			"Shader\\shader.frag.spv",
			pipelineConfig);
	}

	void Game::CreateCommandBuffers() {
		commandBuffers.resize(swapChain->imageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = context->device->getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(device->device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers");
		}

		for (int i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer");
			}

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = swapChain->getRenderPass();
			renderPassInfo.framebuffer = swapChain->getFrameBuffer(i);

			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChain->getSwapChainExtent();

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
			for (auto& block : blocks)
				block->Draw(commandBuffers[i], pipelineLayout, cameraDescriptorSet);

			ball->Draw(commandBuffers[i], pipelineLayout, cameraDescriptorSet);

			for (auto& loot : loots)
				loot->Draw(commandBuffers[i], pipelineLayout, cameraDescriptorSet);

			context->font->Draw(commandBuffers[i]);


			vkCmdEndRenderPass(commandBuffers[i]);
			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer");
			}
		}
	}

	void Game::DrawFrame() {
		uint32_t imageIndex;
		auto result = swapChain->acquireNextImage(&imageIndex);
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image");
		}
		UpdateUniformBuffer(imageIndex);
		result = swapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image");
		}
	}

	void Game::CreateUniformBuffer() {
		const VkDeviceSize bufferSize = sizeof(CameraUbo);
		device->createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			cameraUbo,
			cameraUboMemory);
	}

	void Game::UpdateUniformBuffer(uint32_t currentImage) {
		CameraUbo ubo{};
		glm::vec3 camPos = context->camera->GetPosition();
		glm::vec3 camTarget = context->camera->GetTarget();
		ubo.view = glm::lookAt(camPos, camTarget, glm::vec3(0.0f, 0.0f, 1.0f));
		float aspect = float(swapChain->width()) / float(swapChain->height());
		ubo.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;
		void* data;
		vkMapMemory(device->device(), cameraUboMemory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device->device(), cameraUboMemory);
	}

	void Game::CreateDescriptorSet() {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(device->device(), &allocInfo, &cameraDescriptorSet) != VK_SUCCESS) {
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

		vkUpdateDescriptorSets(device->device(), 1, &descriptorWriteUBO, 0, nullptr);
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

		if (vkCreateDescriptorSetLayout(device->device(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
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

		if (vkCreateDescriptorPool(device->device(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool");
		}
	}
}

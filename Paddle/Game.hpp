#pragma once

#include "VkWindow.hpp"
#include "VkPipeline.hpp"
#include "VkSwapChain.hpp"
#include "Block.hpp"
#include "PlayerPaddle.hpp"
#include "Wall.hpp"
#include "Ball.hpp"
#include "GameFont.hpp"
#include "GameContext.hpp"

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Paddle {
	class Game {
	public:
		Game();
		~Game();

		Game(const Game&) = delete;
		Game& operator=(const Game&) = delete;

		void run();

	private:
		// === Initialization ===
		void CreateGameEntities();
		void CreateGameFont();
		void CreatePipelineLayout();
		void CreatePipeline();
		void CreateCommandBuffers();
		void CreateVertexBuffer();
		void CreateIndexBuffer();
		void CreateUniformBuffer();
		void CreateDescriptorSetLayout();
		void CreateDescriptorPool();
		void CreateDescriptorSet();

		// === Update / Logic ===
		void UpdateUniformBuffer(uint32_t currentImage);
		void UpdateAllEntitiesPosition(const glm::vec3& delta);
		void ResetGame();
		void ResetEntities();

		// === Rendering ===
		void DrawFrame();
		void RenderScoreFont(std::string scoreText, std::string bonusText);
		void RenderGameOverFont(std::string scoreText);

		// === Window & Vulkan Core ===
		Vk::Window window;
		Vk::Device device;
		Vk::SwapChain swapChain;
		std::unique_ptr<Vk::Pipeline> pipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;

		VkBuffer cameraUbo;
		VkDeviceMemory cameraUboMemory;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet cameraDescriptorSet;

		// === Game Components ===
		std::unique_ptr<GameContext> context;
		std::unique_ptr<Ball> ballEntity;
		std::unique_ptr<PlayerPaddle> paddleEntity;
		std::vector<std::unique_ptr<Wall>> wallEntities;
		std::vector<std::unique_ptr<Block>> blocks;
		std::vector<std::unique_ptr<Block>> pendingDeleteBlocks;
	};

}

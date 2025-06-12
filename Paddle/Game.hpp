#pragma once

#include "VkWindow.hpp"
#include "VkPipeline.hpp"
#include "VkSwapChain.hpp"
#include "VkDevice.hpp"
#include "Block.hpp"
#include "GameCamera.hpp"
#include "PlayerPaddle.hpp"
#include "Wall.hpp"
#include "GameFont.hpp"
#include "GameSounds.hpp"

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Paddle {
	class Game {
	public:
		Game();
		~Game();

		static constexpr int WIDTH = 1080;
		static constexpr int HEIGHT = 720;

		Game(const Game&) = delete;
		Game& operator=(const Game&) = delete;

		void run();

	private:
		void CreateGameSounds();
		void CreateBlocks();
		void CreateBall();
		void CreatePaddle();
		void CreateWalls();
		void CreateGameFont();
		void CreatePipelineLayout();
		void CreatePipeline();
		void CreateCommandBuffers();
		void DrawFrame();
		void CreateVertexBuffer();
		void CreateIndexBuffer();
		void CreateUniformBuffer();
		void CreateDescriptorSetLayout();
		void CreateDescriptorPool();
		void CreateDescriptorSet();
		void UpdateUniformBuffer(uint32_t currentImage);
		void UpdateAllEntitiesPosition(const glm::vec3& delta);
		void ResetGame();
		void ResetEntities();
		void RenderScoreFont(std::string scoreText);
		void RenderGameOverFont(std::string scoreText);

		bool isFullscreen;
		Vk::Window window{ WIDTH, HEIGHT, "Paddle Game" };
		Vk::Device device{ window };
		Vk::SwapChain swapChain{ device, window.getExtent() };
		std::unique_ptr<Vk::Pipeline> pipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;

		VkBuffer cameraUbo;
		VkDeviceMemory cameraUboMemory;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet cameraDescriptorSet;

		bool gameOver = false;

		std::unique_ptr<GameSounds> gameSounds;
		std::vector<std::unique_ptr<Block>> pendingDeleteBlocks;
		std::unique_ptr<GameEntity> ballEntity;
		std::unique_ptr<PlayerPaddle> paddleEntity;
		std::vector<std::unique_ptr<GameEntity>> wallEntities;
		std::vector<std::unique_ptr<Block>> blocks;
		std::unique_ptr<GameFont> font;
		GameCamera camera;

		int score;
	};
}

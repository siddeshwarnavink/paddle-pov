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

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Paddle {
	struct CameraUbo {
		glm::mat4 view;
		glm::mat4 proj;
	};

	class Game {
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		Game();
		~Game();

		Game(const Game&) = delete;
		Game& operator=(const Game&) = delete;

		void run();

	private:
		void CreateBlocks();
		void CreateBall();
		void CreatePaddle();
		void CreateWalls();
		void CreateFont();
		void CreateFontPipelineLayout();
		void CreateFontPipeline();
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
		void ResetBlocks();
		void RenderScoreFont(std::string scoreText);
		void RenderGameOverFont(std::string scoreText);

		Vk::Window window{ WIDTH, HEIGHT, "Paddle Game" };
		Vk::Device device{ window };
		Vk::SwapChain swapChain{ device, window.getExtent() };
		std::unique_ptr<Vk::Pipeline> pipeline;
		VkPipelineLayout pipelineLayout;
		std::unique_ptr<Vk::Pipeline> fontPipeline;
		VkPipelineLayout fontPipelineLayout = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> commandBuffers;

		VkBuffer cameraUbo;
		VkDeviceMemory cameraUboMemory;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet cameraDescriptorSet;

		bool showDebug = false;
		bool gameOver = false; 
		
		std::vector<std::unique_ptr<GameEntity>> pendingDeleteEntities;
		std::unique_ptr<GameEntity> ballEntity;
		std::unique_ptr<PlayerPaddle> paddleEntity;
		std::vector<std::unique_ptr<GameEntity>> wallEntities;
		std::vector<std::unique_ptr<GameEntity>> entities;
		std::unique_ptr<GameFont> font;
		GameCamera camera;
	
        int score;
	};
}

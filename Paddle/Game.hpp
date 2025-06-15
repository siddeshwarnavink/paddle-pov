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
#include "Loot.hpp"

#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Paddle {
	struct PendingDestroyEntity {
		GameEntity* entity = nullptr;
		uint64_t frameNumber;
	};

	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;


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
		void ResetGame(uint64_t currentFrame);
		void ResetEntities(uint64_t currentFrame);

		template <typename T>
		void UpdateDestructionQueue(std::vector<T*>& entities, uint64_t currentFrame, bool deleteAll);
		template <typename T>
		void UpdateDestructionQueue(std::vector<T*>& entities, uint64_t currentFrame) {
			UpdateDestructionQueue(entities, currentFrame, false);
		}

		// === Rendering ===
		void DrawFrame();
		void RenderScoreFont(std::string scoreText, std::string livesText, std::string bonusText);
		void RenderGameOverFont(std::string scoreText);

		// === Window & Vulkan Core ===
		Vk::Window window;
		Vk::Device* device;
		Vk::SwapChain* swapChain;
		Vk::Pipeline* pipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<PendingDestroyEntity> destructionQueue;

		VkBuffer cameraUbo;
		VkDeviceMemory cameraUboMemory;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet cameraDescriptorSet;

		// === Game Components ===
		GameContext* context;
		Ball* ball;
		PlayerPaddle* paddle;
		std::array<Wall*, 3> walls;
		std::vector<Block*> blocks;
		std::vector<Loot*> loots;
	};

}

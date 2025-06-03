#include "VkWindow.hpp"
#include "VkPipeline.hpp"
#include "VkSwapChain.hpp"
#include "VkDevice.hpp"
#include "Block.hpp"

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Paddle {
	struct UniformBufferObject {
		glm::mat4 model;
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

		Vk::Window window{ WIDTH, HEIGHT, "Paddle Game" };
		Vk::Device device{ window };
		Vk::SwapChain swapChain{ device, window.getExtent() };
		std::unique_ptr<Vk::Pipeline> pipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;

		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
		uint32_t indexCount = 0;

		VkBuffer uniformBuffer;
		VkDeviceMemory uniformBufferMemory;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet descriptorSet;

		Block block;
	};
}
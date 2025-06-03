#include "VkWindow.hpp"
#include "VkPipeline.hpp"
#include "VkSwapChain.hpp"
#include "VkDevice.hpp"

#include <memory>
#include <vector>

namespace Paddle {
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

		Vk::Window window{ WIDTH, HEIGHT, "Paddle Game" };
		Vk::Device device{ window };
		Vk::SwapChain swapChain{ device, window.getExtent() };
		std::unique_ptr<Vk::Pipeline> pipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;
	};
}
#include "VkWindow.hpp"

namespace Paddle {
	class Game {
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		void run();

	private:
		Vk::Window window{ WIDTH, HEIGHT, "Paddle Game" };
	};
}
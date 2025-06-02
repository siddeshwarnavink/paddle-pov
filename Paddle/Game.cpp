#include "Game.hpp"

namespace Paddle
{
	void Game::run() {
		while (!window.ShouldClose()) {
			window.PollEvents();
		}
	}
}
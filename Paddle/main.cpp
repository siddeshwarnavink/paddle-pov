#include "Game.hpp"

#include <iostream>

int main()
{
	Paddle::Game game;

	try {
		game.run();
	}
	catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

#pragma once 

#include "GameSounds.hpp"
#include "GameFont.hpp"
#include "GameCamera.hpp"

#include <memory>

struct GameContext {
	// === Vulkan ===
	Vk::Device* device;

	// === Game components ===
	std::unique_ptr<Paddle::GameSounds> gameSounds;
	std::unique_ptr<Paddle::GameFont> font;
	std::unique_ptr<Paddle::GameCamera> camera;

	// === Game state ===
	bool gameOver;
	int score;

	GameContext(Vk::Device* device,
		std::unique_ptr<Paddle::GameSounds> gameSounds,
		std::unique_ptr<Paddle::GameFont> font,
		std::unique_ptr<Paddle::GameCamera> camera,
		bool gameOver,
		int score)
		: device(std::move(device)),
		gameSounds(std::move(gameSounds)),
		font(std::move(font)),
		camera(std::move(camera)),
		gameOver(gameOver),
		score(score) {
	}
};

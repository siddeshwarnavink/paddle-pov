#pragma once 

#include "GameSounds.hpp"
#include "GameFont.hpp"
#include "GameCamera.hpp"

struct GameContext {
	// === Vulkan ===
	Vk::Device* device;

	// === Game components ===
	Paddle::GameSounds* gameSounds;
	Paddle::GameFont* font;
	Paddle::GameCamera* camera;

	// === Game state ===
	bool gameOver;
	int score;
	int lives;

	GameContext(Vk::Device* device,
		        Paddle::GameSounds* gameSounds,
		        Paddle::GameFont* font,
		        Paddle::GameCamera* camera)
		: device(device),
		  gameSounds(gameSounds),
		  font(font),
		  camera(camera),
		  gameOver(false),
		  score(0),
		  lives(1) { }
};

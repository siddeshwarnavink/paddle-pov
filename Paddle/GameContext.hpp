#pragma once 

#include <ctime>

#include "GameSounds.hpp"
#include "GameFont.hpp"
#include "GameCamera.hpp"
#include "FlashText.hpp"

struct GameContext {
	// === Vulkan ===
	Vk::Device* device;

	// === Game components ===
	Paddle::GameSounds* gameSounds;
	Paddle::GameFont* font;
	Paddle::GameCamera* camera;
        Paddle::FlashText* fm;

	// === Game state ===
	bool gameOver;
	int score;
	int lives;
        bool bulletMode;
        time_t bulletResetTime = 0;

	GameContext(Vk::Device* device,
		        Paddle::GameSounds* gameSounds,
		        Paddle::GameFont* font,
		        Paddle::GameCamera* camera,
                        Paddle::FlashText* fm)
		: device(device),
		  gameSounds(gameSounds),
		  font(font),
		  camera(camera),
                  fm(fm),
		  gameOver(false),
                  bulletMode(false),
		  score(0),
		  lives(1) { }
};

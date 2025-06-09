#include "GameSounds.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "Vendor\miniaudio.h"

#include <iostream>
#include <stdexcept>

namespace Paddle {
	const std::string prefix = "Assets\\Audio\\";

	GameSounds::GameSounds() {
		ma_result result;

		result = ma_engine_init(NULL, &engine);
		if (result != MA_SUCCESS) {
			throw std::runtime_error("failed to initialize sound");
		}

		const std::string bgmPath = prefix + "bg-music.wav";
		result = ma_sound_init_from_file(&engine, bgmPath.c_str(), 0, NULL, NULL, &bgm);
		if (result != MA_SUCCESS) {
			throw std::runtime_error("failed to load bg music");
		}
		ma_sound_set_volume(&bgm, 0.8f);
		ma_sound_set_looping(&bgm, MA_TRUE);
	}

	GameSounds::~GameSounds() {
		ma_sound_uninit(&bgm);
		ma_engine_uninit(&engine);
	}

	void GameSounds::PauseBgm() {
		ma_sound_stop(&bgm);
	}

	void GameSounds::PlayBgm() {
		ma_sound_start(&bgm);
	}

	void GameSounds::PlaySfx(GameSoundsSfx sfx) {
		std::string filename;

		switch (sfx) {
		case SFX_PADDLE_BOUNCE:
			filename = "paddle-bounce.mp3";
			break;
		case SFX_WALL_BOUNCE:
			filename = "wall-bounce.mp3";
			break;
		case SFX_BLOCKS_RESET:
			filename = "blocks-reset.wav";
			break;
		case SFX_GAME_OVER:
			filename = "game-over.wav";
			break;
		case SFX_BLOCK_EXPLOSION:
			filename = "block-explosion.wav";
			break;
		default:
			std::cerr << "unknown sound fx" << std::endl;
			filename = "wall-bounce.mp3";
			break;
		}

		std::string fullPath = prefix + filename;
		ma_engine_play_sound(&engine, fullPath.c_str(), NULL);
	}
}

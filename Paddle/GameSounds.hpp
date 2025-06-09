#pragma once

#include "Vendor\miniaudio.h"

namespace Paddle {
	enum GameSoundsSfx {
		SFX_PADDLE_BOUNCE = 0,
		SFX_WALL_BOUNCE,
		SFX_BLOCKS_RESET,
		SFX_GAME_OVER,
		SFX_BLOCK_EXPLOSION
	};

	class GameSounds {
	public:
		GameSounds();
		~GameSounds();

		void PlaySfx(GameSoundsSfx sfx);
		void PauseBgm();
		void PlayBgm();

	private:
		ma_engine engine;
		ma_sound bgm;
	};
}

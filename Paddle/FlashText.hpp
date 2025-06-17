#pragma once

#include <vector>
#include <string>
#include <ctime>

#include "GameFont.hpp"
#include "VkSwapChain.hpp"

namespace Paddle {
	struct FlashMessage {
		std::string text;
		time_t time;
		float timeout;
	};

	class FlashText {
	public:
		FlashText(GameFont& font, Vk::SwapChain& swapChain);

		FlashText(const FlashText&) = delete;
		FlashText& operator=(const FlashText&) = delete;

		void Update();
		void Draw();

		void Flash(std::string text, float timeout);
		void Flash(std::string text) { Flash(text, 3.0f); }

	private:
		GameFont& font;
		Vk::SwapChain& swapChain;
		std::vector<FlashMessage> messages;
	};
}

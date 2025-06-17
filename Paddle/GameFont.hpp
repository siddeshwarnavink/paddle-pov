#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "Vendor\stb_truetype.h"

#include "VkDevice.hpp"
#include "VkPipeline.hpp"
#include "VkSwapChain.hpp"
#include "GameVertex.hpp"

#include <vector>
#include <string>
#include <unordered_map>

namespace Paddle {
	enum class FontFamily {
		FONT_FAMILY_TITLE = 0,
		FONT_FAMILY_BODY,
	};

	struct FontFamilyHasher {
		std::size_t operator()(const FontFamily& k) const noexcept {
			return static_cast<std::size_t>(k);
		}
	};

	struct FontFamilyData {
		// --- Font metadata ---
		std::string filePath;
		stbtt_fontinfo fontInfo;
		stbtt_bakedchar bakedChars[96];
		unsigned char* bitmap = nullptr;
		int fontAscent = 0;

		// --- Staging buffer ---
		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

		// --- Vulkan resources for ---
		VkImage fontImage = VK_NULL_HANDLE;
		VkDeviceMemory fontImageMemory = VK_NULL_HANDLE;
		VkImageView fontImageView = VK_NULL_HANDLE;
		VkSampler fontSampler = VK_NULL_HANDLE;
		VkDescriptorSet fontDescriptorSets;
		VkBuffer vertexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
		std::vector<Vertex> verticesInstance;
	};


	class GameFont {
	public:
		GameFont(Vk::Device& device, VkDescriptorPool& descriptorPool, Vk::SwapChain& swapChain);
		~GameFont();

		GameFont(const GameFont&) = delete;
		GameFont& operator=(const GameFont&) = delete;

		void Draw(VkCommandBuffer commandBuffer);

		void AddText(FontFamily family, const std::string& text, float x, float y, float scale, glm::vec3 color);
		void AddText(FontFamily family, const std::string& text) {
			AddText(family, text, 0.0f, 0.0f, 1.0f, glm::vec3(1.0f));
		}

		void ClearText();
		void SetText(FontFamily family, const std::string& text, float x, float y, float scale, glm::vec3 color);

		void CreateVertexBuffer();
		void CreatePipeline();

	private:
		Vk::Device& device;
		VkDescriptorPool& descriptorPool;
		VkDescriptorSetLayout descriptorSetLayout;
		Vk::SwapChain& swapChain;
		Vk::Pipeline* fontPipeline = nullptr;
		VkPipelineLayout fontPipelineLayout = VK_NULL_HANDLE;

		int texWidth, texHeight;
		std::unordered_map<FontFamily, std::string, FontFamilyHasher> fontFilePath;
		std::unordered_map<FontFamily, FontFamilyData, FontFamilyHasher> fontsTable;

		void CreateFonts();
		void CreateFontBuffers();
		void CreatePipelineLayout();
		void CreateDescriptorSet();
	};
}

#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>
#include "Vendor\stb_truetype.h"

#include "VkDevice.hpp"
#include "GameEntity.hpp"

namespace Paddle {
	class GameFont {
	public:
		GameFont(Vk::Device& device);
		~GameFont();

		GameFont(const GameFont&) = delete;
		GameFont& operator=(const GameFont&) = delete;

		void Bind(VkCommandBuffer commandBuffer);
		void Draw(VkCommandBuffer commandBuffer);

		void AddText(const std::string& text, float x, float y, float scale, glm::vec3 color);
		void AddText(const std::string& text) {
			AddText(text, 0.0f, 0.0f, 1.0f, glm::vec3(1.0f));
		}


		void ClearText();
		void SetText(const std::string& text, float x, float y, float scale, glm::vec3 color);
		void CreateVertexBuffer();

		VkImageView GetFontImageView() const { return fontImageView; }
		VkSampler GetFontSampler() const { return fontSampler; }

	private:
		Vk::Device& device;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkDeviceMemory fontImageMemory;
		VkImageView fontImageView;
		VkSampler fontSampler;
		uint32_t vertexCount;

		std::vector<Vertex> verticesInstance;

		int texWidth, texHeight;
		stbtt_bakedchar bakedChars[96]; // ASCII 32..126
		GLuint fontTexture;
		VkImage fontImage;
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		static const std::vector<Vertex> vertices;
		static const std::vector<uint16_t> indices;

		unsigned char* bitmap;

		void CreateFont();
		void CreateFontBuffer();

		stbtt_fontinfo fontInfo;
		int fontAscent = 0;
	};
}

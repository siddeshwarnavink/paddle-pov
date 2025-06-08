#include "GameFont.hpp"
#include "VkDevice.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "Vendor\stb_truetype.h"

#include <cstring>
#include <stdexcept>

namespace Paddle {
	uint32_t vertexCount = 0;

	GameFont::GameFont(Vk::Device& device)
		: device(device) {
		texWidth = 512;
		texHeight = 512;
		CreateFont();
		CreateFontBuffer();
	}

	GameFont::~GameFont() {
		if (vertexBuffer) {
			vkDestroyBuffer(device.device(), vertexBuffer, nullptr);
			vertexBuffer = VK_NULL_HANDLE;
		}
		if (vertexBufferMemory) {
			vkFreeMemory(device.device(), vertexBufferMemory, nullptr);
			vertexBufferMemory = VK_NULL_HANDLE;
		}
		if (stagingBuffer) {
			vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
			stagingBuffer = VK_NULL_HANDLE;
		}
		if (stagingBufferMemory) {
			vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
			stagingBufferMemory = VK_NULL_HANDLE;
		}
		if (fontImageView) {
			vkDestroyImageView(device.device(), fontImageView, nullptr);
			fontImageView = VK_NULL_HANDLE;
		}
		if (fontImage) {
			vkDestroyImage(device.device(), fontImage, nullptr);
			fontImage = VK_NULL_HANDLE;
		}
		if (fontImageMemory) {
			vkFreeMemory(device.device(), fontImageMemory, nullptr);
			fontImageMemory = VK_NULL_HANDLE;
		}
		if (fontSampler) {
			vkDestroySampler(device.device(), fontSampler, nullptr);
			fontSampler = VK_NULL_HANDLE;
		}
		if (bitmap) {
			delete[] bitmap;
			bitmap = nullptr;
		}
	}

	void GameFont::CreateFont() {
		unsigned char* fontBuffer = new unsigned char[1 << 20];
		bitmap = new unsigned char[texWidth * texHeight];

		FILE* fontFile = nullptr;
		if (fopen_s(&fontFile, "Assets\\HennyPenny-Regular.ttf", "rb") != 0 || !fontFile) {
			throw std::runtime_error("Failed to open font file!");
		}

		fread(fontBuffer, 1, 1 << 20, fontFile);
		fclose(fontFile);

		if (!stbtt_InitFont(&fontInfo, fontBuffer, stbtt_GetFontOffsetForIndex(fontBuffer, 0))) {
			throw std::runtime_error("Failed to init font info!");
		}

		int ascent, descent, lineGap;
		stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
		fontAscent = ascent;

		stbtt_BakeFontBitmap(fontBuffer, 0, 96.0, bitmap, texWidth, texHeight, 32, 96, bakedChars);
		delete[] fontBuffer;
	}

	void GameFont::CreateFontBuffer() {
		VkDeviceSize imageSize = texWidth * texHeight;

		device.createBuffer(
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		void* data;
		vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, bitmap, static_cast<size_t>(imageSize));
		vkUnmapMemory(device.device(), stagingBufferMemory);

		device.createImage(
			texWidth, texHeight,
			VK_FORMAT_R8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			fontImage,
			fontImageMemory
		);

		device.transitionImageLayout(fontImage, VK_FORMAT_R8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		device.copyBufferToImage(stagingBuffer, fontImage, texWidth, texHeight);
		device.transitionImageLayout(fontImage, VK_FORMAT_R8_UNORM,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = fontImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8_UNORM;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device.device(), &viewInfo, nullptr, &fontImageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create font image view!");
		}

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &fontSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create font sampler!");
		}
	}


	void GameFont::CreateVertexBuffer() {
		VkDeviceSize bufferSize = sizeof(verticesInstance[0]) * verticesInstance.size();
		device.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBuffer,
			vertexBufferMemory);
		void* data;
		vkMapMemory(device.device(), vertexBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, verticesInstance.data(), (size_t)bufferSize);
		vkUnmapMemory(device.device(), vertexBufferMemory);
	}


	void GameFont::AddText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
		float startX = x;
		float startY = y;
		float posX = x;
		float posY = y;
		for (char c : text) {
			if (c < 32 || c >= 128)
				continue;
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(bakedChars, texWidth, texHeight, c - 32, &posX, &posY, &q, 1);

			float x0 = startX + (q.x0 - x) * scale;
			float y0 = startY + (q.y0 - y) * scale;
			float x1 = startX + (q.x1 - x) * scale;
			float y1 = startY + (q.y1 - y) * scale;
			glm::vec3 normal = { 0.0f, 0.0f, 1.0f };

			// First triangle
			verticesInstance.push_back({ {x0, y0, 0.0f}, color, normal, {q.s0, q.t0} });
			verticesInstance.push_back({ {x1, y0, 0.0f}, color, normal, {q.s1, q.t0} });
			verticesInstance.push_back({ {x0, y1, 0.0f}, color, normal, {q.s0, q.t1} });

			// Second triangle
			verticesInstance.push_back({ {x0, y1, 0.0f}, color, normal, {q.s0, q.t1} });
			verticesInstance.push_back({ {x1, y0, 0.0f}, color, normal, {q.s1, q.t0} });
			verticesInstance.push_back({ {x1, y1, 0.0f}, color, normal, {q.s1, q.t1} });
		}
		vertexCount = static_cast<uint32_t>(verticesInstance.size());
	}

	void GameFont::ClearText() {
		verticesInstance.clear();
		vertexCount = 0;
	}

	void GameFont::SetText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
		ClearText();
		AddText(text, x, y, scale, color);
		CreateVertexBuffer();
	}

	void GameFont::Bind(VkCommandBuffer commandBuffer) {
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	}


	void GameFont::Draw(VkCommandBuffer commandBuffer) {
		vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
	}
}

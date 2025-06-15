#include "GameFont.hpp"
#include "GameCamera.hpp"
#include "VkDevice.hpp"
#include "VkPipeline.hpp"
#include "Utils.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "Vendor\stb_truetype.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <functional>
#include <array>

using Utils::DestroyPtr;
using Utils::DebugLog;

namespace Paddle {
	GameFont::GameFont(Vk::Device& device, VkDescriptorPool& descriptorPool, Vk::SwapChain& swapChain)
		: device(device), descriptorPool(descriptorPool), swapChain(swapChain) {
		texWidth = 512;
		texHeight = 512;

		fontFilePath[FontFamily::FONT_FAMILY_TITLE] = "Assets\\Font\\HennyPenny-Regular.ttf";
		fontFilePath[FontFamily::FONT_FAMILY_BODY] = "Assets\\Font\\ZillaSlab-Regular.ttf";

		CreateFonts();
		CreateFontBuffers();
		CreateDescriptorSet();
		CreatePipelineLayout();
	}

	GameFont::~GameFont() {
		DebugLog("Destroying GameFont resources.");

		for (auto it = fontFilePath.begin(); it != fontFilePath.end(); ++it) {
			FontFamilyData font = fontsTable[(*it).first];
			if (font.vertexBuffer != VK_NULL_HANDLE)
				vkDestroyBuffer(device.device(), font.vertexBuffer, nullptr);
			else DebugLog("Font vertex buffer is null, skipping destruction.");

			if (font.vertexBufferMemory != VK_NULL_HANDLE)
				vkFreeMemory(device.device(), font.vertexBufferMemory, nullptr);
			else DebugLog("Font vertex buffer memory is null, skipping destruction.");

			if (font.stagingBuffer != VK_NULL_HANDLE)
				vkDestroyBuffer(device.device(), font.stagingBuffer, nullptr);
			else DebugLog("Font staging buffer is null, skipping destruction.");

			if (font.stagingBufferMemory != VK_NULL_HANDLE)
				vkFreeMemory(device.device(), font.stagingBufferMemory, nullptr);
			else DebugLog("Font staging buffer memory is null, skipping destruction.");

			if (font.fontImageView != VK_NULL_HANDLE)
				vkDestroyImageView(device.device(), font.fontImageView, nullptr);
			else DebugLog("Font image view is null, skipping destruction.");

			if (font.fontImage != VK_NULL_HANDLE)
				vkDestroyImage(device.device(), font.fontImage, nullptr);
			else DebugLog("Font image is null, skipping destruction.");

			if (font.fontImageMemory != VK_NULL_HANDLE)
				vkFreeMemory(device.device(), font.fontImageMemory, nullptr);
			else DebugLog("Font image memory is null, skipping destruction.");

			if (font.fontSampler != VK_NULL_HANDLE)
				vkDestroySampler(device.device(), font.fontSampler, nullptr);
			else DebugLog("Font sampler is null, skipping destruction.");

			delete[] font.bitmap;
		}
		if (fontPipelineLayout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(device.device(), fontPipelineLayout, nullptr);
		if (descriptorSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(device.device(), descriptorSetLayout, nullptr);
		DestroyPtr(fontPipeline);
	}

	void GameFont::CreateFonts() {
		for (auto it = fontFilePath.begin(); it != fontFilePath.end(); ++it) {
			unsigned char* fontBuffer = new unsigned char[1 << 20];
			fontsTable[(*it).first].bitmap = new unsigned char[texWidth * texHeight];

			FILE* fontFile = nullptr;
			if (fopen_s(&fontFile, (*it).second.c_str(), "rb") != 0 || !fontFile) {
				throw std::runtime_error("Failed to open font file " + (*it).second);
			}

			fread(fontBuffer, 1, 1 << 20, fontFile);
			fclose(fontFile);

			if (!stbtt_InitFont(&fontsTable[(*it).first].fontInfo, fontBuffer, stbtt_GetFontOffsetForIndex(fontBuffer, 0))) {
				throw std::runtime_error("Failed to init font info!");
			}

			int ascent, descent, lineGap;
			stbtt_GetFontVMetrics(&fontsTable[(*it).first].fontInfo, &ascent, &descent, &lineGap);
			fontsTable[(*it).first].fontAscent = ascent;

			stbtt_BakeFontBitmap(fontBuffer, 0, 96.0, fontsTable[(*it).first].bitmap, texWidth, texHeight, 32, 96, fontsTable[(*it).first].bakedChars);
			delete[] fontBuffer;
		}
	}

	void GameFont::CreateFontBuffers() {
		VkDeviceSize imageSize = texWidth * texHeight;

		for (auto it = fontFilePath.begin(); it != fontFilePath.end(); ++it) {
			auto& font = fontsTable[(*it).first];

			device.createBuffer(
				imageSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				font.stagingBuffer,
				font.stagingBufferMemory);
			device.SetObjectName((uint64_t)font.stagingBuffer, VK_OBJECT_TYPE_BUFFER, (*it).second + " Font Staging Buffer");

			void* data;
			vkMapMemory(device.device(), font.stagingBufferMemory, 0, imageSize, 0, &data);
			memcpy(data, font.bitmap, static_cast<size_t>(imageSize));
			vkUnmapMemory(device.device(), font.stagingBufferMemory);

			device.createImage(
				texWidth, texHeight,
				VK_FORMAT_R8_UNORM,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				font.fontImage,
				font.fontImageMemory);

			device.transitionImageLayout(font.fontImage, VK_FORMAT_R8_UNORM,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			device.copyBufferToImage(font.stagingBuffer, font.fontImage, texWidth, texHeight);
			device.transitionImageLayout(font.fontImage, VK_FORMAT_R8_UNORM,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = font.fontImage;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = VK_FORMAT_R8_UNORM;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device.device(), &viewInfo, nullptr, &font.fontImageView) != VK_SUCCESS) {
				throw std::runtime_error("failed to create " + (*it).second + " font image view!");
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

			if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &font.fontSampler) != VK_SUCCESS) {
				throw std::runtime_error("failed to create font sampler!");
			}
		}
	}


	void GameFont::CreateVertexBuffer() {
		for (auto it = fontFilePath.begin(); it != fontFilePath.end(); ++it) {
			auto& font = fontsTable[(*it).first];

			vkDeviceWaitIdle(device.device());
			if (font.vertexBuffer != VK_NULL_HANDLE) {
				vkDestroyBuffer(device.device(), font.vertexBuffer, nullptr);
				font.vertexBuffer = VK_NULL_HANDLE;
			}
			if (font.vertexBufferMemory != VK_NULL_HANDLE) {
				vkFreeMemory(device.device(), font.vertexBufferMemory, nullptr);
				font.vertexBufferMemory = VK_NULL_HANDLE;
			}

			VkDeviceSize bufferSize = sizeof(font.verticesInstance[0]) * font.verticesInstance.size();
			if (bufferSize == 0) continue;

			device.createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				font.vertexBuffer,
				font.vertexBufferMemory);
			device.SetObjectName((uint64_t)font.vertexBuffer, VK_OBJECT_TYPE_BUFFER, "Font Vertex Buffer");
			void* data;
			vkMapMemory(device.device(), font.vertexBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, font.verticesInstance.data(), (size_t)bufferSize);
			vkUnmapMemory(device.device(), font.vertexBufferMemory);
		}
	}


	void GameFont::AddText(FontFamily family, const std::string& text, float x, float y, float scale, glm::vec3 color) {
		auto& font = fontsTable[family];

		float startX = x;
		float startY = y;
		float posX = x;
		float posY = y;
		for (char c : text) {
			if (c < 32 || c >= 128)
				continue;
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(font.bakedChars, texWidth, texHeight, c - 32, &posX, &posY, &q, 1);

			float x0 = startX + (q.x0 - x) * scale;
			float y0 = startY + (q.y0 - y) * scale;
			float x1 = startX + (q.x1 - x) * scale;
			float y1 = startY + (q.y1 - y) * scale;
			glm::vec3 normal = { 0.0f, 0.0f, 1.0f };

			// First triangle
			font.verticesInstance.push_back({ {x0, y0, 0.0f}, color, normal, {q.s0, q.t0} });
			font.verticesInstance.push_back({ {x1, y0, 0.0f}, color, normal, {q.s1, q.t0} });
			font.verticesInstance.push_back({ {x0, y1, 0.0f}, color, normal, {q.s0, q.t1} });

			// Second triangle
			font.verticesInstance.push_back({ {x0, y1, 0.0f}, color, normal, {q.s0, q.t1} });
			font.verticesInstance.push_back({ {x1, y0, 0.0f}, color, normal, {q.s1, q.t0} });
			font.verticesInstance.push_back({ {x1, y1, 0.0f}, color, normal, {q.s1, q.t1} });
		}
	}

	void GameFont::ClearText() {
		for (auto it = fontFilePath.begin(); it != fontFilePath.end(); ++it) {
			auto& font = fontsTable[(*it).first];
			font.verticesInstance.clear();
		}
	}

	void GameFont::SetText(FontFamily family, const std::string& text, float x, float y, float scale, glm::vec3 color) {
		ClearText();
		AddText(family, text, x, y, scale, color);
		CreateVertexBuffer();
	}

	void GameFont::CreateDescriptorSet() {
		//
		// Descriptor set layout
		//
		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &samplerLayoutBinding;

		if (vkCreateDescriptorSetLayout(device.device(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create font descriptor set layout!");
		}

		const auto& fontFamilies = fontsTable;
		for (const auto& kv : fontFamilies) {
			FontFamily family = kv.first;
			VkDescriptorSet fontDescriptorSet;

			VkDescriptorSetAllocateInfo fontAllocInfo{};
			fontAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			fontAllocInfo.descriptorPool = descriptorPool;
			fontAllocInfo.descriptorSetCount = 1;
			fontAllocInfo.pSetLayouts = &descriptorSetLayout;

			if (vkAllocateDescriptorSets(device.device(), &fontAllocInfo, &fontDescriptorSet) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate font descriptor set!");
			}

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = fontsTable[family].fontImageView;
			imageInfo.sampler = fontsTable[family].fontSampler;

			VkWriteDescriptorSet descriptorWriteImage{};
			descriptorWriteImage.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWriteImage.dstSet = fontDescriptorSet;
			descriptorWriteImage.dstBinding = 1;
			descriptorWriteImage.dstArrayElement = 0;
			descriptorWriteImage.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWriteImage.descriptorCount = 1;
			descriptorWriteImage.pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(device.device(), 1, &descriptorWriteImage, 0, nullptr);

			fontsTable[family].fontDescriptorSets = fontDescriptorSet;
		}
	}

	void GameFont::CreatePipelineLayout() {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(glm::mat4);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &fontPipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create font pipeline layout!");
		}
	}

	static VkVertexInputBindingDescription getFontBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 4> getFontAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, normal);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, uv);

		return attributeDescriptions;
	}

	void GameFont::CreatePipeline() {
		DestroyPtr<Vk::Pipeline>(fontPipeline);

		auto pipelineConfig = Vk::Pipeline::DefaultPipelineConfigInfo(swapChain.width(), swapChain.height());
		pipelineConfig.renderPass = swapChain.getRenderPass();
		pipelineConfig.pipelineLayout = fontPipelineLayout;

		static auto bindingDescription = getFontBindingDescription();
		static auto attributeDescriptions = getFontAttributeDescriptions();
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		pipelineConfig.vertexInputInfo = vertexInputInfo;

		fontPipeline = new Vk::Pipeline(
			device,
			"Shader\\font.vert.spv",
			"Shader\\font.frag.spv",
			pipelineConfig);
	}

	void GameFont::Draw(VkCommandBuffer commandBuffer) {
		float orthoLeft = -static_cast<float>(swapChain.width()) / 2.0f;
		float orthoRight = static_cast<float>(swapChain.width()) / 2.0f;
		float orthoBottom = -static_cast<float>(swapChain.height()) / 2.0f;
		float orthoTop = static_cast<float>(swapChain.height()) / 2.0f;
		glm::mat4 ortho = glm::ortho(
			orthoLeft, orthoRight,
			orthoBottom, orthoTop,
			-1.0f, 1.0f
		);

		fontPipeline->bind(commandBuffer);

		const auto& fontFamilies = fontsTable;
		for (const auto& kv : fontFamilies) {
			FontFamily family = kv.first;

			const auto& vertices = fontsTable[family].verticesInstance;
			const auto vertexBuffer = fontsTable[family].vertexBuffer;
			const auto& descriptorSets = fontsTable[family].fontDescriptorSets;
			if (vertices.empty()) continue;

			const VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fontPipelineLayout, 0, 1, &descriptorSets, 0, nullptr);

			vkCmdPushConstants(commandBuffer, fontPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &ortho);

			vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
		}
	}
}

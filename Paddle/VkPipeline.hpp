#pragma once

//#include <vulkan/vulkan.h>

#include "VkDevice.hpp"
#include <string>
#include <vector>

namespace Vk {
	struct PipelineConfigInfo {
        VkViewport viewport;
        VkRect2D scissor;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;
    };

    class Pipeline {
    public:
        Pipeline(Device& device, const std::string vertFilePath, const std::string fragFilePath, const PipelineConfigInfo& configInfo);
		~Pipeline();

        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        void bind(VkCommandBuffer commandBuffer);

        static PipelineConfigInfo DefaultPipelineConfigInfo(uint32_t width, uint32_t height);

    private:
        Device& device;
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;

        static std::vector<char> ReadFile(const std::string& filePath);
        void CreateGraphicsPipeline(const std::string vertFilePath, const std::string fragFilePath, const PipelineConfigInfo& configInfo);
        void CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
    };
}
#pragma once

#include <vulkan/vulkan.h>
#include "ShaderProgramContext.h"
#include "Texture.h"
#include "Framebuffer.h"
#include <string>
#include "PipelineBuilder.h"

namespace VGM
{
	class ShaderProgram
	{
	public:
		ShaderProgram() = default;
		ShaderProgram(const std::vector<std::pair<std::string, VkShaderStageFlagBits>>& shaderSources, VkPipelineLayout layout, PipelineConfigurator& pipelineConfigurator, VkDevice device, uint32_t SamplerCount);
		const std::vector<VkSampler>& getSamplers();
		void cmdBind(VkCommandBuffer commandBuffer);
		void destroy(VkDevice device);

	private:
		VkResult createShaderModule(const std::string& shaderSource, VkShaderModule& shaderModule, const VkDevice device);
		void createSamplers(VkDevice device);
		VkPipeline _pipeline;

		std::vector<VkSampler> _samplers;
	};
}
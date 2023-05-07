#include "Shader.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include "VulkanErrorHandling.h"

VGM::ShaderProgram::ShaderProgram(const std::vector<std::pair<std::string, VkShaderStageFlagBits>>& shaderSources, VkPipelineLayout layout, PipelineConfigurator& pipelineConfigurator, VkDevice device, uint32_t SamplerCount)
{
	VkGraphicsPipelineCreateInfo createInfo = pipelineConfigurator.getPipelineConfiguration();
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	for(unsigned int i = 0; i<shaderSources.size(); i++)
	{
		VkPipelineShaderStageCreateInfo shaderStage = {};
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.pName = "main";
		shaderStage.stage = shaderSources[i].second;

		createShaderModule(shaderSources[i].first, shaderStage.module, device);
		
		shaderStages.push_back(shaderStage);
	}
	
	createInfo.pStages = shaderStages.data();
	createInfo.stageCount = shaderStages.size();
	createInfo.layout = layout;
	createInfo.flags = 0;

	VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &_pipeline));

	for(auto& m : shaderStages)
	{
		vkDestroyShaderModule(device, m.module, nullptr);
	}

	_samplers.resize(SamplerCount);
	createSamplers(device);
}

const std::vector<VkSampler>& VGM::ShaderProgram::getSamplers()
{
	return _samplers;
}

void VGM::ShaderProgram::cmdBind(VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
}

void VGM::ShaderProgram::destroy(VkDevice device)
{
	vkDestroyPipeline(device, _pipeline, nullptr);
}

VkResult VGM::ShaderProgram::createShaderModule(const std::string& shaderSource, VkShaderModule& shaderModule, const VkDevice device)
{
	std::ifstream shaderStream(shaderSource, std::ios::ate | std::ios::binary);

	if(shaderStream.is_open())
	{
		size_t filesize = static_cast<size_t>(shaderStream.tellg());
		std::vector<uint32_t> programBuffer(filesize / sizeof(uint32_t));
		shaderStream.seekg(0);
		uint32_t* ptr = programBuffer.data();
		shaderStream.read((char*)ptr, filesize);

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.pCode = programBuffer.data();
		createInfo.codeSize = programBuffer.size() * sizeof(uint32_t);
		createInfo.flags = 0;

		return vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
	}
	return VK_ERROR_INITIALIZATION_FAILED;
}

void VGM::ShaderProgram::createSamplers(VkDevice device)
{
	VkSamplerCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.anisotropyEnable = VK_FALSE;
	createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	createInfo.compareEnable = VK_FALSE;
	createInfo.flags = 0;
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.maxAnisotropy = 16.0f;
	createInfo.maxLod = 100.0f;
	createInfo.minLod = 0;
	createInfo.mipLodBias = 0;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.unnormalizedCoordinates = VK_FALSE;

	for(auto& s : _samplers)
	{
		vkCreateSampler(device, &createInfo, nullptr, &s);
	}
}

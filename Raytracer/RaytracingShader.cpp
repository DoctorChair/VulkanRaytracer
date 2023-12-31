#include "RaytracingShader.h"
#include "Utils.h"
#include <stdexcept>

RaytracingShader::RaytracingShader(std::vector<std::pair<std::string, VkShaderStageFlagBits>>& shaderSources, VkPipelineLayout layout, VkDevice device, uint32_t maxRecoursionDepth, uint32_t samplerCount)
{
	
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

	for(auto& s : shaderSources)
	{
		VkShaderModule module;
		if(VGM::createShaderModule(s.first, module, device) != VK_SUCCESS)
			throw std::runtime_error("Failed to load shader Module!");

		VkPipelineShaderStageCreateInfo stage = {};
		stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage.stage = s.second;
		stage.pNext = nullptr;
		stage.module = module;
		stage.pName = "main";
		stage.flags = 0;

		shaderStages.push_back(stage);

		VkRayTracingShaderGroupCreateInfoKHR group = {};
		group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		group.pNext = nullptr;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.anyHitShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = VK_SHADER_UNUSED_KHR;
		group.intersectionShader = VK_SHADER_UNUSED_KHR;
		

		if(s.second == VK_SHADER_STAGE_RAYGEN_BIT_KHR)
		{
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			group.generalShader = shaderStages.size() - 1;
		}
		if(s.second == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
		{
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			group.closestHitShader = shaderStages.size() - 1;
			_hitCount++;
		}
		if(s.second == VK_SHADER_STAGE_MISS_BIT_KHR)
		{
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			group.generalShader = shaderStages.size() - 1;
			_missCount++;
		}

		shaderGroups.push_back(group);
	}
	
	VkRayTracingPipelineCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.layout = layout;
	createInfo.maxPipelineRayRecursionDepth = maxRecoursionDepth;
	createInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
	createInfo.pGroups = shaderGroups.data();
	createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	createInfo.pStages = shaderStages.data();

	vkCreateRayTracingPipelinesKHR(device, {}, {}, 1, &createInfo, nullptr, &_raytracingPipeline);

	_shaderGroupCount = shaderGroups.size();

	for(auto& s : shaderStages)
	{
		vkDestroyShaderModule(device, s.module, nullptr);
	}

	_samplers.resize(samplerCount);

	createSamplers(device);
}

void RaytracingShader::cmdBind(VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raytracingPipeline);
}

VkResult RaytracingShader::getShaderHandles(std::vector<uint8_t>& handles, VkDevice device, uint32_t groupHandleSize, uint32_t handleAlignment)
{
	uint32_t alignedGroupHandleSize = groupHandleSize;
	
	uint32_t r = groupHandleSize % handleAlignment;
	if (r > 0)
	{
		alignedGroupHandleSize = alignedGroupHandleSize + (handleAlignment - r);
	}

	handles.resize(groupHandleSize * _shaderGroupCount);

	return vkGetRayTracingShaderGroupHandlesKHR(device, _raytracingPipeline, 0, _shaderGroupCount, groupHandleSize * _shaderGroupCount, handles.data());
}

uint32_t RaytracingShader::shaderGroupCount()
{
	return _shaderGroupCount;
}

uint32_t RaytracingShader::missShaderCount()
{
	return _missCount;
}

uint32_t RaytracingShader::hitShaderCount()
{
	return _hitCount;
}

std::vector<VkSampler>& RaytracingShader::getSamplers()
{
	return _samplers;
}

VkPipeline RaytracingShader::get()
{
	return _raytracingPipeline;
}

void RaytracingShader::destroy(VkDevice device)
{
	vkDestroyPipeline(device, _raytracingPipeline, nullptr);
}

void RaytracingShader::createSamplers(VkDevice device)
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
	createInfo.maxAnisotropy = 8.0f;
	createInfo.maxLod = 100.0f;
	createInfo.minLod = 0;
	createInfo.mipLodBias = 0;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.unnormalizedCoordinates = VK_FALSE;

	for (auto& s : _samplers)
	{
		vkCreateSampler(device, &createInfo, nullptr, &s);
	}
}

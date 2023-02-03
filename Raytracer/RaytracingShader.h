#pragma once

#include "vulkan/vulkan.h"
#include "Extensions.h"
#include <vector>
#include <string>

class RaytracingShader
{
public:
	RaytracingShader() = default;
	RaytracingShader(std::vector<std::pair<std::string, VkShaderStageFlagBits>>& shaderSources, VkPipelineLayout layout, VkDevice device, uint32_t maxRecoursionDepth, uint32_t samplerCount);
	void cmdBind(VkCommandBuffer commandBuffer);
	VkResult getShaderHandles(std::vector<uint8_t>& handles, VkDevice device, uint32_t groupHandleSize, uint32_t handleAlignment);
	uint32_t shaderGroupCount();
	uint32_t missShaderCount();
	uint32_t hitShaderCount();
	std::vector<VkSampler>& getSamplers();

	void destroy(VkDevice device);

private:
	VkPipeline _raytracingPipeline;
	uint32_t _shaderGroupCount = 0;
	uint32_t _missCount = 0;
	uint32_t _hitCount = 0;

	std::vector<VkSampler> _samplers;

	void createSamplers(VkDevice device);
};
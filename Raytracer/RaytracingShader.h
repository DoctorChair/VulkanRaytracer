#pragma once

#include "vulkan/vulkan.h"
#include "Extensions.h"
#include <vector>
#include <string>

class RaytracingShader
{
public:
	RaytracingShader() = default;
	RaytracingShader(VkPipelineLayout, std::vector<std::pair<std::string, VkShaderStageFlagBits>> shaderSources, VkPipelineLayout layout, VkDevice device, uint32_t maxRecoursionDepth);

private:
	VkPipeline _raytracingPipeline;
	uint32_t _shaderGroupCount = 0;
};
#pragma once

#include "vulkan/vulkan.h"
#include <string>

namespace VGM
{
	VkResult createShaderModule(const std::string& shaderSource, VkShaderModule& shaderModule, const VkDevice device);

	VkPhysicalDeviceProperties2 getPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice, void* pNext);
}
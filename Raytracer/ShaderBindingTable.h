#pragma once 
#include "vulkan/vulkan.h"
#include "Extensions.h"
#include <vector>
#include <cstdint>
#include "Buffer.h"

class ShaderBindingTable
{
public:
	ShaderBindingTable() = default;
	ShaderBindingTable(VkPipeline raytracePipeline,
		uint32_t missCount, uint32_t hitCount, VkDevice device, VkPhysicalDevice physicalDevice, VmaAllocator allocator);

	VkStridedDeviceAddressRegionKHR getRaygenRegion();
	VkStridedDeviceAddressRegionKHR getMissRegion();
	VkStridedDeviceAddressRegionKHR getHitRegion();

private:
	VGM::Buffer _sbtBuffer;

	VkStridedDeviceAddressRegionKHR _rgenRegion;
	VkStridedDeviceAddressRegionKHR _missRegion;
	VkStridedDeviceAddressRegionKHR _hitRegion;
	VkStridedDeviceAddressRegionKHR _callRegion;
};
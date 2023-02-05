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
	ShaderBindingTable(uint32_t groupHandleSize, uint32_t handleAlignment, std::vector<uint8_t>& shaderHandels, uint32_t shaderGroupBaseAlignment, 
		uint32_t missCount, uint32_t hitCount, VkDevice device, VmaAllocator allocator);

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
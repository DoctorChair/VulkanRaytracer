#pragma once

#include "vulkan/vulkan.h"
#include "Buffer.h"


class TLAS
{
public:
	TLAS() = default;
	void cmdBuildTLAS(uint32_t instanceCount, VkDeviceAddress firstInstanceAddresses, uint32_t addressAlignment, VkDevice device, VmaAllocator allocator, VkCommandBuffer commandBuffer);
	void cmdUpdateTLAS(uint32_t instanceCount, VkDeviceAddress firstInstanceAddresses, VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator allocator, VkCommandBuffer commandBuffer);
	
	void destroy(VkDevice device, VmaAllocator allocator);
	VkAccelerationStructureKHR* get();

private:
	VGM::Buffer _tlasBuffer;
	VGM::Buffer _scratchBuffer;
	uint32_t _activeInstanceCount = 0;
	VkDeviceAddress _scratchBufferAddress = 0;
	VGM::Buffer _instanceBuffer;
	VkAccelerationStructureKHR _tlas;

	bool _isActive = false;
};
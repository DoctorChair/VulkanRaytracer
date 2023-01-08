#pragma once

#include "vulkan/vulkan.h"
#include "Buffer.h"


class TLAS
{
public:
	TLAS() = default;
	TLAS(VkBuffer instanceBuffer, uint32_t instanceCount, VkDeviceAddress* pBLASAdresses, uint32_t pBLASAdressesCount,
		uint32_t* pNumInstances, uint32_t numInstancesCount, VkDevice device, VmaAllocator allocator, VkCommandBuffer commandBuffer);
	void destroy(VkDevice device, VmaAllocator allocator);

private:
	VGM::Buffer _tlasBuffer;
	VGM::Buffer _scratchBuffer;
	VkAccelerationStructureKHR _tlas;

};
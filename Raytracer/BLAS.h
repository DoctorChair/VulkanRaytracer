#pragma once

#include "vulkan/vulkan.h"
#include "Buffer.h"
#include "vma/vk_mem_alloc.h"

class BLAS
{
public:
	BLAS() = default;
	BLAS(VkDeviceAddress vertexAddress, VkDeviceAddress indexAddress, VkFormat vertexFormat, uint32_t scratchOffsetAlignment,
		VkDeviceSize vertexStride, VkIndexType indexType, uint32_t maxVertices, 
		uint32_t VertexOffset, uint32_t IndicesCount, uint32_t IndicesOffeset, VkDevice device, VmaAllocator& allocator, VkCommandBuffer commandBuffer);
	VkDeviceAddress getAddress(VkDevice device);
	void destroy(VkDevice device, VmaAllocator allocator);

private:
	VGM::Buffer _blasBuffer;
	VGM::Buffer _scratchBuffer;
	VkAccelerationStructureKHR _blas;
};
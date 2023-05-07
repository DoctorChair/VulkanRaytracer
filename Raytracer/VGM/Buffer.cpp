#include "Buffer.h"
#include <cstring>

VGM::Buffer::Buffer(VkBufferUsageFlags bufferUsage, VkDeviceSize size, VmaAllocator& allocator, VkDevice device)
{
	VmaAllocationCreateInfo aci = {};
	
	aci.usage = VMA_MEMORY_USAGE_AUTO;

	if(bufferUsage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT || bufferUsage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
	{
		aci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		aci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	}
	if(bufferUsage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT || bufferUsage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
	{
		aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		aci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		
	}
	if(bufferUsage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT || bufferUsage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT 
		|| bufferUsage & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR)
	{
		aci.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		aci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	}
	if(bufferUsage & VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR)
	{
		aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		aci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	}
	if (bufferUsage & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR)
	{
		aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		aci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	}

	VkBufferCreateInfo bci = {};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.pNext = nullptr;
	bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bci.usage = bufferUsage;
	bci.size = size;

	vmaCreateBuffer(allocator, &bci, &aci, &_buffer, &_allocation, nullptr);
	
	_capacity = size;
	_size = 0;

	vkGetBufferMemoryRequirements(device, _buffer, &_memoryRequirements);
}

VGM::Buffer::Buffer(VkBufferUsageFlags bufferUsage, VmaAllocationCreateFlags flags, VkDeviceSize size, VmaAllocator& allocator, VkDevice device)
{
	VmaAllocationCreateInfo aci = {};

	aci.usage = VMA_MEMORY_USAGE_AUTO;
	aci.flags = flags;

	VkBufferCreateInfo bci = {};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.pNext = nullptr;
	bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bci.usage = bufferUsage;
	bci.size = size;

	vmaCreateBuffer(allocator, &bci, &aci, &_buffer, &_allocation, nullptr);

	_capacity = size;
	_size = 0;

	vkGetBufferMemoryRequirements(device, _buffer, &_memoryRequirements);
}

void VGM::Buffer::uploadData(const void* data, size_t length, VmaAllocator allocator)
{
	void* ptr;
	vmaMapMemory(allocator, _allocation, &ptr);
	std::memcpy(ptr, data, length);
	vmaUnmapMemory(allocator, _allocation);
	_size = length;
}

void* VGM::Buffer::map(VmaAllocator allocator)
{
	void* ptr;
	vmaMapMemory(allocator, _allocation, &ptr);
	return ptr;
}

void VGM::Buffer::memcopy(void* dst, void* src, size_t length)
{
	memcpy(dst, src, length);
}

void VGM::Buffer::unmap(VmaAllocator allocator)
{
	vmaUnmapMemory(allocator, _allocation);
}

VkBufferMemoryBarrier2 VGM::Buffer::createBufferMemoryBarrier2(VkDeviceSize size, VkDeviceSize offset, VkAccessFlags2 srcAccessFlags, VkAccessFlags2 dstAccessFlags, 
	VkPipelineStageFlags2 srcStageFlags, VkPipelineStageFlags2 dstStageFlags,
	uint32_t srcIndex, uint32_t dstIndex)
{
	VkBufferMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
	barrier.pNext = nullptr;
	barrier.buffer = _buffer;
	barrier.size = size;
	barrier.offset = offset;
	barrier.srcAccessMask = srcAccessFlags;
	barrier.dstAccessMask = dstAccessFlags;
	barrier.srcStageMask = srcStageFlags;
	barrier.dstStageMask = dstStageFlags;
	barrier.srcQueueFamilyIndex = srcIndex;
	barrier.dstQueueFamilyIndex = dstIndex;

	return barrier;
}

void VGM::Buffer::cmdMemoryBarrier(VkCommandBuffer commandBuffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStageFlags, VkPipelineStageFlags dstStageFlags, VkDependencyFlags dependencyFlags)
{
	VkMemoryBarrier memBarrier = {};
	memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memBarrier.pNext = nullptr;
	memBarrier.srcAccessMask = srcAccessMask;
	memBarrier.dstAccessMask = dstAccessMask;

	vkCmdPipelineBarrier(commandBuffer, srcStageFlags, dstStageFlags, dependencyFlags, 1, &memBarrier, 0, nullptr, 0, nullptr);
}

void VGM::Buffer::cmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, uint32_t regionCount, VkBufferCopy* pRegions)
{
	vkCmdCopyBuffer(commandBuffer, srcBuffer, _buffer, regionCount, pRegions);
}

void VGM::Buffer::cmdAppendCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkDeviceSize length, VkDeviceSize srcOffset)
{
	VkBufferCopy region;
	region.dstOffset = _size;
	region.srcOffset = srcOffset;
	region.size = length;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, _buffer, 1, &region);

	_size = _size + length;
}

VkDeviceAddress VGM::Buffer::getDeviceAddress(VkDevice device)
{
	VkBufferDeviceAddressInfo addressInfo;
	addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	addressInfo.pNext = nullptr;
	addressInfo.buffer = _buffer;
	return vkGetBufferDeviceAddress(device, &addressInfo);
}

VkDeviceSize VGM::Buffer::getAlignment(VkDevice device)
{
	
	return _memoryRequirements.alignment;
}

VkBuffer VGM::Buffer::get()
{
	return _buffer;
}

VkDeviceSize VGM::Buffer::capacity()
{
	return _capacity;
}

VkDeviceSize VGM::Buffer::size()
{
	return _size;
}

void VGM::Buffer::destroy(VmaAllocator& allocator)
{
	vmaDestroyBuffer(allocator, _buffer, _allocation);
}

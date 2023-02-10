#include "ShaderBindingTable.h"
#include <memory>
#include <iostream>

ShaderBindingTable::ShaderBindingTable(VkPipeline raytracePipeline,
	uint32_t missCount, uint32_t hitCount, VkDevice device, VkPhysicalDevice physicalDevice, VmaAllocator allocator)
{
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR pipelineProperties = {};
	pipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	pipelineProperties.pNext = nullptr;
	VkPhysicalDeviceProperties2 properties2{};
	properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	properties2.pNext = &pipelineProperties;
	vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);

	uint32_t grouphandleAlignment = pipelineProperties.shaderGroupHandleAlignment;
	uint32_t groupHandleSize = pipelineProperties.shaderGroupHandleSize;
	uint32_t groupBaseAlignment = pipelineProperties.shaderGroupBaseAlignment;


	size_t count = static_cast<size_t>(1) + missCount + hitCount;

	std::vector<uint8_t> shaderGroupHandels(count * groupHandleSize);
	
	vkGetRayTracingShaderGroupHandlesKHR(device, raytracePipeline, 0, count,
		count * groupHandleSize, shaderGroupHandels.data());

	auto align = [&](uint32_t input, uint32_t alignment)
	{
		uint32_t r = input % alignment;
		return (r == 0) ? input : input + (alignment - r);
	};

	uint32_t alignedGroupHandleSize = align(groupHandleSize, grouphandleAlignment);

	_rgenRegion.stride = align(alignedGroupHandleSize, groupBaseAlignment);
	_rgenRegion.size = _rgenRegion.stride;

	_missRegion.stride = alignedGroupHandleSize;
	_missRegion.size = align(alignedGroupHandleSize * missCount, groupBaseAlignment);

	_hitRegion.stride = alignedGroupHandleSize;
	_hitRegion.size = align(alignedGroupHandleSize * hitCount, groupBaseAlignment);

	VkDeviceSize sbtBufferSize = _rgenRegion.size + _missRegion.size + _hitRegion.size;

	_sbtBuffer = VGM::Buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		| VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR, sbtBufferSize, allocator, device);

	VkDeviceAddress sbtAddress = _sbtBuffer.getDeviceAddress(device);

	_rgenRegion.deviceAddress = sbtAddress;
	_missRegion.deviceAddress = sbtAddress + _rgenRegion.size;
	_hitRegion.deviceAddress = sbtAddress + _rgenRegion.size + _missRegion.size;

	auto getHandle = [&](int index) {return shaderGroupHandels.data() + index * groupHandleSize; };

	void* ptr = _sbtBuffer.map(allocator);
	uint8_t* pSBT = reinterpret_cast<uint8_t*>(ptr);
	uint32_t groupHandleIndex = 0;

	_sbtBuffer.memcopy(pSBT, getHandle(groupHandleIndex++), groupHandleSize);

	pSBT = reinterpret_cast<uint8_t*>(ptr) + _rgenRegion.size;
	for(unsigned int i = 0; i < missCount; i++)
	{
		_sbtBuffer.memcopy(pSBT, getHandle(groupHandleIndex++), groupHandleSize);
		pSBT = pSBT + _missRegion.stride;
	}

	pSBT = reinterpret_cast<uint8_t*>(ptr) + _rgenRegion.size + _missRegion.size;
	for (unsigned int i = 0; i < hitCount; i++)
	{
		_sbtBuffer.memcopy(pSBT, getHandle(groupHandleIndex++), groupHandleSize);
		pSBT = pSBT + _hitRegion.stride;
	}

	_sbtBuffer.unmap(allocator);
}

VkStridedDeviceAddressRegionKHR ShaderBindingTable::getRaygenRegion()
{
	return _rgenRegion;
}

VkStridedDeviceAddressRegionKHR ShaderBindingTable::getMissRegion()
{
	return _missRegion;
}

VkStridedDeviceAddressRegionKHR ShaderBindingTable::getHitRegion()
{
	return _hitRegion;
}

#include "ShaderBindingTable.h"
#include <memory>

ShaderBindingTable::ShaderBindingTable(uint32_t groupHandleSize, uint32_t handleAlignment, std::vector<uint8_t>& shaderHandels, uint32_t shaderGroupBaseAlignment, 
	uint32_t missCount, uint32_t hitCount, VkDevice device, VmaAllocator allocator)
{
	uint32_t alignedGroupHandleSize = groupHandleSize;
	uint32_t r = groupHandleSize % handleAlignment;
	if(r > 0)
	{
		alignedGroupHandleSize = alignedGroupHandleSize + (handleAlignment - r);
	}

	r = alignedGroupHandleSize % shaderGroupBaseAlignment;
	_rgenRegion.stride = (r == 0) ? alignedGroupHandleSize : alignedGroupHandleSize + (shaderGroupBaseAlignment - r);
	_rgenRegion.size = _rgenRegion.stride;
	r = (alignedGroupHandleSize * missCount) % shaderGroupBaseAlignment;
	_missRegion.stride = alignedGroupHandleSize;
	_missRegion.size = (r == 0) ? alignedGroupHandleSize * missCount : alignedGroupHandleSize * missCount + (shaderGroupBaseAlignment - r);
	r = (alignedGroupHandleSize * hitCount) % shaderGroupBaseAlignment;
	_hitRegion.stride = alignedGroupHandleSize;
	_hitRegion.size = (r == 0) ? alignedGroupHandleSize * hitCount : alignedGroupHandleSize * hitCount + (shaderGroupBaseAlignment - r);

	VkDeviceSize sbtSize = _rgenRegion.size + _missRegion.size + _hitRegion.size;

	_sbtBuffer = VGM::Buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		| VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
		, sbtSize, allocator, device);

	VkDeviceAddress stbAddress = _sbtBuffer.getDeviceAddress(device);

	_rgenRegion.deviceAddress = stbAddress;
	_missRegion.deviceAddress = stbAddress + _rgenRegion.size;
	_hitRegion.deviceAddress = stbAddress + _rgenRegion.size + _missRegion.size;

	uint8_t* ptr = reinterpret_cast<uint8_t*>(_sbtBuffer.map(allocator));
	uint8_t* pData = nullptr;
	uint32_t handleIndex = 0;

	pData = ptr;
	_sbtBuffer.memcopy(pData, shaderHandels.data() + handleIndex * groupHandleSize, groupHandleSize);
	
	pData = ptr + _rgenRegion.size;
	for(unsigned int i = 0; i<missCount; i++)
	{
		handleIndex++;
		_sbtBuffer.memcopy(pData, shaderHandels.data() + handleIndex * groupHandleSize, groupHandleSize);
		pData = pData + _missRegion.stride;
	}

	pData = ptr + _rgenRegion.size + _missRegion.size;
	for(unsigned int i = 0; i<hitCount; i++)
	{
		handleIndex++;
		_sbtBuffer.memcopy(pData, shaderHandels.data() + handleIndex * groupHandleSize, groupHandleSize);
		pData = pData + _hitRegion.stride;
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

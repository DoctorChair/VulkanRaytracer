#include "ShaderBindingTable.h"
#include <memory>

ShaderBindingTable::ShaderBindingTable(uint32_t shaderGroupCount, uint32_t groupHandleSize, uint32_t handleAlignment, std::vector<uint8_t>& shaderHandels, VkDevice device, VmaAllocator allocator)
{
	uint32_t alignedGroupHandleSize = groupHandleSize;
	uint32_t r = groupHandleSize % handleAlignment;
	if(r > 0)
	{
		alignedGroupHandleSize = alignedGroupHandleSize + (handleAlignment - r);
	}

	VkDeviceSize sbtSize = (VkDeviceSize)alignedGroupHandleSize * (VkDeviceSize)shaderGroupCount;

	_sbtBuffer = VGM::Buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		| VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
		, sbtSize, allocator);

	 void* ptr = _sbtBuffer.map(allocator);

	uint8_t* dst = reinterpret_cast<uint8_t*>(ptr);

	for(unsigned int i = 0; i<shaderHandels.size(); i++)
	{
		uint8_t* src = shaderHandels.data() + i * groupHandleSize;

		memcpy(dst, src, groupHandleSize);

		dst = dst + alignedGroupHandleSize;
	}
	_sbtBuffer.unmap(allocator);
}

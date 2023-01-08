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
	ShaderBindingTable(uint32_t shaderGroupCount, uint32_t groupHandleSize, uint32_t handleAlignment, std::vector<uint8_t>& shaderHandels, VkDevice device, VmaAllocator allocator);

private:
	VGM::Buffer _sbtBuffer;

};
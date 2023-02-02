#include "TLAS.h"
#include <vector>

void TLAS::cmdBuildTLAS(uint32_t instanceCount, VkDeviceAddress firstInstanceAddresses, uint32_t scratchOffsetAlignment, VkDevice device, VmaAllocator allocator, VkCommandBuffer commandBuffer)
{
	VkAccelerationStructureBuildRangeInfoKHR range;
	range.primitiveCount = instanceCount;
	range.firstVertex = 0;
	range.primitiveOffset = 0;
	range.transformOffset = 0;

	VkAccelerationStructureGeometryInstancesDataKHR instances = {};
	instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	instances.arrayOfPointers = VK_FALSE;
	instances.data.deviceAddress = firstInstanceAddresses;

	VkAccelerationStructureGeometryKHR geometry = {};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.pNext = nullptr;
	geometry.geometry.instances = instances;
	geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		
	VkAccelerationStructureBuildGeometryInfoKHR build = {};
	build.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build.pNext = nullptr;
	build.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
	build.geometryCount = 1;
	build.pGeometries = &geometry;
	build.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	build.srcAccelerationStructure = VK_NULL_HANDLE;

	VkAccelerationStructureBuildSizesInfoKHR size = {};
	size.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	size.pNext = nullptr;

	vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build, &range.primitiveCount, &size);

	_tlasBuffer = VGM::Buffer(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
		| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size.accelerationStructureSize, allocator, device);

	VkAccelerationStructureCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.type = build.type;
	createInfo.size = size.accelerationStructureSize;
	createInfo.buffer = _tlasBuffer.get();
	createInfo.offset = 0;

	vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &_tlas);

	build.dstAccelerationStructure = _tlas;

	_scratchBuffer = VGM::Buffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		size.buildScratchSize + scratchOffsetAlignment, allocator, device);

	VkDeviceAddress scratchAddress = _scratchBuffer.getDeviceAddress(device);
	if (scratchOffsetAlignment != 0)
	{
		VkDeviceAddress r = scratchAddress % scratchOffsetAlignment;
		scratchAddress = (r == 0) ? scratchAddress : scratchAddress + (scratchOffsetAlignment - r);
	}
	build.scratchData.deviceAddress = scratchAddress;

	VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &range;

	vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &build, &pRangeInfo);

	_structureActive = true;
}

void TLAS::cmdUpdateTLAS(uint32_t instanceCount, VkDeviceAddress firstInstanceAddresses, VkDeviceSize addressAlignment, VkDevice device, VmaAllocator allocator, VkCommandBuffer commandBuffer)
{
	VkAccelerationStructureBuildRangeInfoKHR range;
	range.primitiveCount = instanceCount;
	range.firstVertex = 0;
	range.primitiveOffset = 0;
	range.transformOffset = 0;

	VkAccelerationStructureGeometryInstancesDataKHR instances = {};
	instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	instances.arrayOfPointers = VK_FALSE;
	instances.data.deviceAddress = firstInstanceAddresses;

	VkAccelerationStructureGeometryKHR geometry = {};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.pNext = nullptr;
	geometry.geometry.instances = instances;
	geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;

	VkAccelerationStructureBuildGeometryInfoKHR build = {};
	build.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build.pNext = nullptr;
	build.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
	build.geometryCount = 1;
	build.pGeometries = &geometry;
	build.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
	build.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	build.srcAccelerationStructure = _tlas;
	build.dstAccelerationStructure = _tlas;

	VkAccelerationStructureBuildSizesInfoKHR size = {};
	size.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	size.pNext = nullptr;

	vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build, &range.primitiveCount, &size);

	VkAccelerationStructureCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.type = build.type;
	createInfo.size = size.accelerationStructureSize;
	createInfo.buffer = _tlasBuffer.get();
	createInfo.offset = 0;

	vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &_tlas);

	build.dstAccelerationStructure = _tlas;

	build.scratchData.deviceAddress = _scratchBuffer.getDeviceAddress(device);

	VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &range;

	vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &build, &pRangeInfo);
}

void TLAS::destroy(VkDevice device, VmaAllocator allocator)
{
	_tlasBuffer.destroy(allocator);
	_scratchBuffer.destroy(allocator);
	vkDestroyAccelerationStructureKHR(device, _tlas, nullptr);
}

VkAccelerationStructureKHR* TLAS::get()
{
	return &_tlas;
}

bool TLAS::isUpdateable()
{
	return _structureActive;
}



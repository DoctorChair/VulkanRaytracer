#include "TLAS.h"
#include <vector>

TLAS::TLAS(VkBuffer instanceBuffer, uint32_t instanceCount, VkDeviceAddress* pBLASAdresses, uint32_t pBLASAdressesCount, 
	uint32_t* pNumInstances, uint32_t numInstancesCount, VkDevice device, VmaAllocator allocator, VkCommandBuffer commandBuffer)
{
	VkAccelerationStructureBuildRangeInfoKHR range;
	range.primitiveCount = instanceCount;
	range.firstVertex = 0;
	range.primitiveOffset = 0;
	range.transformOffset = 0;

	std::vector<VkAccelerationStructureGeometryKHR> geometries;
	geometries.reserve(instanceCount);

	for(unsigned int i = 0; i < numInstancesCount; i++)
	{
		uint32_t numInstances = pNumInstances[i];

		for(unsigned int b = 0; b<numInstances; b++)
		{
			VkAccelerationStructureGeometryInstancesDataKHR instances = {};
			instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
			instances.arrayOfPointers = VK_FALSE;
			instances.data.deviceAddress = pBLASAdresses[b];

			VkAccelerationStructureGeometryKHR geometry = {};
			geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			geometry.pNext = nullptr;
			geometry.geometry.instances = instances;

			geometries.push_back(geometry);
		}
	}
	
	VkAccelerationStructureBuildGeometryInfoKHR build = {};
	build.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build.pNext = nullptr;
	build.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build.geometryCount = geometries.size();
	build.pGeometries = geometries.data();
	build.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	build.srcAccelerationStructure = VK_NULL_HANDLE;

	VkAccelerationStructureBuildSizesInfoKHR size = {};
	size.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	size.pNext = nullptr;
	
	vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build, &range.primitiveCount, &size);

	_tlasBuffer = VGM::Buffer(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
		| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size.accelerationStructureSize, allocator);

	VkAccelerationStructureCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.type = build.type;
	createInfo.size = size.accelerationStructureSize;
	createInfo.buffer = _tlasBuffer.get();
	createInfo.offset = 0;

	vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &_tlas);

	_scratchBuffer = VGM::Buffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size.buildScratchSize, allocator);

	build.scratchData.deviceAddress = _scratchBuffer.getDeviceAddress(device);

	VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &range;

	vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &build, &pRangeInfo);
}

#include "BLAS.h"

BLAS::BLAS(VkDeviceAddress vertexAddress, VkDeviceAddress indexAddress, VkFormat vertexFormat, 
	VkDeviceSize vertexStride, VkIndexType indexType, uint32_t maxVertices, 
	uint32_t VertexOffset, uint32_t IndicesCount, uint32_t IndicesOffeset, VkDevice device, VmaAllocator& allocator, VkCommandBuffer commandBuffer)
{
	VkAccelerationStructureGeometryTrianglesDataKHR tris = {};
	tris.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	tris.pNext = nullptr;
	tris.indexType = indexType;
	tris.vertexFormat = vertexFormat;
	tris.transformData = { 0 };
	tris.maxVertex = maxVertices;
	tris.indexData.deviceAddress = indexAddress;
	tris.vertexData.deviceAddress = vertexAddress;
	tris.vertexStride = vertexStride;

	VkAccelerationStructureGeometryKHR geo = {};
	geo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geo.pNext = nullptr;
	geo.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geo.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	geo.geometry.triangles = tris;

	VkAccelerationStructureBuildRangeInfoKHR range;
	range.firstVertex = VertexOffset;
	range.primitiveCount = IndicesCount / 3;
	range.primitiveOffset = IndicesOffeset;
	range.transformOffset = 0;

	VkAccelerationStructureBuildGeometryInfoKHR build;
	build.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build.pNext = nullptr;
	build.geometryCount = 1;
	build.pGeometries = &geo;
	build.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	build.srcAccelerationStructure = VK_NULL_HANDLE;

	VkAccelerationStructureBuildSizesInfoKHR buildSize;
	buildSize.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	buildSize.pNext = nullptr;
	
	vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, 
		&build, &range.primitiveCount, &buildSize);

	_blasBuffer = VGM::Buffer(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
		| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buildSize.accelerationStructureSize, allocator);

	VkAccelerationStructureCreateInfoKHR accelInfo = {};
	accelInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelInfo.type = build.type;
	accelInfo.pNext = nullptr;
	accelInfo.offset = 0;
	accelInfo.buffer = _blasBuffer.get();
	accelInfo.size = buildSize.accelerationStructureSize;

	vkCreateAccelerationStructureKHR(device, &accelInfo, nullptr, &_blas);

	build.dstAccelerationStructure = _blas;

	_scratchBuffer = VGM::Buffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buildSize.buildScratchSize, allocator);

	build.scratchData.deviceAddress = _scratchBuffer.getDeviceAddress(device);

	VkAccelerationStructureBuildRangeInfoKHR * pRangeInfo = &range;

	vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &build, &pRangeInfo);
}

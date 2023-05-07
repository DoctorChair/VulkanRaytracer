#include "Extensions.h"


	//VK_KHR_ray_tracing_pipeline
	static PFN_vkCmdSetRayTracingPipelineStackSizeKHR pfn_vkCmdSetRayTracingPipelineStackSizeKHR = 0;
	static PFN_vkCmdTraceRaysKHR pfn_vkCmdTraceRaysKHR = 0;
	static PFN_vkCmdTraceRaysIndirectKHR pfn_vkCmdTraceRaysIndirectKHR = 0;
	static PFN_vkCreateRayTracingPipelinesKHR pfn_vkCreateRayTracingPipelinesKHR = 0;
	static PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR pfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = 0;
	static PFN_vkGetRayTracingShaderGroupHandlesKHR pfn_vkGetRayTracingShaderGroupHandlesKHR = 0;
	static PFN_vkGetRayTracingShaderGroupStackSizeKHR	pfn_vkGetRayTracingShaderGroupStackSizeKHR = 0;

	//VK_KHR_acceleration_structure
	static PFN_vkBuildAccelerationStructuresKHR					 pfn_vkBuildAccelerationStructuresKHR = 0;
	static PFN_vkCmdBuildAccelerationStructuresIndirectKHR			 pfn_vkCmdBuildAccelerationStructuresIndirectKHR = 0;
	static PFN_vkCmdBuildAccelerationStructuresKHR					 pfn_vkCmdBuildAccelerationStructuresKHR = 0;
	static PFN_vkCmdCopyAccelerationStructureKHR					 pfn_vkCmdCopyAccelerationStructureKHR = 0;
	static PFN_vkCmdCopyAccelerationStructureToMemoryKHR			 pfn_vkCmdCopyAccelerationStructureToMemoryKHR = 0;
	static PFN_vkCmdCopyMemoryToAccelerationStructureKHR			 pfn_vkCmdCopyMemoryToAccelerationStructureKHR = 0;
	static PFN_vkCmdWriteAccelerationStructuresPropertiesKHR		 pfn_vkCmdWriteAccelerationStructuresPropertiesKHR = 0;
	static PFN_vkCopyAccelerationStructureKHR						 pfn_kCopyAccelerationStructureKHR = 0;
	static PFN_vkCopyAccelerationStructureToMemoryKHR				 pfn_vkCopyAccelerationStructureToMemoryKHR = 0;
	static PFN_vkCopyMemoryToAccelerationStructureKHR				 pfn_vkCopyMemoryToAccelerationStructureKHR = 0;
	static PFN_vkCreateAccelerationStructureKHR					 pfn_vkCreateAccelerationStructureKHR = 0;
	static PFN_vkDestroyAccelerationStructureKHR					 pfn_vkDestroyAccelerationStructureKHR = 0;
	static PFN_vkGetAccelerationStructureBuildSizesKHR				 pfn_vkGetAccelerationStructureBuildSizesKHR = 0;
	static PFN_vkGetAccelerationStructureDeviceAddressKHR			 pfn_vkGetAccelerationStructureDeviceAddressKHR = 0;
	static PFN_vkGetDeviceAccelerationStructureCompatibilityKHR	 pfn_vkGetDeviceAccelerationStructureCompatibilityKHR = 0;
	static PFN_vkWriteAccelerationStructuresPropertiesKHR			 pfn_vkWriteAccelerationStructuresPropertiesKHR = 0;

	//VK_DEFFERED_HOST_OPERATIONS
	static PFN_vkCreateDeferredOperationKHR pfn_vkCreateDeferredOperationKHR = 0;
	static PFN_vkDeferredOperationJoinKHR pfn_vkDeferredOperationJoinKHR = 0;
	static PFN_vkDestroyDeferredOperationKHR pfn_vkDestroyDeferredOperationKHR = 0;
	static PFN_vkGetDeferredOperationMaxConcurrencyKHR pfn_vkGetDeferredOperationMaxConcurrencyKHR = 0;
	static PFN_vkGetDeferredOperationResultKHR pfn_vkGetDeferredOperationResultKHR = 0;

	void VGM::loadVulkanExtensions(VkInstance instance, VkDevice device)
	{
		pfn_vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
		pfn_vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
		pfn_vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
		pfn_vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
		pfn_vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
		pfn_vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR");
		pfn_vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
		pfn_vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
	}

	VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysKHR(VkCommandBuffer commandBuffer,
		const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
		const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable,
		const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
		const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable,
		uint32_t                                    width,
		uint32_t                                    height,
		uint32_t                                    depth)
	{
		pfn_vkCmdTraceRaysKHR(commandBuffer,
			pRaygenShaderBindingTable,
			pMissShaderBindingTable,
			pHitShaderBindingTable,
			pCallableShaderBindingTable,
			width,
			height,
			depth);
	}

	VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureKHR(VkDevice device,
		const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkAccelerationStructureKHR* pAccelerationStructure)
	{
		return pfn_vkCreateAccelerationStructureKHR(device, pCreateInfo, pAllocator, pAccelerationStructure);
	}

	VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer,
		uint32_t infoCount,
		const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
		const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
	{
		return pfn_vkCmdBuildAccelerationStructuresKHR(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
	}

	VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureBuildSizesKHR(
		VkDevice                                    device,
		VkAccelerationStructureBuildTypeKHR         buildType,
		const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
		const uint32_t* pMaxPrimitiveCounts,
		VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo)
	{
		pfn_vkGetAccelerationStructureBuildSizesKHR(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
	}

	VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesKHR(VkDevice device,
		VkDeferredOperationKHR deferredOperation,
		VkPipelineCache pipelineCache,
		uint32_t createInfoCount,
		const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
		const VkAllocationCallbacks* pAllocator,
		VkPipeline* pPipelines)
	{
		return pfn_vkCreateRayTracingPipelinesKHR(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
	}

	VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesKHR(VkDevice device,
		VkPipeline pipeline,
		uint32_t firstGroup,
		uint32_t groupCount,
		size_t dataSize,
		void* pData)
	{
		return pfn_vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData);
	}

	VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureKHR(VkDevice device,
		VkAccelerationStructureKHR accelerationStructure,
		const VkAllocationCallbacks* pAllocator)
	{
		pfn_vkDestroyAccelerationStructureKHR(device, accelerationStructure, pAllocator);
	}

	VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* pInfo)
	{
		return pfn_vkGetAccelerationStructureDeviceAddressKHR(device, pInfo);
	}
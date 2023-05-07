#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace VGM
{

	class DescriptorSetAllocator
	{
	public:
		DescriptorSetAllocator() = default;
		DescriptorSetAllocator(const std::vector<VkDescriptorPoolSize> sizes, uint32_t maxSets, VkDescriptorPoolCreateFlags flags, unsigned int poolCount, VkDevice device);
		VkResult allocateDescriptorSet(VkDescriptorSetLayout& setLayout, VkDescriptorSet* descriptorSet, VkDevice device);
		VkResult resetPools(VkDevice device);
		void destroy(VkDevice device);

	private:
		VkResult createDescriptorPool(const std::vector<VkDescriptorPoolSize> sizes, uint32_t maxSets, VkDescriptorPoolCreateFlags flags, VkDevice device);

		std::vector<VkDescriptorPool> _pools;
		unsigned int fullPools = 0;
		
		std::vector<VkDescriptorPoolSize> _sizes;
		uint32_t _maxSets;
		VkDescriptorPoolCreateFlags _flags;
	};

}
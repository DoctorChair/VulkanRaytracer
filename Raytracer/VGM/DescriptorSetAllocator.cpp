#include "DescriptorSetAllocator.h"

VGM::DescriptorSetAllocator::DescriptorSetAllocator(const std::vector<VkDescriptorPoolSize> sizes, uint32_t maxSets, VkDescriptorPoolCreateFlags flags, unsigned int poolCount, VkDevice device)
{
	_pools.reserve(poolCount);
	for(unsigned int i = 0; i<poolCount; i++)
	{
		createDescriptorPool(sizes, maxSets, flags, device);
	}
}

VkResult VGM::DescriptorSetAllocator::allocateDescriptorSet(VkDescriptorSetLayout& setLayout, VkDescriptorSet* descriptorSet, VkDevice device)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = _pools[fullPools];
	allocInfo.pSetLayouts = &setLayout;
	allocInfo.descriptorSetCount = 1;
	
	VkResult result = vkAllocateDescriptorSets(device, &allocInfo, descriptorSet);

	if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_OUT_OF_DEVICE_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
	{
		fullPools++;
		if (fullPools > _pools.size())
		{
			result = createDescriptorPool(_sizes, _maxSets, _flags, device);
			if (result != VK_SUCCESS)
			{
				return result;
			}
		}

		result = vkAllocateDescriptorSets(device, &allocInfo, descriptorSet);
		if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_OUT_OF_DEVICE_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
		{
			return result;
		}
	}
	return result;
}

VkResult VGM::DescriptorSetAllocator::resetPools(VkDevice device)
{
	VkResult result;
	for(auto& p : _pools)
	{
		result = vkResetDescriptorPool(device, p, 0);
		if (result != VK_SUCCESS)
			return result;
	}
	fullPools = 0;
	return VK_SUCCESS;
}

void VGM::DescriptorSetAllocator::destroy(VkDevice device)
{
	for(auto& p : _pools)
	{
		vkDestroyDescriptorPool(device, p, nullptr);
	}
}

VkResult VGM::DescriptorSetAllocator::createDescriptorPool(const std::vector<VkDescriptorPoolSize> sizes, uint32_t maxSets, VkDescriptorPoolCreateFlags flags, VkDevice device)
{
	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.poolSizeCount = sizes.size();
	createInfo.pPoolSizes = sizes.data();
	createInfo.maxSets = maxSets;
	createInfo.flags = flags;
	
	_pools.emplace_back();
	return vkCreateDescriptorPool(device, &createInfo, nullptr, &_pools.back());
}

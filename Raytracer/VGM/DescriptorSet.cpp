#include "DescriptorSet.h"

VGM::DescriptorSetLayoutBuilder VGM::DescriptorSetLayoutBuilder::begin()
{
	 DescriptorSetLayoutBuilder builder;

	 return builder;
}

VGM::DescriptorSetLayoutBuilder& VGM::DescriptorSetLayoutBuilder::addBinding(uint32_t bindingIndex, VkDescriptorType type, VkSampler* pImutableSampler, VkShaderStageFlags shaderStageFlags, uint32_t DescriptorCount)
{
	VkDescriptorSetLayoutBinding binding = {};

	binding.binding = bindingIndex;
	binding.descriptorCount = DescriptorCount;
	binding.descriptorType = type;
	binding.pImmutableSamplers = nullptr;
	binding.stageFlags = shaderStageFlags;
	
	_bindings.push_back(binding);

	return *this;
}

VkResult VGM::DescriptorSetLayoutBuilder::createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout* layout)
{
	createInfo.bindingCount = _bindings.size();
	createInfo.pBindings = _bindings.data();
	
	return vkCreateDescriptorSetLayout(device, &createInfo, nullptr, layout);
}

VGM::DescriptorSetBuilder VGM::DescriptorSetBuilder::begin()
{
	return DescriptorSetBuilder();
}

VGM::DescriptorSetBuilder::DescriptorSetBuilder()
{
}

VGM::DescriptorSetBuilder& VGM::DescriptorSetBuilder::bindBuffer(VkDescriptorType type, VkDescriptorBufferInfo& bufferInfo)
{
	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;
	write.descriptorType = type;
	write.descriptorCount = 1;
	write.dstBinding = descriptorWrites.size();
	write.dstSet = VK_NULL_HANDLE;
	write.pBufferInfo = &bufferInfo;
	
	descriptorWrites.push_back(write);

	return *this;
}

VGM::DescriptorSetBuilder& VGM::DescriptorSetBuilder::bindImage(VkDescriptorType type, VkDescriptorImageInfo* pImageInfos, VkSampler* imutableSampler, uint32_t descriptorCount)
{
	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;
	write.descriptorType = type;
	write.descriptorCount = descriptorCount;
	write.dstBinding = descriptorWrites.size();
	write.dstSet = VK_NULL_HANDLE;
	write.pImageInfo = pImageInfos;

	descriptorWrites.push_back(write);

	return *this;
}

VGM::DescriptorSetBuilder& VGM::DescriptorSetBuilder::bindAccelerationStructure(VkDescriptorType type, VkWriteDescriptorSetAccelerationStructureKHR* accelWrite, uint32_t descriptorCount)
{
	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = accelWrite;
	write.descriptorType = type;
	write.descriptorCount = descriptorCount;
	write.dstBinding = descriptorWrites.size();
	write.dstSet = VK_NULL_HANDLE;

	descriptorWrites.push_back(write);

	return *this;
}

VkResult VGM::DescriptorSetBuilder::createDescriptorSet(VkDevice device, VkDescriptorSet* descriptorSet, VkDescriptorSetLayout* layout, DescriptorSetAllocator& allocator)
{
	allocInfo.pSetLayouts = layout;
	VkResult result = allocator.allocateDescriptorSet(*layout, descriptorSet, device);
	
	if (result != VK_SUCCESS)
		return result;
	if (descriptorWrites.size() > 0)
	{
		for (auto& w : descriptorWrites)
		{
			w.dstSet = *descriptorSet;
		}
		vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}

	return result;
}


VGM::DescriptorSetUpdater VGM::DescriptorSetUpdater::begin(VkDescriptorSet* pDescriptorSets)
{
	return DescriptorSetUpdater(pDescriptorSets);
}

VGM::DescriptorSetUpdater::DescriptorSetUpdater(VkDescriptorSet* pDescriptorSetst)
{
	_pDescriptorSets = pDescriptorSetst;
}

VGM::DescriptorSetUpdater& VGM::DescriptorSetUpdater::bindBuffer(VkDescriptorType type, VkDescriptorBufferInfo& bufferInfo, uint32_t descriptorCount, uint32_t dstSetIndex, uint32_t bindingIndex)
{
	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;
	write.descriptorType = type;
	write.descriptorCount = 1;
	write.dstBinding = bindingIndex;
	write.dstSet = _pDescriptorSets[dstSetIndex];
	write.pBufferInfo = &bufferInfo;

	descriptorWrites.push_back(write);

	return *this;
}

VGM::DescriptorSetUpdater& VGM::DescriptorSetUpdater::bindImage(VkDescriptorType type, VkDescriptorImageInfo* pImageInfos, VkSampler* imutableSampler, uint32_t descriptorCount, uint32_t dstSetIndex, uint32_t bindingIndex)
{
	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;
	write.descriptorType = type;
	write.descriptorCount = descriptorCount;
	write.dstBinding = bindingIndex;
	write.dstSet = _pDescriptorSets[dstSetIndex];
	write.pImageInfo = pImageInfos;


	descriptorWrites.push_back(write);

	return *this;
}

VGM::DescriptorSetUpdater& VGM::DescriptorSetUpdater::bindAccelerationStructure(VkDescriptorType type, VkWriteDescriptorSetAccelerationStructureKHR* accelWrite, uint32_t descriptorCount, uint32_t dstSetIndex, uint32_t bindingIndex)
{
	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = accelWrite;
	write.descriptorType = type;
	write.descriptorCount = descriptorCount;
	write.dstBinding = bindingIndex;
	write.dstSet = _pDescriptorSets[dstSetIndex];

	descriptorWrites.push_back(write);

	return *this;
}

void VGM::DescriptorSetUpdater::updateDescriptorSet(VkDevice device)
{
	vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

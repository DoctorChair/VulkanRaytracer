#pragma once
#include "vulkan/vulkan.h"
#include <vector>
#include "DescriptorSetAllocator.h"

namespace VGM
{
	class DescriptorSetLayoutBuilder
	{
	public:
		static DescriptorSetLayoutBuilder begin();
		DescriptorSetLayoutBuilder() = default;
		DescriptorSetLayoutBuilder& addBinding(uint32_t bindingIndex, VkDescriptorType type, VkSampler* pImutableSampler, VkShaderStageFlags shaderStageFlags, uint32_t DescriptorCount);
		VkResult createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout* layout);

	private:
		VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			0,
			nullptr };

		std::vector<VkDescriptorSetLayoutBinding> _bindings;
	};

	class DescriptorSetBuilder
	{
	public:
		static DescriptorSetBuilder begin();
		DescriptorSetBuilder();

		DescriptorSetBuilder& bindBuffer(VkDescriptorType type, VkDescriptorBufferInfo& bufferInfo);
		DescriptorSetBuilder& bindImage(VkDescriptorType type, VkDescriptorImageInfo* pImageInfos, VkSampler* imutableSampler, uint32_t descriptorCount);
		DescriptorSetBuilder& bindAccelerationStructure(VkDescriptorType type, VkWriteDescriptorSetAccelerationStructureKHR* accelWrite, uint32_t descriptorCount);
		VkResult createDescriptorSet(VkDevice device, VkDescriptorSet* descriptorSet, VkDescriptorSetLayout* layout, DescriptorSetAllocator& allocator);
		

	private:
		std::vector<VkWriteDescriptorSet> descriptorWrites;
		DescriptorSetLayoutBuilder layoutBuilder;

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			nullptr,
			VK_NULL_HANDLE,
			0,
			nullptr };
	};

	class DescriptorSetUpdater
	{
	public:
		static DescriptorSetUpdater begin(VkDescriptorSet* pDescriptorSets);
		DescriptorSetUpdater(VkDescriptorSet* pDescriptorSets);
		DescriptorSetUpdater& bindBuffer(VkDescriptorType type, VkDescriptorBufferInfo& bufferInfo, uint32_t descriptorCount, uint32_t dstSetIndex, uint32_t bindingIndex);
		DescriptorSetUpdater& bindImage(VkDescriptorType type, VkDescriptorImageInfo* pImageInfos, VkSampler* imutableSampler, uint32_t descriptorCount, uint32_t dstSetIndex, uint32_t bindingIndex);
		DescriptorSetUpdater& bindAccelerationStructure(VkDescriptorType type, VkWriteDescriptorSetAccelerationStructureKHR* accelWrite, uint32_t descriptorCount, uint32_t dstSetIndex, uint32_t bindingIndex);
		void updateDescriptorSet(VkDevice device);

	private:
		std::vector<VkWriteDescriptorSet> descriptorWrites;
		VkDescriptorSet* _pDescriptorSets;
		uint32_t descriptorSetsCount;
	};
}
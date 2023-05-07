#include "Utils.h"
#include <vector>
#include <fstream>

VkResult VGM::createShaderModule(const std::string& shaderSource, VkShaderModule& shaderModule, const VkDevice device)
{
	std::ifstream shaderStream(shaderSource, std::ios::ate | std::ios::binary);

	if (shaderStream.is_open())
	{
		size_t filesize = static_cast<size_t>(shaderStream.tellg());
		std::vector<char> programBuffer(filesize);
		shaderStream.seekg(0);
		char* ptr = programBuffer.data();
		shaderStream.read((char*)ptr, filesize);

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.pCode = reinterpret_cast<const uint_fast32_t*>(programBuffer.data());
		createInfo.codeSize = programBuffer.size();
		createInfo.flags = 0;

		return vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
	}
	return VK_ERROR_INITIALIZATION_FAILED;
}

VkPhysicalDeviceProperties2 VGM::getPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice, void* pNext)
{
	VkPhysicalDeviceProperties2 properties2 = {};
	properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	properties2.pNext = pNext;

	vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);
	return properties2;
}

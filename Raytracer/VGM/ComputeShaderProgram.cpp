#include "ComputeShaderProgram.h"
#include <fstream>

VGM::ComputeShaderProgram::ComputeShaderProgram(const std::string& shaderSource, VkShaderStageFlagBits shaderStageFlagBits, VkPipelineLayout layout, VkDevice device)
{
    VkShaderModule module;
    createShaderModule(shaderSource, module, device);

    VkPipelineShaderStageCreateInfo stage = {};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.pNext = nullptr;
    stage.pName = "main";
    stage.pSpecializationInfo = nullptr;
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = module;
    stage.flags = 0;
 

    VkComputePipelineCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    info.pNext = nullptr;
    info.stage = stage;
    info.layout = layout;
    info.flags = 0;
    info.basePipelineHandle = 0;
    info.basePipelineIndex = 0;

    vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &_pipeline);
}

void VGM::ComputeShaderProgram::cmdBind(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
}

void VGM::ComputeShaderProgram::destroy(VkDevice device)
{
    vkDestroyPipeline(device, _pipeline, nullptr);
}

VkResult VGM::ComputeShaderProgram::createShaderModule(const std::string& shaderSource, VkShaderModule& shaderModule, const VkDevice device)
{
    std::ifstream shaderStream(shaderSource, std::ios::ate | std::ios::binary);

    if (shaderStream.is_open())
    {
        size_t filesize = static_cast<size_t>(shaderStream.tellg());
        std::vector<uint32_t> programBuffer(filesize / sizeof(uint32_t));
        shaderStream.seekg(0);
        uint32_t* ptr = programBuffer.data();
        shaderStream.read((char*)ptr, filesize);

        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.pCode = programBuffer.data();
        createInfo.codeSize = programBuffer.size() * sizeof(uint32_t);
        createInfo.flags = 0;

        return vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    }
    return VK_ERROR_INITIALIZATION_FAILED;
}

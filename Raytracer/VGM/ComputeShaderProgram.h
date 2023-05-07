
#include <vulkan/vulkan.h>
#include <vector>
#include <string>


namespace VGM
{
	class ComputeShaderProgram
	{
	public:
		ComputeShaderProgram() = default;
		ComputeShaderProgram(const std::string& shaderSource, VkShaderStageFlagBits shaderStageFlagBits, VkPipelineLayout layout, VkDevice device);
		void cmdBind(VkCommandBuffer commandBuffer);
		void destroy(VkDevice device);

	private:
		VkResult createShaderModule(const std::string& shaderSource, VkShaderModule& shaderModule, const VkDevice device);
		VkPipeline _pipeline;
	};
}
#include "VulkanContext.h"
#include "RenderpassBuilder.h"
#include "Texture.h"
#include "Buffer.h"
#include "DescriptorSet.h"
#include "Shader.h"
#include "CommandBuffer.h"
#include "CommandBufferAllocator.h"

#include "VulkanErrorHandling.h"

#include "ModelLoader/ModelLoader.h"

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
		500, 500, 
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

	VkPhysicalDeviceVulkan13Features features13 = {};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.pNext = nullptr;
	features13.dynamicRendering = true;

	VGM::VulkanContext context(window, 1, 3, 0, 
		{}, 
		{"VK_LAYER_KHRONOS_validation"},
		{VK_KHR_SWAPCHAIN_EXTENSION_NAME}, &features13);

	ModelLoader modelLoader;

	std::string path = "C:\\Users\\Eric\\Downloads\\SunTemple_v4\\SunTemple_v4\\SunTemple\\SunTemple.fbx";
	ModelData data = modelLoader.loadModel(path);
	
	VkDescriptorSetLayout layout;
	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
		.createDescriptorSetLayout(context._device, &layout);

	std::vector<VkDescriptorPoolSize> sizes = { {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10} };
	VGM::DescriptorSetAllocator descriptorAllocator(sizes, 10, 0, 10, context._device);

	VGM::Buffer uniformBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 100 * sizeof(float), context._gpuAllocator);

	VkDescriptorBufferInfo info;
	info.buffer = uniformBuffer.get();
	info.offset = 0;
	info.range = uniformBuffer.size();
	
	VkDescriptorSet descriptorSet;

	VGM::DescriptorSetBuilder::begin()
		.bindBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, info)
		.createDescriptorSet(context._device, &descriptorSet, &layout, descriptorAllocator);

	VGM::PipelineConfigurator configuration;
	std::vector<VkFormat> renderingColorFormats = { context._swapchainFormat.format };
	
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = { colorBlendAttachment };

	configuration.setRenderingFormats(renderingColorFormats, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED);
	configuration.setViewportState(500, 500, 0.0f, 1.0f, 0.0f, 0.0f);
	configuration.setScissorState(500, 500, 0.0f, 0.0f);
	configuration.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	configuration.setColorBlendingState(colorBlendAttachments);
	configuration.setDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

	std::vector<std::pair<std::string, VkShaderStageFlagBits>> sources = {
		{ "C:\\Users\\Eric\\projects\\VGM\\shaders\\SPIRV\\dummyVERT.spv", VK_SHADER_STAGE_VERTEX_BIT },
		{ "C:\\Users\\Eric\\projects\\VGM\\shaders\\SPIRV\\dummyFRAG.spv", VK_SHADER_STAGE_FRAGMENT_BIT} 
	};

	VkPipelineLayout pipelineLayout;
	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.pNext = nullptr;
	layoutCreateInfo.flags = 0;
	layoutCreateInfo.pushConstantRangeCount = 0;
	layoutCreateInfo.setLayoutCount = 0;
	vkCreatePipelineLayout(context._device, &layoutCreateInfo, nullptr, &pipelineLayout);
	
	VGM::ShaderProgram shader(sources, pipelineLayout, configuration, context._device);
	
	VGM::Texture renderDepthTarget(VK_FORMAT_D32_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, { 500, 500, 1 }, 1, 1, context._gpuAllocator);
	VkImageView depthView;
	renderDepthTarget.createImageView(0, 1, 0, 1, context._device, &depthView);

	VkClearColorValue clearColor = { 0.0f, 0.0f, 1.0f, 0.0f };

	std::vector<VGM::Framebuffer> presentFramebuffers;
	presentFramebuffers.resize(context._swapchainImageViews.size());
	for (unsigned int i = 0; i < context._swapchainImageViews.size(); i++)
	{
		presentFramebuffers[i].bindColorTextureTarget(context._swapchainImageViews[i],
			VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
			clearColor);
		presentFramebuffers[i].bindDepthTextureTarget(depthView,
			VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
			0.0f);
	}

	VGM::CommandBufferAllocator graphicsCommandBufferAllocator(context._graphicsQueueFamilyIndex, context._device);
	VGM::CommandBuffer graphicsCommandBuffer(graphicsCommandBufferAllocator, context._device);
	
	graphicsCommandBuffer.begin(nullptr, 0, context._device);
	renderDepthTarget.cmdPrepareTextureForFragmentShaderWrite(graphicsCommandBuffer.get());
	graphicsCommandBuffer.end();
	graphicsCommandBuffer.submit(nullptr, 0, nullptr, 0, nullptr, VK_NULL_HANDLE, context._graphicsQeueu);

	vkDeviceWaitIdle(context._device);

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkFence renderFence;
	vkCreateFence(context._device, &fenceCreateInfo, nullptr, &renderFence);

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	VkSemaphore renderSemaphore;
	vkCreateSemaphore(context._device, &semaphoreCreateInfo, nullptr, &renderSemaphore);


	bool quit = false;
	SDL_Event e;
	while (!quit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
		}

		uint32_t index = context.aquireNextImageIndex(nullptr);

		graphicsCommandBuffer.begin(&renderFence, 1, context._device);
		context.cmdPrepareSwapchainImageForRendering(graphicsCommandBuffer.get());
		VkRect2D viewArea = { 0, 0, 500, 500 };
		
		presentFramebuffers[index].cmdBeginRendering(graphicsCommandBuffer.get(), viewArea);
		
		shader.cmdBind(graphicsCommandBuffer.get());
		vkCmdDraw(graphicsCommandBuffer.get(), 3, 1, 0, 0);
		
		presentFramebuffers[index].cmdEndRendering(graphicsCommandBuffer.get());

		context.cmdPrepareSwaphainImageForPresent(graphicsCommandBuffer.get());
		graphicsCommandBuffer.end();
		VkPipelineStageFlags waitStage = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		graphicsCommandBuffer.submit(&renderSemaphore, 1, &context._presentSemaphore, 1, &waitStage, renderFence, context._graphicsQeueu);
		
		context.presentImage(&renderSemaphore, 1);
	}

	vkDeviceWaitIdle(context._device);

	uniformBuffer.destroy(context._gpuAllocator);
	renderDepthTarget.destroy(context._gpuAllocator);
	shader.destroy(context._device);
	graphicsCommandBufferAllocator.destroy(context._device);
	descriptorAllocator.destroy(context._device);

	context.destroy();

	return 0;
}
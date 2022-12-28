#pragma once
#include "SDL.h"
#include "VulkanContext.h"
#include "Buffer.h"
#include "Framebuffer.h"
#include "CommandBuffer.h"
#include "CommandBufferAllocator.h"
#include "DescriptorSet.h"
#include "DescriptorSetAllocator.h"
#include "Texture.h"
#include "Shader.h"
#include "PipelineBuilder.h"

#include "glm/glm.hpp"

struct GBuffer
{
	VGM::Framebuffer framebuffer;

	VGM::Texture positionBuffer;
	VkImageView positionView;
	VGM::Texture normalBuffer;
	VkImageView normalView;
	VGM::Texture idBuffer;
	VkImageView idView;
	VGM::Texture albedoBuffer;
	VkImageView albedoView;
};

struct VertexBuffer
{
	VGM::Buffer vertices;
	VGM::Buffer indices;
};

struct Material
{
	uint32_t albedoIndex;
	uint32_t metallicIndex;
	uint32_t normalIndex;
	uint32_t roughnessIndex;
};

struct Mesh
{
	uint32_t indexOffset;
	uint32_t vertexOffset;
	uint32_t indicesCount;
	uint32_t vertexCount;
	Material material;
};

struct DrawData
{
	glm::mat4 modelMatrix;
	Material material;
};

struct Model
{
	std::vector<Mesh> meshes;
};

struct globalRenderData
{
	float fog;
};

struct CameraData
{
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	glm::mat4 cameraMatrix;
	glm::vec3 cameraPosition;
};

struct FrameSynchro
{
	VkSemaphore _availabilitySemaphore;
	VkSemaphore _renderSemaphore;
	VkFence _renderFence;
};

class Raytracer
{
public:
	Raytracer() = default;
	void init(uint32_t windowWidth, uint32_t windowHeight);
	void loadeMesh();
	void execute();
	void destroy();

private:
	void initVertexBuffer();
	void initTextureArrays();
	void initgBuffers();
	void initDescriptorSetAllocator();
	void initgBufferDescriptorSets();
	void initGBufferShader();
	void initCommandBuffers();
	void initSyncStructures();
	void initDataBuffers();

	VGM::VulkanContext renderContext;
	SDL_Window* window;
	uint32_t windowWidth;
	uint32_t windowHeight;

	uint32_t _maxBuffers  = 3;
	uint32_t _maxDrawCount = 1000;

	VGM::Texture _albedoTextureArray;
	VkSampler _albedoSampler;
	VkImageView _albedoTextureView;

	VGM::Texture _metallicTextureArray;
	VkSampler _metallicSampler;
	VkImageView _metallicTextureView;

	VGM::Texture _normalTextureArray;
	VkSampler _normalSampler;
	VkImageView _normalTextureView;

	VGM::Texture _roughnessTextureArray;
	VkSampler _roughnessSampler;
	VkImageView _roughnessTextureView;

	VertexBuffer _vertexBuffer;

	VGM::ShaderProgram _gBufferShader;
	VkPipelineLayout _gBufferPipelineLayout;
	std::vector<GBuffer> _gBufferChain;

	VGM::DescriptorSetAllocator _descriptorSetAllocator;

	VGM::CommandBufferAllocator _renderCommandBufferAllocator;
	uint32_t _currentFrameIndex = 0;
	std::vector<VGM::CommandBuffer> _renderCommandBuffers;
	std::vector<FrameSynchro> _frameSynchroStructs;
	std::vector<VGM::Buffer> _cameraBuffers;
	std::vector<VGM::Buffer> _globalRenderDataBuffers;
	std::vector<VGM::Buffer> _drawDataIsntanceBuffers;

	VkDescriptorSetLayout level0Layout;
	VkDescriptorSetLayout level1Layout;
	VkDescriptorSetLayout level2Layout;
};
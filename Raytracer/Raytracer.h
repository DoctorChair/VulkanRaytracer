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

#include "Vertex.h"

#include "BLAS.h"
#include "TLAS.h"
#include "RaytracingShader.h"
#include "ShaderBindingTable.h"

#include <unordered_map>
#include <unordered_set>

#include <iostream>


using namespace std;
#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

struct GBuffer
{
	VGM::Framebuffer framebuffer;

	VGM::Texture colorBuffer;
	VkImageView colorView;
	VGM::Texture positionBuffer;
	VkImageView positionView;
	VGM::Texture normalBuffer;
	VkImageView normalView;
	VGM::Texture idBuffer;
	VkImageView idView;
	VGM::Texture depthBuffer;
	VkImageView depthView;
	VGM::Texture roughnessMetalnessBuffer;
	VkImageView roughnessMetalnessView;
};

struct DefferedBuffer
{
	VGM::Framebuffer framebuffer;

	VGM::Texture colorBuffer;
	VkImageView colorView;
};

struct RaytraceBuffer
{
	VGM::Texture colorBuffer;
	VkImageView colorView;
};

struct HistoryBuffer
{
	std::vector<VGM::Texture> velocityHistoryBuffer;
	std::vector<VkImageView> velocityViews;

	std::vector<VGM::Texture> idHistoryBuffer;
	std::vector<VkImageView> idViews;

	VGM::Texture priorPositionBuffer;
	VkImageView priorPositionView;

	uint32_t currentIndex = 0;
};

struct MeshBufferAddressData
{
	VkDeviceAddress vertexAddress = 0;
	VkDeviceAddress instanceAddress = 0;
};

struct MeshBuffer
{
	VGM::Buffer vertices;
	VGM::Buffer indices;
	VGM::Buffer addressBuffer;
};

struct AccelerationStructure
{
	std::vector<BLAS> bottomLevelAccelStructures;
	std::vector<TLAS> topLevelAccelStructures;
	std::vector<VGM::Buffer> instanceBuffers;
	std::vector<VkAccelerationStructureInstanceKHR> _instanceTransferCache;
};

struct Material
{
	uint32_t albedoIndex = 0;
	uint32_t metallicIndex = 0;
	uint32_t normalIndex = 0;
	uint32_t roughnessIndex = 0;
};

struct Mesh
{
	uint32_t indexOffset = 0;
	uint32_t vertexOffset = 0;
	uint32_t indicesCount = 0;
	uint32_t vertexCount = 0;
	uint32_t blasIndex = 0;
	Material material = {};
};

struct MeshInstance
{
	uint32_t blasIndex;
	uint32_t meshIndex;
	Material material;
	uint32_t instanceID;
	glm::mat4 transform = glm::mat4(1.0f);
};

struct DrawData
{
	glm::mat4 modelMatrix;
	Material material;
	uint32_t ID;
	uint32_t vertexOffset;
	uint32_t indicesOffset;
	uint32_t padding;
};

struct Model
{
	std::vector<Mesh> meshes;
};

struct GlobalRenderData
{
	float fog;
	uint32_t sunLightCount;
	uint32_t pointLightCount;
	uint32_t spotLightCount;
	uint32_t maxRecoursionDepth;
	uint32_t maxDiffuseSampleCount;
	uint32_t maxSpecularSampleCount;
	uint32_t maxShadowRaySampleCount;
	uint32_t noiseSampleTextureIndex;
	uint32_t sampleSequenceLength;
	uint32_t frameNumber;
	uint32_t historyLength;
};

struct CameraData
{
	glm::mat4 viewMatrix;
	glm::mat4 inverseViewMatrix;
	glm::mat4 projectionMatrix;
	glm::mat4 inverseProjectionMatrix;
	glm::vec3 cameraPosition;
};

struct FrameSynchro
{
	VkSemaphore _offsceenRenderSemaphore;
	VkSemaphore _defferedRenderSemaphore;
	VkSemaphore _raytraceSemaphore;
	VkSemaphore _compositingSemaphore;
	VkSemaphore _presentSemaphore;
	VkFence _offsrceenRenderFence;
	VkFence _defferedRenderFence;
	VkFence _raytraceFence;
	VkFence _compositingFence;

	VkEvent _defferedFinishedEvent;
	VkEvent _raytraceFinishedEvent;
};

struct SunLight
{
	glm::vec3 direction;
	float padding0;
	glm::vec3 color;
	float strength;
};

struct PointLight
{
	glm::vec3 position;
	float diameter;
	glm::vec3 color;
	float strength;
};

struct SpotLight
{
	glm::vec3 position;
	glm::vec3 direction;
	float openingAngle;
	glm::vec4 color;
	float strength;
};

class Raytracer
{
public:
	Raytracer() = default;
	void init(SDL_Window* window, uint32_t windowWidth, uint32_t windowHeight);
	void resizeSwapchain(uint32_t windowWidth, uint32_t windowHeight);

	Mesh loadMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, const std::string& name);
	uint32_t loadTexture(std::vector<unsigned char>& pixels, uint32_t width, uint32_t height, uint32_t nr_channels, const std::string& name);

	MeshInstance getMeshInstance(const std::string& name);

	void uploadMeshes();
	void uploadTextures();

	//update tlas and place draw call;
	void drawMeshInstance(MeshInstance instance, glm::mat4 transform);
	void setNoiseTextureIndex(uint32_t index);


	void drawMesh(Mesh mesh, glm::mat4 transform, uint32_t objectID);
	void drawSunLight(SunLight light);
	void drawPointLight(PointLight light);
	void drawSpotLight(SpotLight light);
	void setCamera(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, glm::vec3 position);
	void update();
	void destroy();

private:
	void initDescriptorSetAllocator();
	void initDescriptorSetLayouts();
	void initDescripotrSets();
	void initGBufferShader();
	void initDefferedShader();
	void initRaytraceShader();
	void initCompositingShader();
	void initCommandBuffers();
	void initSyncStructures();
	void initDataBuffers();
	void initLightBuffers();

	void initMeshBuffer();
	void initgBuffers();
	void initDefferedBuffers();
	void initRaytraceBuffers();
	void initHistoryBuffers();
	void initPresentFramebuffers();
	void initTextureArrays();
	void initShaderBindingTable();

	void loadPlaceholderMeshAndTexture();

	void updateGBufferDescriptorSets();
	void updateDefferedDescriptorSets();
	void updateRaytraceDescripotrSets();

	void updateCompositingDescriptorSets();

	void updateGBufferFramebufferBindings();
	void updateCompositingFramebufferBindings();

	void executeDefferedPass();
	void executeRaytracePass();
	void executeCompositPass();

	float genHaltonSample(uint32_t base, uint32_t index);
	void genHaltonSequence(uint32_t a, uint32_t b, uint32_t length);

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR _raytracingProperties;

	VGM::VulkanContext _vulkan;
	uint32_t _windowWidth;
	uint32_t _windowHeight;

	uint32_t nativeRenderingReselutionX;
	uint32_t nativeRenderingReselutionY;

	uint32_t _concurrencyCount = 2;
	uint32_t _maxDrawCount = 100000;
	uint32_t _maxTextureCount = 1024;
	uint32_t _maxTriangleCount = 12000000;
	uint32_t _maxMipMapLevels = 4;
	uint64_t _timeout = 100000000;
	
	uint32_t _maxSunLights = 10;
	uint32_t _maxPointLighst = 10;
	uint32_t _maxSpotLights = 10;
	
	uint32_t _maxRecoursionDepth = 2;
	uint32_t _diffuseSampleCount = 1;
	uint32_t _specularSampleCount = 1;
	uint32_t _shadowSampleCount = 4;
	uint32_t _sampleSequenceLength = 8;
	uint32_t _historyLength = 1;

	uint32_t nativeWidth = 1920/2;
	uint32_t nativeHeight = 1080/2;

	uint32_t _frameNumber = 0;

	std::vector<VGM::Texture> _textures;
	std::vector<VkImageView> _views;
	std::unordered_map<std::string, uint32_t> _loadedImages;

	std::vector<Mesh> _meshes;
	std::unordered_map<std::string, uint32_t> _loadedMeshes;
	
	MeshBuffer _meshBuffer;
	AccelerationStructure _accelerationStructure;

	uint32_t _activeInstanceCount = 0;

	VGM::ShaderProgram _gBufferShader;
	VkPipelineLayout _gBufferPipelineLayout;
	std::vector<GBuffer> _gBufferChain;

	VGM::ShaderProgram _defferedShader;
	VkPipelineLayout _defferedPipelineLayout;
	std::vector<DefferedBuffer> _defferdBufferChain;

	RaytracingShader _raytraceShader;
	VkPipelineLayout _raytracePipelineLayout;
	ShaderBindingTable _shaderBindingTable;
	std::vector<RaytraceBuffer> _raytraceBufferChain;
	
	VGM::ShaderProgram _compositingShader;
	VkPipelineLayout _compositingPipelineLayout;
	std::vector<VGM::Framebuffer> _presentFramebuffers;

	HistoryBuffer _historyBuffer;

	VGM::DescriptorSetAllocator _descriptorSetAllocator;

	uint32_t _currentFrameIndex = 1;
	uint32_t _currentSwapchainIndex = 0;

	VGM::CommandBufferAllocator _renderCommandBufferAllocator;
	std::vector<VGM::CommandBuffer> _offsceenRenderCommandBuffers;
	std::vector<VGM::CommandBuffer> _defferedRenderCommandBuffers;
	std::vector<VGM::CommandBuffer> _compositingCommandBuffers;

	VGM::CommandBufferAllocator _computeCommandBufferAllocator;
	std::vector<VGM::CommandBuffer> _raytraceRenderCommandBuffers;
	VGM::CommandBuffer _raytraceBuildCommandBuffer;

	VGM::CommandBufferAllocator _transferCommandBufferAllocator;
	VGM::CommandBuffer _transferCommandBuffer;

	std::vector<FrameSynchro> _frameSynchroStructs;
	

	std::vector<VGM::Buffer> _cameraBuffers;
	std::vector<VGM::Buffer> _globalRenderDataBuffers;
	std::vector<VGM::Buffer> _drawDataIsntanceBuffers;
	std::vector<VGM::Buffer> _drawIndirectCommandBuffer;
	
	std::vector<VGM::Buffer> _instanceIndexBuffers;
	std::vector<VGM::Buffer> _instanceIndexTransferBuffers;
	VGM::Buffer _sampleSequenceBuffer;

	std::vector<VGM::Buffer> _sunLightBuffers;
	std::vector<VGM::Buffer> _pointLightBuffers;
	std::vector<VGM::Buffer> _spotLightBuffers;

	std::vector<VkDrawIndexedIndirectCommand> _drawCommandTransferCache;
	std::vector<DrawData> _drawDataTransferCache;

	std::vector<SunLight> _sunLightTransferCache;
	std::vector<PointLight> _pointLightTransferCache;
	std::vector<SpotLight> _spotLightTransferCache;

	CameraData _cameraData = {glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::vec3(0.0f)};
	GlobalRenderData _globalRenderData = { 0.0f, 0, 0, 0 };

	VGM::DescriptorSetAllocator _offsecreenDescriptorSetAllocator;
	VGM::DescriptorSetAllocator _textureDescriptorSetAllocator;
	VGM::DescriptorSetAllocator _defferedDescriptorSetAllocator;
	VGM::DescriptorSetAllocator _raytracerDescriptorSetAllocator;
	VGM::DescriptorSetAllocator _compositingDescriptorSetAllocator;

	VkDescriptorSetLayout _globalLayout;
	VkDescriptorSetLayout _gBufferLayout1;

	VkDescriptorSetLayout _gBufferLayout2;

	VkDescriptorSetLayout _textureLayout;

	VkDescriptorSetLayout _defferedLayout;

	VkDescriptorSetLayout _lightLayout;

	VkDescriptorSetLayout _raytracerLayout1;
	VkDescriptorSetLayout _raytracerLayout2;

	VkDescriptorSetLayout _compositingLayout;

	std::vector<VkDescriptorSet> _globalDescriptorSets;

	std::vector<VkDescriptorSet> _gBuffer1DescripotrSets;
	std::vector<VkDescriptorSet> _gBuffer2DescriptorSets;

	std::vector<VkDescriptorSet> _textureDescriptorSets;

	std::vector<VkDescriptorSet> _defferedDescriptorSets;

	std::vector<VkDescriptorSet> _lightDescripotrSets;

	std::vector<VkDescriptorSet> _raytracer1DescriptorSets;
	std::vector<VkDescriptorSet> _raytracer2DescriptorSets;

	std::vector<VkDescriptorSet> _compositingDescriptorSets;

	Mesh _dummyMesh;
};
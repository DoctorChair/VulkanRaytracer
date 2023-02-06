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

struct GBuffer
{
	VGM::Framebuffer framebuffer;

	VGM::Texture positionBuffer;
	VkImageView positionView;
	VGM::Texture colorBuffer;
	VkImageView colorView;
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

struct MeshBuffer
{
	VGM::Buffer vertices;
	VGM::Buffer indices;
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
};

struct DrawData
{
	glm::mat4 modelMatrix;
	Material material;
	uint32_t ID;
	uint32_t padding[3];
};

struct Model
{
	std::vector<Mesh> meshes;
};

struct GlobalRenderData
{
	float fog;
	uint32_t sunLightCount = 0;
	uint32_t pointLightCout = 0;
	uint32_t spotLightCount = 0;
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
	VkSemaphore _availabilitySemaphore;
	VkSemaphore _presentSemaphore;
	VkSemaphore _offsceenRenderSemaphore;
	VkSemaphore _defferedRenderSemaphore;
	VkSemaphore _raytraceSemaphore;
	VkSemaphore _compositingSemaphore;
	
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
	glm::vec4 color;
	float padding;
};

struct PointLight
{
	glm::vec3 position;
	glm::vec4 color;
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
	void init(SDL_Window* window);

	Mesh loadMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, const std::string& name);
	uint32_t loadTexture(std::vector<unsigned char> pixels, uint32_t width, uint32_t height, uint32_t nr_channels, const std::string& name);

	MeshInstance getMeshInstance(const std::string& name);

	void uploadMeshes();
	void uploadTextures();

	//update tlas and place draw call;
	void drawMeshInstance(MeshInstance instance, glm::mat4 transform);

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
	void initPresentFramebuffers();
	void initTextureArrays();
	void initShaderBindingTable();

	void loadPlaceholderMeshAndTexture();

	void updateGBufferDescriptorSets();
	void updateDefferedDescriptorSets();
	void updateRaytraceDescripotrSets();
	void updateCompositingDescriptorSets();

	void executeDefferedPass();
	void executeRaytracePass();
	void executeCompositPass();

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR _raytracingProperties;

	VGM::VulkanContext _vulkan;
	uint32_t windowWidth;
	uint32_t windowHeight;

	uint32_t nativeRenderingReselutionX;
	uint32_t nativeRenderingReselutionY;

	uint32_t _concurrencyCount = 3;
	uint32_t _maxDrawCount = 100000;
	uint32_t _maxTextureCount = 1024;
	uint32_t _maxTriangleCount = 12000000;
	uint32_t _maxMipMapLevels = 4;
	uint64_t _timeout = 100000000;
	uint32_t _maxSunLights = 10;
	uint32_t _maxPointLighst = 10;
	uint32_t _maxSpotLights = 10;
	uint32_t _maxRecoursionDepth = 2;

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

	VGM::DescriptorSetAllocator _descriptorSetAllocator;

	uint32_t _currentFrameIndex = 0;
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
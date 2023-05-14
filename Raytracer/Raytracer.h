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
#include "ComputeShaderProgram.h"

#include "glm/glm.hpp"

#include "Vertex.h"

#include "BLAS.h"
#include "TLAS.h"
#include "RaytracingShader.h"
#include "ShaderBindingTable.h"

#include <unordered_map>
#include <unordered_set>

#include <iostream>
#include <functional>

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
	VGM::Texture depthBuffer;
	VkImageView depthView;
	VGM::Texture roughnessMetalnessBuffer;
	VkImageView roughnessMetalnessView;
	VGM::Texture emissionBuffer;
	VkImageView emissionView;
};

struct DefferedBuffer
{
	VGM::Framebuffer framebuffer;

	VGM::Texture colorBuffer;
	VkImageView colorView;
};


struct HistoryBuffer
{
	std::vector<VGM::Texture> velocityHistoryBuffer;
	std::vector<VkImageView> velocityViews;

	std::vector<VGM::Texture> radianceHistoryBuffer;
	std::vector<VkImageView> radianceViews;
	
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
	uint32_t emissionIndex = 0;
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
	glm::mat4 previousTransform = glm::mat4(1.0f);
};

struct DrawData
{
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	glm::mat4 previousModelMatrix = glm::mat4(1.0f);
	Material material;
	uint32_t ID;
	uint32_t vertexOffset;
	uint32_t indicesOffset;
	float emissionIntensity = 0.0;
	float padding[3];
};

struct Model
{
	std::vector<Mesh> meshes;
};

struct GlobalRenderData
{
	uint32_t sunLightCount;
	uint32_t pointLightCount;
	uint32_t maxRecoursionDepth;
	uint32_t maxDiffuseSampleCount;
	uint32_t maxSpecularSampleCount;
	uint32_t maxShadowRaySampleCount;
	uint32_t noiseSampleTextureIndex;
	uint32_t frameNumber;
	uint32_t historyLength;
	uint32_t historyIndex;
	uint32_t nativeResolutionWidth;
	uint32_t nativeResolutionHeight;
	uint32_t environmentTextureIndex;
	float heurisitcExponent = 1.0f;
};

struct CameraData
{
	glm::mat4 viewMatrix;
	glm::mat4 inverseViewMatrix;
	glm::mat4 projectionMatrix;
	glm::mat4 inverseProjectionMatrix;
	glm::vec3 cameraPosition;
	float padding;
	glm::mat4 previousProjectionMatrix = glm::mat4(1.0f);
	glm::mat4 previousViewMatrix = glm::mat4(1.0f);
};

struct FrameSynchro
{
	VkSemaphore _defferedRenderSemaphore;
	VkSemaphore _raytraceSemaphore;
	VkSemaphore _postProcessSemaphore;
	VkSemaphore _overlapSemaphore;
	VkSemaphore _guiSemaphore;
	VkSemaphore _presentSemaphore;
	VkFence _frameFence;
};

struct SunLight
{
	glm::vec3 direction;
	float radius;
	glm::vec3 color;
	float strength;
};

struct PointLight
{
	glm::vec3 position;
	float radius;
	glm::vec3 color;
	float strength;
	uint32_t emissionIndex;
	uint32_t padding[3];
};

struct SpotLight
{
	glm::vec3 position;
	glm::vec3 direction;
	float openingAngle;
	glm::vec4 color;
	float strength;
};

struct PointLightSourceInstance
{
	glm::vec3 position;
	float radius;
	glm::vec3 color;
	float strength;
	MeshInstance lightModel;
};

struct PostProcessLeapfrogBuffer
{
	std::vector<VGM::Texture> textures;
	std::vector<VkImageView> views;
};

struct RaytracingBackbuffer
{
	VGM::Texture directIluminationBackbuffer;
	VkImageView directIluminationView;
	VGM::Texture indirectIluminationBackbuffer;
	VkImageView indirectIluminationView;
};

struct PipelineMeasurements
{
	float timestampPeriod = 0.0f;
	std::vector<VkQueryPool> frameQuerryPools;
	std::vector<uint64_t> _gBufferPassIntervals;
	std::vector<uint64_t> _raytracePassIntervals;
	std::vector<uint64_t> _postProcessPassIntervals;
	std::vector<uint64_t> _guiPassIntervals;
	std::vector<uint64_t> _timesScale;
};
 
struct Scene
{
	std::vector<MeshInstance>* meshInstances = nullptr;
	std::vector<PointLight> pointLights;
	std::vector<SunLight> directionalLights;
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
	void drawMeshInstance(MeshInstance instance, uint32_t cullMask);
	void drawLightMeshInstance(MeshInstance meshInstance, uint32_t cullMask, float intenisty);
	void setBlueNoiseTextureIndex(uint32_t index);
	void setRandomValueTextureIndex(uint32_t index);
	void setEnvironmentTextureIndex(uint32_t index);

	void drawMesh(Mesh mesh, glm::mat4 transform, uint32_t objectID);
	void drawSunLight(SunLight light);
	void drawPointLight(PointLightSourceInstance light);
	void drawSpotLight(SpotLight light);
	void setCamera(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, glm::vec3 position);
	void drawDebugGui(SDL_Window* window);
	void hideGui();
	void handleGuiEvents(SDL_Event* event);
	void update();
	void registerLoadSceneCallback(std::function<std::vector<MeshInstance>* (const std::string& path, Raytracer& raytracer)> callback);

	void destroy();

private:
	void initGui(SDL_Window* window, int width, int height);
	void initQueryPool();
	void initDescriptorSetAllocator();
	void initDescriptorSetLayouts();
	void initDescripotrSets();
	void initGBufferShader();
	void initDefferedShader();
	void initRaytraceShader();
	void initAccumulateShader();
	void initPostProcessingChain();
	void initCommandBuffers();
	void initSyncStructures();
	void initDataBuffers();
	void initLightBuffers();
	void initMeshBuffer();
	void initgBuffers();
	void initDefferedBuffers();
	void initRaytraceBuffers();
	void initRaytracingBackBuffers();
	void initHistoryBuffers();
	void initPostProcessBuffers();
	void initPresentFramebuffers();
	void initTextureArrays();
	void initShaderBindingTable();

	void initBufferState();

	void loadPlaceholderMeshAndTexture();

	void initSamplers();

	void updateGBufferDescriptorSets();
	void updateDefferedDescriptorSets();
	void updateRaytraceDescripotrSets();
	void updateCompositingDescriptorSets();
	void updatePostProcessDescriptorSets();

	void updateGBufferFramebufferBindings();
	void updateCompositingFramebufferBindings();

	void updateDescriptorSets();

	void executeDefferedPass();
	void executeRaytracePass();
	void executeSeperatedRaytracePass();
	void executeAccumulatePass();
	void executePostProcessPass();
	void executeGuiPass();

	void loadScene(const std::string& path);
	void drawScene();

	void getTimestamps();
	void writeToCSV(std::string path, unsigned int length, unsigned int start);
	

private:
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR _raytracingProperties;

	VGM::VulkanContext _vulkan;
	uint32_t _windowWidth;
	uint32_t _windowHeight;

	uint32_t nativeRenderingReselutionX;
	uint32_t nativeRenderingReselutionY;

	uint32_t _concurrencyCount = 2;
	uint32_t _frameMeasurementPeriod = 10000;
	uint32_t  _timestampCaptureLength = 0;
	uint32_t _maxDrawCount = 100000;
	uint32_t _maxTextureCount = 1024;
	uint32_t _maxTriangleCount = 12000000;
	uint32_t _maxMipMapLevels = 3;
	uint64_t _timeout = UINT64_MAX;
	uint32_t _maxPostProcessChainLength = 4;
	
	uint32_t _maxSunLights = 10;
	uint32_t _maxPointLighst = 10;
	uint32_t _maxSpotLights = 10;
	
	uint32_t _maxRecoursionDepth = 2;
	uint32_t _diffuseSampleCount = 1;
	uint32_t _specularSampleCount = 1;
	uint32_t _shadowSampleCount = 1;
	uint32_t _historyLength = 32;
	uint32_t _currentBackProjectionLength = 1;
	float _heuristicExponent = 1.0;

	float _radius = 1.0f;
	float _intensity = 100.0f;
	float _thetaSun = 0.0f;
	float _PhiSun = 0.0f;

	uint32_t _blueNoiseSampleIndex = 0;
	uint32_t _randomUniformSampleIndex = 0;
	bool _useBlueNoise = false;

	uint32_t nativeWidth = 1920;
	uint32_t nativeHeight = 1080;

	uint32_t _frameNumber = 0;

	bool drawIluminatenSeperated = true;
	bool drawGUI = true;

	PipelineMeasurements _measurments;
	bool _captureActive = false;
	unsigned int _captureIndex = 0;
	unsigned int _capturePeriodLength = 1;

	std::vector<VGM::Texture> _textures;
	std::vector<VkImageView> _views;
	std::unordered_map<std::string, uint32_t> _loadedImages;

	std::vector<Mesh> _meshes;
	std::unordered_map<std::string, uint32_t> _loadedMeshes;
	
	MeshBuffer _meshBuffer;
	AccelerationStructure _accelerationStructure;

	HistoryBuffer _historyBuffer;

	uint32_t _activeInstanceCount = 0;

	VGM::ShaderProgram _gBufferShader;
	VkPipelineLayout _gBufferPipelineLayout;
	std::vector<GBuffer> _gBufferChain;

	VGM::ShaderProgram _defferedShader;
	VkPipelineLayout _defferedPipelineLayout;
	std::vector<DefferedBuffer> _defferdBufferChain;

	std::vector<RaytracingBackbuffer> _raytracingBackbuffers;

	RaytracingShader _raytraceShader;
	VkPipelineLayout _raytracePipelineLayout;
	ShaderBindingTable _shaderBindingTable;
	
	VGM::ComputeShaderProgram _combineBackBufferShader;

	RaytracingShader _directIluminationShader;
	ShaderBindingTable _directIluminationSBT;
	RaytracingShader _indirectIluminationShader;
	ShaderBindingTable _indirectIluminationSBT;

	VkRenderPass _guiRenderpass;
	std::vector<VkFramebuffer> _guiFramebuffers;
	VkDescriptorPool _guiPool;
	bool _guiActive;

	VkPipelineLayout _postProcessPipelineLayout;
	std::vector<VGM::ComputeShaderProgram> _postProcessingChain;
	VGM::ComputeShaderProgram _colorMapingShader;
	PostProcessLeapfrogBuffer _leapFrogBuffer;

	VGM::DescriptorSetAllocator _descriptorSetAllocator;

	uint32_t _currentFrameIndex = 0;
	uint32_t _currentSwapchainIndex = 0;

	VGM::CommandBufferAllocator _renderCommandBufferAllocator;
	std::vector<VGM::CommandBuffer> _guiCommandBuffers;
	std::vector<VGM::CommandBuffer> _defferedRenderCommandBuffers;
	std::vector<VGM::CommandBuffer> _postProcessCommandBuffers;

	VGM::CommandBufferAllocator _computeCommandBufferAllocator;
	std::vector<VGM::CommandBuffer> _raytraceRenderCommandBuffers;
	VGM::CommandBuffer _raytraceBuildCommandBuffer;

	VGM::CommandBufferAllocator _transferCommandBufferAllocator;
	VGM::CommandBuffer _transferCommandBuffer;

	std::vector<FrameSynchro> _frameSynchroStructs;
	
	VkSampler _linearSampler;
	VkSampler _nearestNeighborSampler;
	VkSampler _lowFidelitySampler;

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
	GlobalRenderData _globalRenderData = { 0, 0, 0 };

	VGM::DescriptorSetAllocator _offsecreenDescriptorSetAllocator;
	VGM::DescriptorSetAllocator _textureDescriptorSetAllocator;
	VGM::DescriptorSetAllocator _defferedDescriptorSetAllocator;
	VGM::DescriptorSetAllocator _raytracerDescriptorSetAllocator;
	VGM::DescriptorSetAllocator _postProcessDescriptorSetAllocator;

	VkDescriptorSetLayout _globalLayout;
	VkDescriptorSetLayout _drawInstanceLayout;

	VkDescriptorSetLayout _samplerLayout;

	VkDescriptorSetLayout _textureLayout;

	VkDescriptorSetLayout _lightLayout;

	VkDescriptorSetLayout _raytracerLayout1;
	VkDescriptorSetLayout _raytracerLayout2;

	VkDescriptorSetLayout _postProcessLayout;
	VkDescriptorSetLayout _postProcessInputOutputLayout;

	std::vector<VkDescriptorSet> _globalDescriptorSets;

	std::vector<VkDescriptorSet> _drawInstanceDescriptorSets;
	std::vector<VkDescriptorSet> _samplerDescriptorSets;

	std::vector<VkDescriptorSet> _textureDescriptorSets;

	std::vector<VkDescriptorSet> _defferedDescriptorSets;

	std::vector<VkDescriptorSet> _lightDescripotrSets;

	std::vector<VkDescriptorSet> _raytracer1DescriptorSets;
	std::vector<VkDescriptorSet> _raytracer2DescriptorSets;

	std::vector<VkDescriptorSet> _postProcessDescriptorSets;

	std::vector<VkDescriptorSet> _postProcessInputOutputDescriptorSets;

	Mesh _dummyMesh;

	Scene _scene;

	std::function<std::vector<MeshInstance>* (const std::string& path, Raytracer& raytracer)> _loadSceneCallback;
};
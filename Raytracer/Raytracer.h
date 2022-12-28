#pragma once
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
	VGM::Texture normalBuffer;
	VGM::Texture idBuffer;
	VGM::Texture albedoBuffer;
};

struct VertexBuffer
{
	VGM::Buffer vertices;
	VGM::Buffer indices;
};

struct Material
{

};

struct Mesh
{
	Material material;
};

struct Model
{

};

struct globalRenderData
{
	
};

struct CameraData
{
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	glm::mat4 cameraMatrix;
	glm::vec3 cameraPosition;
};

class Raytracer
{
public:


private:
	VGM::VulkanContext renderContext;
	SDL_Window* window;

	VGM::Texture _albedoTextureArray;
	VGM::Texture _metallicTextureArray;
	VGM::Texture _normalTextureArray;
	VGM::Texture _roughnessTextureArray;

	VertexBuffer _vertexBuffer;

	VGM::ShaderProgram _gBufferShader;
	VkPipelineLayout _gBufferPipelineLayout;
	std::vector<GBuffer> _gBufferChain;

	VGM::DescriptorSetAllocator _descriptorSetAllocator;

};
#pragma once

#include <string>
#include <vector>
#include "glm/glm.hpp"
#include "Vertex.h"

struct pbrMaterialData
{
	std::string albedo;
	std::string metallic;
	std::string normal;
	std::string roughness;
};

struct TextureData
{
	uint32_t nrChannels;
	uint32_t width;
	uint32_t height;
	std::vector<unsigned char> pixels;
};

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	glm::mat4 transform;
	pbrMaterialData material;
	std::string name;
};

struct ModelData
{
	std::vector<MeshData> meshes;
};
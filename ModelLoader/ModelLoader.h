#pragma once
#include "glm/glm.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

	pbrMaterialData material;

	std::string name;
};

struct ModelData
{
	std::vector<MeshData> meshes;
};

class ModelLoader
{
public:
	ModelLoader() = default;
	ModelData loadModel(const std::string& path);
	TextureData* getTextureData(const std::string& texture);
	void freeAssets();

private:
	void handleNode(aiNode* node, const aiScene* scene, ModelData& model, const std::string& root);
	MeshData copyMeshData(aiMesh* mesh, const aiScene* scene, const std::string& root);
	std::string getTextureSubPath(aiMaterial* material, aiTextureType type);
	void loadTextureData(std::string& path);

	std::unordered_map <std::string, ModelData> _loadedModels;
	std::unordered_map <std::string, TextureData> _loadedTextures;
};
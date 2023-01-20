#pragma once
#include "glm/glm.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Vertex.h"
#include "DataStructs.h"

class ModelLoader
{
public:
	ModelLoader() = default;
	ModelData loadModel(const std::string& path);
	TextureData* getTextureData(const std::string& texture);
	void freeAssets();

private: 
	void handleNode(aiNode* node, const aiScene* scene, ModelData& model, const std::string& root, glm::mat4 transfrom);
	MeshData copyMeshData(aiMesh* mesh, const aiScene* scene, const std::string& root, glm::mat4 transform);
	std::string getTextureSubPath(aiMaterial* material, aiTextureType type);

	std::unordered_map <std::string, ModelData> _loadedModels;
	std::unordered_map <std::string, TextureData> _loadedTextures;
};


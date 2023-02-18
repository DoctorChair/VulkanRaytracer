#pragma once
#include "glm/glm.hpp"
#include <vector>
#include <unordered_map>


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "DataStructs.h"


class TextureLoader
{
public:
	TextureLoader() = default;
	void operator()(const aiScene* scene, const std::string& root, aiTextureType type, std::unordered_map<std::string, TextureData>* textureMap);
	TextureData loadTexture(const std::string& path);

private:
	std::string getTextureSubPath(aiMaterial* material, aiTextureType type);
	void loadSceneTextures(const aiScene* scene, const std::string& root, aiTextureType type, std::unordered_map<std::string, TextureData>* textureMap);
	void loadTextureData(std::string& path, std::unordered_map<std::string, TextureData>* textureMap);
};
#include "TextureLoader.h"
#include <filesystem>
#include <regex>
#include <thread>


#include "stb_image.h"

#include <iostream>

void TextureLoader::operator()(const aiScene* scene, const std::string& root, aiTextureType type, std::unordered_map<std::string, TextureData>* textureMap)
{
	loadSceneTextures(scene, root, type, textureMap);
}


void TextureLoader::loadSceneTextures(const aiScene* scene, const std::string& root, aiTextureType type, std::unordered_map<std::string, TextureData>* textureMap)
{
	for (unsigned int i = 0; i < scene->mNumMaterials; i++)
	{
		std::string subpath = getTextureSubPath(scene->mMaterials[i], type);
		if (subpath != "")
		{
			std::string path = root + "/" + subpath;
			loadTextureData(path, textureMap);
		}
	}
}

TextureData TextureLoader::loadTexture(const std::string& path)
{
	TextureData textureData;

	int width, height, nrChannels;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
	if (data != nullptr)
	{
		textureData.height = static_cast<unsigned int>(height);
		textureData.width = static_cast<unsigned int>(width);
		textureData.nrChannels = static_cast<unsigned int>(nrChannels);
		textureData.pixels.resize(textureData.width * textureData.height * textureData.nrChannels);
		memcpy(textureData.pixels.data(), data, textureData.width * textureData.height * textureData.nrChannels * sizeof(unsigned char));
		std::cout << "Loaded texture: " << path << "\n";
	}

	return textureData;
}

std::string TextureLoader::getTextureSubPath(aiMaterial* material, aiTextureType type)
{
	std::string texturePath;

	aiString string;
	material->GetTexture(type, 0, &string);

	if (string.length > 0)
		texturePath = string.C_Str();

	return texturePath;
}

void TextureLoader::loadTextureData(std::string& path, std::unordered_map<std::string, TextureData>* textureMap)
{
	TextureData textureData;

	std::string identifier = std::filesystem::path(path).filename().string();
	if (textureMap->find(identifier) == textureMap->end())
	{
		path = std::regex_replace(path, std::regex("/"), "\\");
		int width, height, nrChannels;
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
		if (data != nullptr)
		{
			textureData.height = static_cast<unsigned int>(height);
			textureData.width = static_cast<unsigned int>(width);
			textureData.nrChannels = static_cast<unsigned int>(nrChannels);
			textureData.pixels.resize(textureData.width * textureData.height * textureData.nrChannels);
			memcpy(textureData.pixels.data(), data, textureData.width * textureData.height * textureData.nrChannels * sizeof(unsigned char));
			std::cout << "Loaded texture: " << identifier << "\n";
			textureMap->insert(std::make_pair(identifier, textureData));
		}
	}

	else return;
}
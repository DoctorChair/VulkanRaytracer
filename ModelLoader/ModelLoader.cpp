#include "ModelLoader.h"
#include <filesystem>
#include <regex>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

ModelData ModelLoader::loadModel(const std::string& path)
{
	ModelData modelData;

	if (_loadedModels.find(std::filesystem::path(path).filename().string()) == _loadedModels.end())
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

		if (scene != nullptr)
		{
			std::string root = std::filesystem::path(path).parent_path().string();
			handleNode(scene->mRootNode, scene, modelData, root);
		}
		else
		{
			return modelData;
		}

		_loadedModels.insert(std::make_pair(std::filesystem::path(path).filename().string(), modelData));
	}

	return _loadedModels[std::filesystem::path(path).filename().string()];
}

void ModelLoader::handleNode(aiNode* node, const aiScene* scene, ModelData& model, const std::string& root)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		unsigned int index = node->mMeshes[i];
		MeshData meshData = copyMeshData(scene->mMeshes[index], scene, root);
		model.meshes.push_back(meshData);
	}

	for(unsigned int i = 0; i<node->mNumChildren; i++)
	{
		handleNode(node->mChildren[i], scene, model, root);
	}
}

MeshData ModelLoader::copyMeshData(aiMesh* mesh, const aiScene* scene, const std::string& root)
{
	MeshData meshData;
	meshData.vertices.resize(mesh->mNumVertices);

	for(unsigned int i = 0; i<mesh->mNumVertices; i++)
	{
		Vertex vertex;

		vertex.position.x = mesh->mVertices[i].x;
		vertex.position.y = mesh->mVertices[i].y;
		vertex.position.z = mesh->mVertices[i].z;

		if (mesh->HasTextureCoords(0))
		{
			vertex.texCoord.x = mesh->mTextureCoords[0][i].x;
			vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
		}

		if(mesh->HasTangentsAndBitangents())
		{
			vertex.normal.x = mesh->mNormals[i].x;
			vertex.normal.y = mesh->mNormals[i].y;
			vertex.normal.z = mesh->mNormals[i].z;

			vertex.tangent.x = mesh->mTangents[i].x;
			vertex.tangent.y = mesh->mTangents[i].y;
			vertex.tangent.z = mesh->mTangents[i].z;
			
			vertex.bitangent.x = mesh->mBitangents[i].x;
			vertex.bitangent.y = mesh->mBitangents[i].y;
			vertex.bitangent.z = mesh->mBitangents[i].z;
		}

		meshData.vertices[i] = vertex;
	}

	for(unsigned int i = 0; i<mesh->mNumFaces; i++)
	{
		for(unsigned int j = 0; j<mesh->mFaces[j].mNumIndices; j++)
		{
			meshData.indices.push_back(mesh->mFaces[i].mIndices[j]);
		}
	}
	
	unsigned int materialIndex = mesh->mMaterialIndex;
	aiMaterial* material = scene->mMaterials[materialIndex];

	std::string subpath= getTextureSubPath(material, aiTextureType_DIFFUSE);
	if(subpath != "")
	{
		std::string path = root + "/" + subpath;
		loadTextureData(path);
		meshData.material.albedo = std::filesystem::path(subpath).filename().string();
	}

	subpath = getTextureSubPath(material, aiTextureType_METALNESS);
	if (subpath != "")
	{
		std::string path = root + "/" + subpath;
		loadTextureData(path);
		meshData.material.metallic = std::filesystem::path(subpath).filename().string();
	}

	subpath = getTextureSubPath(material, aiTextureType_NORMALS);
	if (subpath != "")
	{
		std::string path = root + "/" + subpath;
		loadTextureData(path);
		meshData.material.normal = std::filesystem::path(subpath).filename().string();
	}

	subpath = getTextureSubPath(material, aiTextureType_DIFFUSE_ROUGHNESS);
	if (subpath != "")
	{
		std::string path = root + "/" + subpath;
		loadTextureData(path);
		meshData.material.roughness = std::filesystem::path(subpath).filename().string();
	}

	return meshData;
}

std::string ModelLoader::getTextureSubPath(aiMaterial* material, aiTextureType type)
{
	std::string texturePath;

	aiString string;
	material->GetTexture(type, 0, &string);
	texturePath = string.C_Str();

	return texturePath;
}

void ModelLoader::loadTextureData(std::string& path)
{
	TextureData textureData;

	std::string identifier = std::filesystem::path(path).filename().string();
	if (_loadedTextures.find(identifier) == _loadedTextures.end())
	{
		path = std::regex_replace(path, std::regex("/"), "\\");
		int width, height, nrChannels;
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
		if(data != nullptr)
		{
			textureData.height = static_cast<unsigned int>(height);
			textureData.width = static_cast<unsigned int>(width);
			textureData.nrChannels = static_cast<unsigned int>(nrChannels);
			textureData.pixels.resize(textureData.width * textureData.height * textureData.nrChannels);
			memcpy(textureData.pixels.data(), data, textureData.width * textureData.height * textureData.nrChannels * sizeof(unsigned char));
			std::cout << "Loaded texture: " << identifier << "\n";
		}

		_loadedTextures.insert(std::make_pair(identifier, textureData));
	}
	else return;
}
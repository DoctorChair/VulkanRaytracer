#include "ModelLoader.h"
#include <filesystem>
#include <regex>
#include <thread>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

#include "TextureLoader.h"

ModelData ModelLoader::loadModel(const std::string& path)
{
	ModelData modelData;

	if (_loadedModels.find(std::filesystem::path(path).filename().string()) == _loadedModels.end())
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		
		if (scene != nullptr)
		{
			std::string root = std::filesystem::path(path).parent_path().string();

			std::unordered_map<std::string, TextureData> diffuseTextures;
			std::unordered_map<std::string, TextureData> normalTextures;
			std::unordered_map<std::string, TextureData> metallicTextures;
			std::unordered_map<std::string, TextureData> roughnessTextures;
			std::unordered_map<std::string, TextureData> emissionTextures;
			std::thread diffuseLoadThread(TextureLoader(), scene, root, aiTextureType_DIFFUSE, &diffuseTextures);
			std::thread normalLoadThread(TextureLoader(), scene, root, aiTextureType_NORMALS, &normalTextures);
			std::thread metallicLoadThread(TextureLoader(), scene, root, aiTextureType_METALNESS, &metallicTextures);
			std::thread roughnessoadThread(TextureLoader(), scene, root, aiTextureType_DIFFUSE_ROUGHNESS, &roughnessTextures);
			std::thread emissionThread(TextureLoader(), scene, root, aiTextureType_EMISSIVE, &emissionTextures);

			aiMatrix4x4 t = scene->mRootNode->mTransformation;
			glm::mat4 matrix;
			matrix[0] = { t.a1, t.b1, t.c1, t.d1 };
			matrix[1] = { t.a2, t.b2, t.c2, t.d2 };
			matrix[2] = { t.a3, t.b3, t.c3, t.d3 };
			matrix[3] = { t.a4, t.b4, t.c4, t.d4 };

			handleNode(scene->mRootNode, scene, modelData, root, matrix);

			diffuseLoadThread.join();
			normalLoadThread.join();
			metallicLoadThread.join();
			roughnessoadThread.join();
			emissionThread.join();

			_loadedTextures.merge(diffuseTextures);
			_loadedTextures.merge(normalTextures);
			_loadedTextures.merge(metallicTextures);
			_loadedTextures.merge(roughnessTextures);
			_loadedTextures.merge(emissionTextures);
		}
		else
		{
			return modelData;
		}
		
		_loadedModels.insert(std::make_pair(std::filesystem::path(path).filename().string(), modelData));
	}

	return _loadedModels[std::filesystem::path(path).filename().string()];
}

TextureData* ModelLoader::getTextureData(const std::string& texture)
{
	return &_loadedTextures.at(texture);
}

void ModelLoader::freeAssets()
{
	_loadedTextures.clear();
	_loadedModels.clear();
}

void ModelLoader::handleNode(aiNode* node, const aiScene* scene, ModelData& model, const std::string& root, glm::mat4 transfrom)
{
	aiMatrix4x4 t = node->mTransformation;
	glm::mat4 matrix;
	matrix[0] = { t.a1, t.b1, t.c1, t.d1 };
	matrix[1] = { t.a2, t.b2, t.c2, t.d2 };
	matrix[2] = { t.a3, t.b3, t.c3, t.d3 };
	matrix[3] = { t.a4, t.b4, t.c4, t.d4 };

	transfrom = transfrom * matrix;

	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		unsigned int index = node->mMeshes[i];

		MeshData meshData = copyMeshData(scene->mMeshes[index], scene, root, transfrom);
		model.meshes.push_back(meshData);
	}
	
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		handleNode(node->mChildren[i], scene, model, root, transfrom);
	}
}

MeshData ModelLoader::copyMeshData(aiMesh* mesh, const aiScene* scene, const std::string& root, glm::mat4 transform)
{
	MeshData meshData;
	meshData.vertices.resize(mesh->mNumVertices);

	meshData.transform = transform;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;

		vertex.position.x = mesh->mVertices[i].x;
		vertex.position.y = mesh->mVertices[i].y;
		vertex.position.z = mesh->mVertices[i].z;
		vertex.position.w = 1.0f;

		for (unsigned int j = 0; j < 1; j++)
		{
			if (mesh->HasTextureCoords(j))
			{
				vertex.texCoordPair.x = mesh->mTextureCoords[j][i].x;
				vertex.texCoordPair.y = mesh->mTextureCoords[j][i].y;
			}
		}
		if (mesh->HasTangentsAndBitangents())
		{
			vertex.normal.x = mesh->mNormals[i].x;
			vertex.normal.y = mesh->mNormals[i].y;
			vertex.normal.z = mesh->mNormals[i].z;

			vertex.tangent.x = mesh->mTangents[i].x;
			vertex.tangent.y = mesh->mTangents[i].y;
			vertex.tangent.z = mesh->mTangents[i].z;
		}

		meshData.vertices[i] = vertex;
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++)
		{
			meshData.indices.push_back(mesh->mFaces[i].mIndices[j]);
		}
	}

	unsigned int materialIndex = mesh->mMaterialIndex;
	aiMaterial* material = scene->mMaterials[materialIndex];

	std::string subpath = getTextureSubPath(material, aiTextureType_DIFFUSE);
	if (subpath != "")
	{
		meshData.material.albedo = std::filesystem::path(subpath).filename().string();
	}

	subpath = getTextureSubPath(material, aiTextureType_METALNESS);
	if (subpath != "")
	{
		meshData.material.metallic = std::filesystem::path(subpath).filename().string();
	}

	subpath = getTextureSubPath(material, aiTextureType_NORMALS);
	if (subpath != "")
	{
		meshData.material.normal = std::filesystem::path(subpath).filename().string();
	}

	subpath = getTextureSubPath(material, aiTextureType_DIFFUSE_ROUGHNESS);
	if (subpath != "")
	{
		meshData.material.roughness = std::filesystem::path(subpath).filename().string();
	}

	subpath = getTextureSubPath(material, aiTextureType_EMISSIVE);
	if (subpath != "")
	{
		meshData.material.emission = std::filesystem::path(subpath).filename().string();
	}

	meshData.name = mesh->mName.C_Str();

	return meshData;
}

std::string ModelLoader::getTextureSubPath(aiMaterial* material, aiTextureType type)
{
	std::string texturePath;

	aiString string;
	material->GetTexture(type, 0, &string);

	if (string.length > 0)
		texturePath = string.C_Str();

	return texturePath;
}
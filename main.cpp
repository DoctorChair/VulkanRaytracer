#include "VulkanContext.h"
#include "ModelLoader/ModelLoader.h"
#include "Raytracer/Raytracer.h"
#include "glm/gtc/matrix_transform.hpp"
#include "ModelLoader/TextureLoader.h"
#include <random>
#include "CameraController.h"

std::vector<MeshInstance>* loadSceneCallback(const std::string& path, Raytracer& raytracer)
{
	ModelLoader loader;

	ModelData scene = loader.loadModel(path);

	int i = 0;

	std::vector<Mesh> testScene;

	std::vector<MeshInstance>* instances = new std::vector<MeshInstance>;
	for (auto& m : scene.meshes)
	{
		testScene.push_back(raytracer.loadMesh(m.vertices, m.indices, m.name));

		if (!m.material.albedo.empty())
		{
			TextureData* data = loader.getTextureData(m.material.albedo);
			testScene.back().material.albedoIndex = raytracer.loadTexture(data->pixels, data->width, data->height, data->nrChannels, m.material.albedo);
		}
		if (!m.material.normal.empty())
		{
			TextureData* data = loader.getTextureData(m.material.normal);
			testScene.back().material.normalIndex = raytracer.loadTexture(data->pixels, data->width, data->height, data->nrChannels, m.material.normal);
		}
		if (!m.material.metallic.empty())
		{
			TextureData* data = loader.getTextureData(m.material.metallic);
			testScene.back().material.metallicIndex = raytracer.loadTexture(data->pixels, data->width, data->height, data->nrChannels, m.material.metallic);
		}
		if (!m.material.roughness.empty())
		{
			TextureData* data = loader.getTextureData(m.material.roughness);
			testScene.back().material.roughnessIndex = raytracer.loadTexture(data->pixels, data->width, data->height, data->nrChannels, m.material.roughness);
		}

		instances->push_back(raytracer.getMeshInstance(m.name));
		instances->back().material = testScene.back().material;
		instances->back().transform = m.transform;
	}

	loader.freeAssets();

	return instances;
}

TextureData createUniformValueTexture(unsigned int width, unsigned int height, unsigned int numChannels)
{
	TextureData texture;
	texture.width = width;
	texture.height = height;
	texture.nrChannels = numChannels;
	texture.pixels.resize(width * height * std::max(numChannels, (unsigned int)4), 0);
	for (int j = 0; j < numChannels && j<4; j++)
	{
		std::random_device rand;
		std::mt19937 generator(rand());
		std::uniform_real_distribution<> distribution(0.0, 1.0);
		for (int n = 0; n < width * height * numChannels - (numChannels-j); n = n + j + 1)
		{	
			unsigned char value = static_cast<unsigned char>(floor(distribution(generator) * 255.0));
			texture.pixels[n] = value;
		}
	}

	return texture;
}

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1920, 1080,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

	Raytracer raytracer;
	raytracer.init(window, 1920, 1080);
	std::function<std::vector<MeshInstance>* (const std::string& path, Raytracer& raytracer)> callback = loadSceneCallback;

	raytracer.registerLoadSceneCallback(callback);

	TextureData uniformRandomeValueTexture = createUniformValueTexture(1024, 1024, 4);

	TextureLoader textureLoader;
	TextureData noiseTexture = textureLoader.loadTexture("C:\\Users\\Eric\\projects\\textures\\FreeBlueNoiseTextures\\Data\\1024_1024\\LDR_RGBA_0.png");
	TextureData environmentTexture = textureLoader.loadTexture("C:\\Users\\Eric\\projects\\textures\\evening_meadow_4k.hdr");

	uint32_t noiseTexureIndex = raytracer.loadTexture(noiseTexture.pixels, noiseTexture.width, noiseTexture.height, noiseTexture.nrChannels, "blueNoiseSampleTexture");
	uint32_t uniformRandomNoise = raytracer.loadTexture(uniformRandomeValueTexture.pixels
		, uniformRandomeValueTexture.width,
		uniformRandomeValueTexture.height
		, uniformRandomeValueTexture.nrChannels
		, "unfirmValuesSampleTexture");
	uint32_t environmentTextureIndex = raytracer.loadTexture(environmentTexture.pixels, environmentTexture.width, environmentTexture.height, environmentTexture.nrChannels, "environmentMapTexture");

	raytracer.setBlueNoiseTextureIndex(noiseTexureIndex);
	raytracer.setRandomValueTextureIndex(uniformRandomNoise);
	raytracer.setEnvironmentTextureIndex(0);

	ModelLoader loader;
	
	//ModelData scene = loader.loadModel("C:\\Users\\Eric\\projects\\scenes\\sponza\\NewSponza_Main_glTF_NoDecals_Mirror_002.gltf");

	//ModelData scene = loader.loadModel("C:\\Users\\Eric\\projects\\scenes\\TestScene\\TestScene.gltf");

	/*ModelData scene = loader.loadModel("C:\\Users\\Eric\\projects\\scenes\\CornellBox\\CornellBox.gltf");*/

	ModelData pointSource = loader.loadModel("C:\\Users\\Eric\\projects\\scenes\\LightSources\\\LightSource.gltf");

	int i = 0;

	/*std::vector<Mesh> testScene;

	std::vector<MeshInstance> instances;
	for(auto& m : scene.meshes)
	{
		testScene.push_back(raytracer.loadMesh(m.vertices, m.indices, m.name));
		
		if (!m.material.albedo.empty())
		{
			TextureData* data = loader.getTextureData(m.material.albedo);
			testScene.back().material.albedoIndex = raytracer.loadTexture(data->pixels, data->width, data->height, data->nrChannels, m.material.albedo);
		}
		if (!m.material.normal.empty())
		{
			TextureData* data = loader.getTextureData(m.material.normal);
			testScene.back().material.normalIndex = raytracer.loadTexture(data->pixels, data->width, data->height, data->nrChannels, m.material.normal);
		}
		if (!m.material.metallic.empty())
		{
			TextureData* data = loader.getTextureData(m.material.metallic);
			testScene.back().material.metallicIndex = raytracer.loadTexture(data->pixels, data->width, data->height, data->nrChannels, m.material.metallic);
		}
		if (!m.material.roughness.empty())
		{
			TextureData* data = loader.getTextureData(m.material.roughness);
			testScene.back().material.roughnessIndex = raytracer.loadTexture(data->pixels, data->width, data->height, data->nrChannels, m.material.roughness);
		}

		instances.push_back(raytracer.getMeshInstance(m.name));
		instances.back().material = testScene.back().material;
		instances.back().transform = m.transform;
	}*/

	MeshInstance pointLightInstance;
	raytracer.loadMesh(pointSource.meshes[0].vertices, pointSource.meshes[0].indices, pointSource.meshes[0].name);
	pointLightInstance = raytracer.getMeshInstance(pointSource.meshes[0].name);
	TextureData* data = loader.getTextureData(pointSource.meshes[0].material.albedo);
	pointLightInstance.material.albedoIndex = 0;
	data = loader.getTextureData(pointSource.meshes[0].material.normal);
	pointLightInstance.material.normalIndex = 0;
	data = loader.getTextureData(pointSource.meshes[0].material.roughness);
	pointLightInstance.material.roughnessIndex = 0;
	data = loader.getTextureData(pointSource.meshes[0].material.metallic);
	pointLightInstance.material.metallicIndex = 0;
	data = loader.getTextureData(pointSource.meshes[0].material.emission);
	pointLightInstance.material.emissionIndex = raytracer.loadTexture(data->pixels, data->width, data->height, data->nrChannels, pointSource.meshes[0].material.emission);

	loader.freeAssets();

	SunLight sun = {};
	sun.color = glm::vec3(1.0f , 1.0f, 5.0f);
	sun.direction = glm::vec3(1.0f, -1.0f, 0.0f);
	sun.strength = 50.0f;
	sun.radius = 0.2;

	SpotLight spot = {};

	glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f);

	PointLightSourceInstance p;
	p.position = glm::vec3(0.0f, 4.0f, 0.0f);
	p.radius = 0.5f;
	p.strength = 50.0f;
	p.lightModel = pointLightInstance;

	CameraController camera(2.0f, 20.0f, 90.0f, 1920.0f/1080.0f, 0.1f, 100.0f, up);
	
	bool quit = false;
	SDL_Event e;

	uint64_t now = SDL_GetPerformanceCounter();
	uint64_t last;
	float deltaTime = 0.0f;
	
	int mouseX, mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);
	float deltaYaw = 0.0f;
	float deltaPitch = 0.0f;

	float deltaFront = 0.0f;
	float deltaRight = 0.0f;

	camera.update(deltaTime, deltaYaw, deltaPitch, deltaRight, deltaFront);

	bool cameraActive = true;
	bool guiActive = true;

	

	while (!quit)
	{
		last = now;
		now = SDL_GetPerformanceCounter();
		deltaTime = ((float)(now - last) * 1000.0f) / (float)SDL_GetPerformanceFrequency();
		deltaTime = deltaTime * 0.001f;

		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			raytracer.handleGuiEvents(&e);

			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			if (e.type == SDL_MOUSEMOTION)
			{
				int x, y;
				SDL_GetMouseState(&x, &y);
				deltaYaw = static_cast<float>(mouseX - x);
				deltaPitch = static_cast<float>(mouseY - y);
				mouseX = x;
				mouseY = y;
			}
			if(e.type == SDL_KEYDOWN)
			{
				switch (e.key.keysym.sym) 
				{
				case SDLK_w:
					deltaFront = 1.0f;
					break;
				case SDLK_s:
					deltaFront = -1.0f;
					break;
				case SDLK_d:
					deltaRight = 1.0f;
					break;
				case SDLK_a:
					deltaRight = -1.0f;
					break;
				case SDLK_LCTRL:
					cameraActive = !cameraActive;
					break;
				case SDLK_LALT:
					{
					guiActive = !guiActive;
					raytracer.hideGui();
					}
					break;
				}
			}
			if (e.type == SDL_KEYUP)
			{
				switch (e.key.keysym.sym)
				{
				case SDLK_w:
					deltaFront = 0.0f;
					break;
				case SDLK_s:
					deltaFront = 0.0f;
					break;
				case SDLK_d:
					deltaRight = 0.0f;
					break;
				case SDLK_a:
					deltaRight = 0.0f;
					break;
				}
			}
			if (e.type == SDL_WINDOWEVENT)
			{
				if(e.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					
				}
			}
		}

		if (!cameraActive)
		{
			camera.update(deltaTime, deltaYaw, deltaPitch, deltaRight, deltaFront);
			raytracer.setCamera(camera._viewMatrix, camera._projectionMatrix, camera._position);
		}
		
		/*uint32_t id = 0;
		for(unsigned int i = 0; i<testScene.size(); i++)
		{
			instances[i].previousTransform = instances[i].transform;
			raytracer.drawMeshInstance(instances[i], 0x01);
			id++;
		}*/
		
		raytracer.drawPointLight(p);
		
		if(guiActive)
		{ 
			
			raytracer.drawDebugGui(window);
		}

		
		raytracer.update();
	}

	raytracer.destroy();

	return 0;
}
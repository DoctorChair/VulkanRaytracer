#include "VulkanContext.h"
#include "ModelLoader/ModelLoader.h"
#include "Raytracer/Raytracer.h"
#include "glm/gtc/matrix_transform.hpp"
#include "ModelLoader/TextureLoader.h"

#include "CameraController.h"

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1920, 1080,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

	Raytracer raytracer;
	raytracer.init(window, 1920, 1080);

	TextureLoader textureLoader;
	TextureData noiseTexture = textureLoader.loadTexture("C:\\Users\\Eric\\projects\\textures\\FreeBlueNoiseTextures\\Data\\1024_1024\\LDR_RGBA_0.png");
	TextureData environmentTexture = textureLoader.loadTexture("C:\\Users\\Eric\\projects\\textures\\evening_meadow_4k.hdr");

	uint32_t noiseTexureIndex = raytracer.loadTexture(noiseTexture.pixels, noiseTexture.width, noiseTexture.height, noiseTexture.nrChannels, "blueNoiseSampleTexture");

	uint32_t environmentTextureIndex = raytracer.loadTexture(environmentTexture.pixels, environmentTexture.width, environmentTexture.height, environmentTexture.nrChannels, "environmentMapTexture");

	raytracer.setNoiseTextureIndex(noiseTexureIndex);
	raytracer.setEnvironmentTextureIndex(environmentTextureIndex);

	ModelLoader loader;
	
	ModelData scene = loader.loadModel("C:\\Users\\Eric\\projects\\scenes\\sponza\\NewSponza_Main_glTF_NoDecals_Mirror_002.gltf");

	//ModelData scene = loader.loadModel("C:\\Users\\Eric\\projects\\scenes\\TestScene\\TestScene.gltf");

	//ModelData scene = loader.loadModel("C:\\Users\\Eric\\projects\\scenes\\CornellBox\\CornellBox.gltf");

	ModelData pointSource = loader.loadModel("C:\\Users\\Eric\\projects\\scenes\\LightSources\\\LightSource.gltf");

	int i = 0;

	std::vector<Mesh> testScene;

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
	}

	MeshInstance pointLightInstance;
	raytracer.loadMesh(pointSource.meshes[0].vertices, pointSource.meshes[0].indices, pointSource.meshes[0].name);
	pointLightInstance = raytracer.getMeshInstance(pointSource.meshes[0].name);
	TextureData* data = loader.getTextureData(pointSource.meshes[0].material.albedo);
	pointLightInstance.material.albedoIndex = 0;
	data = loader.getTextureData(pointSource.meshes[0].material.normal);
	pointLightInstance.material.normalIndex = raytracer.loadTexture(data->pixels, data->width, data->height, data->nrChannels, pointSource.meshes[0].material.normal);
	data = loader.getTextureData(pointSource.meshes[0].material.roughness);
	pointLightInstance.material.roughnessIndex = raytracer.loadTexture(data->pixels, data->width, data->height, data->nrChannels, pointSource.meshes[0].material.roughness);
	data = loader.getTextureData(pointSource.meshes[0].material.metallic);
	pointLightInstance.material.metallicIndex = raytracer.loadTexture(data->pixels, data->width, data->height, data->nrChannels, pointSource.meshes[0].material.metallic);
	data = loader.getTextureData(pointSource.meshes[0].material.emission);
	pointLightInstance.material.emissionIndex = raytracer.loadTexture(data->pixels, data->width, data->height, data->nrChannels, pointSource.meshes[0].material.emission);

	loader.freeAssets();

	PointLight point = {};
	point.radius = 0.05f;
	point.position = glm::vec3(0.0f, 10.0f, 0.0f);
	point.color = glm::vec3(1.0f, 1.0f, 0.8f);
	point.strength = 5000.0f;
	point.emissionIndex = 2;

	PointLight point2 = {};
	point2.radius = 0.2f;
	point2.position = glm::vec3(0.0f, 8.0f, 0.0f);
	point2.color = glm::vec3(1.0f, 1.0f, 0.8f);
	point2.strength = 50.0f;

	PointLight point3 = {};
	point3.radius = 0.2f;
	point3.position = glm::vec3(0.0f, 6.0f, 0.0f);
	point3.color = glm::vec3(1.0f, 1.0f, 0.8f);
	point3.strength = 5000.0f;

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

	while (!quit)
	{
		last = now;
		now = SDL_GetPerformanceCounter();
		deltaTime = ((float)(now - last) * 1000.0f) / (float)SDL_GetPerformanceFrequency();
		deltaTime = deltaTime * 0.001f;

		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
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

		camera.update(deltaTime, deltaYaw, deltaPitch, deltaRight, deltaFront);
		raytracer.setCamera(camera._viewMatrix, camera._projectionMatrix, camera._position);

		uint32_t id = 0;
		for(unsigned int i = 0; i<testScene.size(); i++)
		{
			instances[i].previousTransform = instances[i].transform;
			raytracer.drawMeshInstance(instances[i], 0x01);
			id++;
		}
		
		raytracer.drawPointLight(p);
		/*raytracer.drawPointLight(point2);
		raytracer.drawPointLight(point3);*/
		//raytracer.drawSpotLight(spot);
		//raytracer.drawSunLight(sun);
		raytracer.update();
	}

	raytracer.destroy();

	return 0;
}
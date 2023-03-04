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
	
	uint32_t noiseTexureIndex = raytracer.loadTexture(noiseTexture.pixels, noiseTexture.width, noiseTexture.height, noiseTexture.nrChannels, "blueNoiseSampleTexture");

	raytracer.setNoiseTextureIndex(noiseTexureIndex);

	ModelLoader loader;
	
	//ModelData mod = loader.loadModel("C:\\Users\\Eric\\Blender\\Starfighter\\Starfighter.obj");

	//ModelData sponza = loader.loadModel("C:\\Users\\Eric\\projects\\scenes\\sponza_original\\sponza.obj");

	//ModelData sponza = loader.loadModel("C:\\Users\\Eric\\projects\\scenes\\survival_guitar_backpack\\scene.gltf");

	//ModelData sunTemple = loader.loadModel("C:\\Users\\Eric\\projects\\scenes\\sun_temple\\SunTemple.fbx");

	ModelData sponza = loader.loadModel("C:\\Users\\Eric\\projects\\scenes\\sponza\\NewSponza_Main_glTF_NoDecals_002.gltf");

	//ModelData sponza = loader.loadModel("C:\\Users\\Eric\\projects\\scenes\\TestScene\\TestScene.gltf");

	int i = 0;

	std::vector<Mesh> testScene;

	std::vector<MeshInstance> instances;
	for(auto& m : sponza.meshes)
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

	loader.freeAssets();

	PointLight point = {};
	point.diameter = 0.2f;
	point.position = glm::vec3(0.0f, 8.0f, 0.0f);
	point.color = glm::vec3(1.0f, 1.0f, 0.9f);
	point.strength = 50.0f;

	PointLight point2 = {};
	point2.diameter = 0.1f;
	point2.position = glm::vec3(0.0f, 2.0f, 0.0f);
	point2.color = glm::vec3(1.0f, 1.0f, 0.0f);

	SunLight sun = {};
	sun.color = glm::vec3(1.0f , 1.0f, 1.0f);
	sun.direction = glm::vec3(1.0f, -1.0f, 0.0f);
	sun.strength = 0.0;

	SpotLight spot = {};

	glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f);

	CameraController camera(100.0f, 20.0f, 90.0f, 1920.0f/1080.0f, 0.1f, 100.0f, up);
	
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
			raytracer.drawMeshInstance(instances[i], instances[i].transform);
			id++;
		}
		
		raytracer.drawPointLight(point);
		raytracer.drawSpotLight(spot);
		raytracer.drawSunLight(sun);
		raytracer.update();
	}

	raytracer.destroy();

	return 0;
}
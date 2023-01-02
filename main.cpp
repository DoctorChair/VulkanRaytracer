#include "VulkanContext.h"
#include "RenderpassBuilder.h"
#include "Texture.h"
#include "Buffer.h"
#include "DescriptorSet.h"
#include "Shader.h"
#include "CommandBuffer.h"
#include "CommandBufferAllocator.h"

#include "VulkanErrorHandling.h"

#include "ModelLoader/ModelLoader.h"
#include "Raytracer/Raytracer.h"

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1024, 1024,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

	ModelLoader loader;
	ModelData mod = loader.loadModel("C:\\Users\\Eric\\Blender\\Starfighter\\Starfighter.obj");

	Raytracer raytracer;
	raytracer.init(window);

	Mesh mesh = raytracer.loadMesh(mod.meshes[0].vertices, mod.meshes[0].indices);
	Mesh mesh2 = raytracer.loadMesh(mod.meshes[3].vertices, mod.meshes[3].indices);

	std::vector<unsigned char> pixels(1024 * 1024, 255);

	uint32_t index = raytracer.loadTexture(pixels, 1024, 1024, 1);

	bool quit = false;
	SDL_Event e;
	while (!quit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
		}
		glm::mat4 matrix = glm::mat4(10.0f);
		raytracer.drawMesh(mesh, glm::mat4(1.0f), 0);
		raytracer.drawMesh(mesh2, glm::mat4(1.0f), 0);
		raytracer.update();
	}

	raytracer.destroy();

	return 0;
}
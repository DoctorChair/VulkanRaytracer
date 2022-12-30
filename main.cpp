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

	Raytracer raytracer;
	raytracer.init(window);

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

		raytracer.update();
	}

	raytracer.destroy();

	return 0;
}
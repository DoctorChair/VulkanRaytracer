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
	Raytracer raytracer;
	raytracer.init(500, 500);

	return 0;
}
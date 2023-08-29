# Hybrid Vulkan Raytracer
This program was created for my beachlor thesis.

## Building
vcpkg is needed to build it in addition to a C++2017 capable compiler and Cmake.
Employed Libraries are:
* Vulkan
* SDL2
* assimp
* stb
* ImGui
* ImPlot

## Features
The program employs hybrid rendering, multiple importance sampling, backprojection and the cook-torrance BRDF to create rendered images.
Rendered images contain raytraced cast shadows, diffuse reflections and specular reflections.
Via backprojection multiple past frames are combined to a single frame to enable real-time-framerates.

## Images
![AfterColorMapping(1)](https://github.com/DoctorChair/VulkanRaytracer/assets/25391031/cdbb1168-2b0a-4a7a-a63c-cfd2d3f314d0)

Here the reflectivity was boosted to showcase mirroring effects.
![FresnelEffect(1)](https://github.com/DoctorChair/VulkanRaytracer/assets/25391031/17600a69-0a3c-4869-a04f-c18c4fa3c5c1)

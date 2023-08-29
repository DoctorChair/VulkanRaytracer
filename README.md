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
![AfterColorMapping(1)](https://github.com/DoctorChair/VulkanRaytracer/assets/25391031/6112af3c-a2c9-4d2c-b303-31e62ade3792)

Here the reflectivity was boosted to showcase mirroring effects.
![FresnelEffect(1)](https://github.com/DoctorChair/VulkanRaytracer/assets/25391031/6d92eda8-a32d-42f3-8f44-f1666f5375c1)


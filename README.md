# Hybrid Vulkan Raytracer
This program was created for my bachelor thesis.

## Building
vcpkg is needed to build the programm in addition to a C++2017 capable compiler and a raytracing capable GPU.
Used Libraries are:
* Vulkan
* SDL2
* assimp
* stb
* ImGui
* ImPlot

## Features
The program uses hybrid rendering, multiple importance sampling, backprojection and the cook-torrance BRDF to create rendered images.
Rendered images contain raytraced cast shadows, diffuse reflections and specular reflections.
Via backprojection multiple past frames are combined to a single frame to enable real-time framerates.

## Images
![AfterColorMapping(1)](https://github.com/DoctorChair/VulkanRaytracer/assets/25391031/6112af3c-a2c9-4d2c-b303-31e62ade3792)

![FresnelEffect(1)](https://github.com/DoctorChair/VulkanRaytracer/assets/25391031/6d92eda8-a32d-42f3-8f44-f1666f5375c1)


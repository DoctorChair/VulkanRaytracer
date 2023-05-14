# Hybrid Vulkan Raytracer
This program was created for my beachlor thesis.

## Building
vcpkg is needed to build it in addition to a C++2017 capable compiler and Cmake.
The topmost CMakeLists.txt builds the program.
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

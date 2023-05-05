cmake_minimum_required(VERSION 3.7.2)

# set the project name
project(Raytracer)

# set c++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#add library
add_library(${PROJECT_NAME}
Raytracer.h
Raytracer.cpp
TLAS.h
TLAS.cpp
BLAS.h
BLAS.cpp
RaytracingShader.h
RaytracingShader.cpp
ShaderBindingTable.h
ShaderBindingTable.cpp
"${CMAKE_SOURCE_DIR}/external/imGui/imgui.h"
"${CMAKE_SOURCE_DIR}/external/imGui/imgui.cpp"
"${CMAKE_SOURCE_DIR}/external/imGui/imgui_demo.cpp"
"${CMAKE_SOURCE_DIR}/external/imGui/imgui_draw.cpp"
"${CMAKE_SOURCE_DIR}/external/imGui/imgui_tables.cpp"
"${CMAKE_SOURCE_DIR}/external/imGui/imgui_widgets.cpp"
"${CMAKE_SOURCE_DIR}/external/imGui/backends/imgui_impl_vulkan.cpp"
"${CMAKE_SOURCE_DIR}/external/imGui/backends/imgui_impl_sdl2.cpp"
"${CMAKE_SOURCE_DIR}/external/imGui/implot.cpp"
"${CMAKE_SOURCE_DIR}/external/imGui/implot.h"
"${CMAKE_SOURCE_DIR}/external/imGui/implot_items.cpp"
"${CMAKE_SOURCE_DIR}/external/imGui/implot_demo.cpp"
)

#add includes
target_include_directories(${PROJECT_NAME}  PUBLIC
    "${PROJECT_BINARY_DIR}"
    "C:/Users/Eric/projects/VGM/VGM"
    "${CMAKE_SOURCE_DIR}/shared"
    "${CMAKE_SOURCE_DIR}/external/imgui"
    "${CMAKE_SOURCE_DIR}/external/imgui/backends"
)

#add imGui
target_link_libraries(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR/build/external/Debug/imgui.lib}")

#add VGM subproject
target_link_libraries(${PROJECT_NAME} PUBLIC C:/Users/Eric/projects/VGM/build/VGM/Debug/VGM.lib)

#add SDL2
find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL2::SDL2 SDL2::SDL2main)

#add Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)
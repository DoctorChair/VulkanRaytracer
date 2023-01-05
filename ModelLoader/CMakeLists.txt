cmake_minimum_required(VERSION 3.7.2)

# set the project name
project(ModelLoader)

# set c++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#add library
add_library(${PROJECT_NAME}
ModelLoader.h
ModelLoader.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
"C:/VulkanSDK/1.3.236.0/Include"
"stb_image.h"
"C:/Users/Eric/projects/VulkanRaytracing/shared"
)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC assimp::assimp)
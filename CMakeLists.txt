cmake_minimum_required(VERSION 3.7.2)

# set the project name
project(VulkanRayTracing)

# set c++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(shaders)
add_subdirectory(ModelLoader)

#add executable
add_executable(${PROJECT_NAME} 
    ${CMAKE_SOURCE_DIR}/main.cpp
    )

#add includes
target_include_directories(${PROJECT_NAME}  PUBLIC
    "${PROJECT_BINARY_DIR}"
    "C:/Users/Eric/projects/VGM/VGM" 
)

#add VGM subproject
target_link_libraries(${PROJECT_NAME} PUBLIC C:/Users/Eric/projects/VGM/build/VGM/Debug/VGM.lib)

#add ModelLoading subproject
target_link_libraries(${PROJECT_NAME} PUBLIC ModelLoader)

#add SDL2
find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL2::SDL2 SDL2::SDL2main)

#add Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)


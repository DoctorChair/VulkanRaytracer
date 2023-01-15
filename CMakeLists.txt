cmake_minimum_required(VERSION 3.7.2)

# set the project name
project(VulkanRayTracing)

# set c++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(shaders)
add_subdirectory(ModelLoader)
add_subdirectory(Raytracer)

#add executable
add_executable(${PROJECT_NAME} 
    ${CMAKE_SOURCE_DIR}/main.cpp
    ${CMAKE_SOURCE_DIR}/CameraController.h
    ${CMAKE_SOURCE_DIR}/CameraController.cpp
    )

#add includes
target_include_directories(${PROJECT_NAME}  PUBLIC
   "${PROJECT_BINARY_DIR}"
)

#add ModelLoading subproject
target_link_libraries(${PROJECT_NAME} PUBLIC ModelLoader)

#add Raytracer subproject
target_link_libraries(${PROJECT_NAME} PUBLIC Raytracer)


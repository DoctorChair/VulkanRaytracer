#pragma once
#include "glm/glm.hpp"

struct Vertex 
{
	glm::vec3 position;
	glm::vec2 texCoord[4];
	glm::vec3 normal;
	glm::vec3 tangent;
	float padding;
};
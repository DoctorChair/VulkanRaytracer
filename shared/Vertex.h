#pragma once
#include "glm/glm.hpp"

struct Vertex 
{
	glm::vec4 position;
	glm::vec4 normal;
	glm::vec4 tangent;
	glm::vec4 texCoordPair;
};
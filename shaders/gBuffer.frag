#version 460

//shader input
layout (location = 0) in vec3 inPos;

//output write
layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outID;
layout (location = 3) out vec4 outAlbedo;

layout(set = 1, binding = 0) uniform sampler2D albedoTextureArray;
layout(set = 1, binding = 1) uniform sampler2D metallicTextureArray;
layout(set = 1, binding = 2) uniform sampler2D normalTextureArray;
layout(set = 1, binding = 3) uniform sampler2D roughnessTextureArray;


void main()
{
	//return color
	outPosition = vec4(inPos,1.0f);
}


#version 460

//shader input
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 texCoords;

//output write
layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outID;
layout (location = 3) out vec4 outAlbedo;

layout(set = 1, binding = 0) uniform sampler albedoSampler;
layout(set = 1, binding = 1) uniform sampler metallicSampler;
layout(set = 1, binding = 2) uniform sampler normalTSampler;
layout(set = 1, binding = 3) uniform sampler roughnessSampler;

layout(set = 3, binding = 0) uniform texture2D textures[1024]; 

void main()
{
	//return color
	vec4 color = texture(sampler2D(textures[0], albedoSampler), texCoords);
	outPosition = vec4(color);
}


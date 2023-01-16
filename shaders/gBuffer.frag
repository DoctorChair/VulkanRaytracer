#version 460

//shader input
layout (location = 0) in vec3 outPos;
layout (location = 1) in vec2 texCoords;
layout (location = 2) in flat uint albedoIndex;
layout (location = 3) in flat uint normalIndex;
layout (location = 4) in flat uint metallicIndex;
layout (location = 5) in flat uint roughnessIndex;
//output write
layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outID;

layout(set = 1, binding = 0) uniform sampler albedoSampler;
layout(set = 1, binding = 1) uniform sampler metallicSampler;
layout(set = 1, binding = 2) uniform sampler normalSampler;
layout(set = 1, binding = 3) uniform sampler roughnessSampler;

layout(set = 3, binding = 0) uniform texture2D textures[1024]; 

void main()
{
	//return color
	vec4 color = texture(sampler2D(textures[albedoIndex], albedoSampler), texCoords);
	vec4 normal = texture(sampler2D(textures[normalIndex], normalSampler), texCoords);
	vec4 metallic = texture(sampler2D(textures[metallicIndex], metallicSampler), texCoords);
	vec4 rougness = texture(sampler2D(textures[roughnessIndex], roughnessSampler), texCoords);
	

	outColor = color;
	outNormal = normal;
	outID = vec4(0.0, 0.0, 0.0, 1.0);
}


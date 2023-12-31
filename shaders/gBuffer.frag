#version 460
#extension GL_GOOGLE_include_directive : enable

#include "BRDF.glsl"
#include "common.glsl"
//shader input
layout (location = 0) in vec4 position;
layout (location = 1) in vec2 TexCoords0;
layout (location = 2) in vec2 TexCoords1;
layout (location = 3) in vec2 TexCoords2;
layout (location = 4) in vec2 TexCoords3;
layout (location = 5) in flat uint instanceIndex;
layout (location = 6) in vec3 T;
layout (location = 7) in vec3 B;
layout (location = 8) in vec3 N;
layout (location = 9) in flat uint ID;
layout (location = 10) in vec4 previousProjectionSpacePosition;
layout (location = 11) in vec4 currentProjectionSpacePosition;
layout (location = 12) in flat vec3 meshNormal;
//output write
layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outRoughnessMetallness;
layout (location = 3) out vec4 outPosition;
layout (location = 4) out vec4 outEmission;
layout (location = 5) out vec4 outVelocity;


layout(set = 0, binding = 0) uniform  CameraBuffer{
	mat4 viewMatrix;
	mat4 inverseviewMatrix;
    mat4 projectionMatrix;
    mat4 inverseProjectionMatrix;
	vec3 cameraPosition;
	mat4 previousProjectionMatrix;
	mat4 previousViewMatrix;
} cameraData;

layout(set = 1, binding = 0) uniform sampler linearSampler;
layout(set = 1, binding = 1) uniform sampler nearestSampler;

layout(std430, set = 2, binding = 0) readonly buffer DrawInstanceBuffer{
	drawInstanceData instanceData[];
} drawData;

layout(set = 3, binding = 0) uniform texture2D textures[1024]; 

float encodeRGBtoFloat( vec3 rgb ) 
{
	rgb = normalize(rgb);
	
	int x = int(floor(rgb.x * 255.0));
	int y = int(floor(rgb.y * 255.0)) * int(256);
	int z = int(floor(rgb.z * 255.0)) * int(65536);

	int packed = x + y + z;

  	return float(packed);
}


void main()
{
	uint albedoIndex = drawData.instanceData[instanceIndex].material.albedoIndex;
	uint normalIndex = drawData.instanceData[instanceIndex].material.normalIndex;
	uint metallicIndex = drawData.instanceData[instanceIndex].material.metallicIndex;
	uint roughnessIndex = drawData.instanceData[instanceIndex].material.roughnessIndex;
	uint emissionIndex = drawData.instanceData[instanceIndex].material.emissionIndex;

	//return color
	vec4 color = texture(sampler2D(textures[albedoIndex], linearSampler), TexCoords0);
	vec4 normal = texture(sampler2D(textures[normalIndex], linearSampler), TexCoords0);
	vec4 metallic = texture(sampler2D(textures[metallicIndex], linearSampler), TexCoords0);
	vec4 roughness = texture(sampler2D(textures[roughnessIndex], linearSampler), TexCoords0);
	vec4 emission = texture(sampler2D(textures[emissionIndex], linearSampler), TexCoords0);

	mat3 tbnMatrix = mat3(T, -B, N);
	
	normal.xyz = normalize(normal.xyz * 2.0 - 1.0); 
	normal.xyz = tbnMatrix * normal.xyz;
	
	if(normalIndex != 0)
	{
		outNormal =  vec4(normalize(normal.xyz), normal.w);
	}
	else
	{
		outNormal = vec4(N, 1.0);
	}

	outRoughnessMetallness.x = roughness.y;// * float(roughnessIndex != 0) + 1.0f * float(roughnessIndex == 0);
	outRoughnessMetallness.y = metallic.z;// * float(metallicIndex != 0) + 0.0f * float(metallicIndex == 0);
	outRoughnessMetallness.w = metallic.w;

	outColor = color;

	outPosition = position;

	outEmission = (vec4(0, 0, 0, color.w)) + (emission * drawData.instanceData[instanceIndex].emissionIntensity);
	
	vec3 previous = (vec3(previousProjectionSpacePosition.xyz / previousProjectionSpacePosition.w)) * 0.5 + 0.5;
	vec3 current = (vec3(currentProjectionSpacePosition.xyz / currentProjectionSpacePosition.w)) * 0.5 + 0.5;
	outVelocity = vec4(current - previous, 1.0f);
	outVelocity.z = float(ID);
	outVelocity.w = currentProjectionSpacePosition.w;

	outVelocity = float(length(outVelocity) >= 0.001) * outVelocity;
}


#version 460
#extension GL_GOOGLE_include_directive : enable

#include "BRDF.glsl"
#include "common.glsl"

//shader input
layout (location = 0) in vec2 texCoord;

//output write
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform  CameraBuffer{
	mat4 viewMatrix;
	mat4 inverseViewMatrix;
  	mat4 projectionMatrix;
  	mat4 inverseProjectionMatrix;
	vec3 cameraPosition;
} cameraData;

layout(set = 0, binding = 1) uniform  RenderBuffer{
	float fog;
	uint sunLightCount;
	uint pointLightCount;
	uint spotLightCount;
  	uint maxRecoursionDepth;
  	uint maxDiffuseSampleCount;
  	uint maxSpecularSampleCount;
 	uint maxShadowRaySampleCount;
  	uint noiseSampleTextureIndex;
} globalDrawData;


layout(set = 1, binding = 0) uniform sampler2D inColor;
layout(set = 1, binding = 1) uniform sampler2D inNormal;
layout(set = 1, binding = 2) uniform sampler2D inDepth;
layout(set = 1, binding = 3) uniform sampler2D inRoughnessMetalness;
layout(set = 1, binding = 4) uniform sampler2D inID;
layout(set = 1, binding = 5) uniform sampler2D inPosition;

layout(std430, set = 2, binding = 0) readonly buffer SunBuffer{

	SunLight sunLights[];
} sunLightBuffer;

layout(std430, set = 2, binding = 1) readonly buffer PointBuffer{

	PointLight pointLights[];
} pointLightBuffer;

layout(std430, set = 2, binding = 2) readonly buffer SpotBuffer{

	SpotLight spotLights[];
} spotLightBuffer;


void main()
{
	float depth = texture(inDepth, texCoord).x;
    vec4 clipSpacePosition = vec4(texCoord * 2.0 - 1.0, depth, 1.0);
    vec4 viewSpacePosition = cameraData.inverseProjectionMatrix * clipSpacePosition;
    viewSpacePosition = viewSpacePosition / viewSpacePosition.w;
	vec4 position = cameraData.inverseViewMatrix * viewSpacePosition;
	

	vec3 normal = normalize(texture(inNormal, texCoord).xyz);
	vec3 color = texture(inColor, texCoord).xyz;
	vec4 roughnessMetalness = normalize(texture(inRoughnessMetalness, texCoord));
	
	float reflectance = 0.04;	
	float roughness = roughnessMetalness.x;
	float metallic = roughnessMetalness.y;

	vec3 viewDirection = normalize(cameraData.cameraPosition - position.xyz);

	vec3 radiance = vec3(0.0);

	/* for(int i = 0; i < globalDrawData.sunLightCount; i++)
	{
	vec3 lightColor = normalize(sunLightBuffer.sunLights[i].color.xyz);
	vec3 lightDirection = -normalize(sunLightBuffer.sunLights[i].direction);

	vec3 brdf = cookTorrancePhongBRDF(viewDirection, lightDirection, normal, color, metallic, roughness, reflectance);

	float irradiance = max(dot(normal, lightDirection), 0.0);
	
	radiance =  radiance + irradiance * brdf * 2.0 * lightColor;	
	}

	for(int i = 0; i < globalDrawData.pointLightCount; i++)
	{
	vec3 lightColor = normalize(pointLightBuffer.pointLights[i].color.xyz);

	vec3 lightDirection = pointLightBuffer.pointLights[i].position - position.xyz;
	
	float distance = length(lightDirection);
	float attenuation = 1.0/(distance*distance);

	lightDirection = normalize(lightDirection);

	vec3 brdf = cookTorrancePhongBRDF(viewDirection, lightDirection, normal, color, metallic, roughness, reflectance);

	float irradiance = max(dot(normal, lightDirection), 0.0);

	radiance =  radiance + irradiance * brdf * lightColor * 50.0 * attenuation;
	} */

	outColor = vec4(radiance, 1.0f);
}


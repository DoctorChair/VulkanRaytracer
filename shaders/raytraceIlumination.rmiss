#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "BRDF.glsl"
#include "common.glsl"


layout(set = 0, binding = 1) uniform  RenderBuffer{
	uint sunLightCount;
	uint pointLightCount;
	uint maxRecoursionDepth;
	uint maxDiffuseSampleCount;
	uint maxSpecularSampleCount;
	uint maxShadowRaySampleCount;
	uint noiseSampleTextureIndex;
	uint frameNumber;
	uint historyLength;
	uint historyIndex;
	uint nativeResolutionWidth;
	uint nativeResolutionHeight;
  	uint environmentTextureIndex;
	float heuristicExponent;
} globalDrawData;


layout(set = 4, binding = 0) uniform sampler albedoSampler;

layout(set = 6, binding = 0) uniform texture2D textures[1024]; 


struct hitPayload
{
	  uint depth;
  vec3 radiance;
};

layout(location = 1) rayPayloadInEXT hitPayload incomigPayload;

void main()
{
  vec2 sphereUV = vec2(atan(gl_WorldRayDirectionEXT.x, gl_WorldRayDirectionEXT.z) / (2*M_PI) + 0.5, -gl_WorldRayDirectionEXT.y * 0.5 + 0.5); 

  vec4 envColor = texture(sampler2D(textures[globalDrawData.environmentTextureIndex], albedoSampler), sphereUV);

  incomigPayload.radiance = envColor.xyz;
}
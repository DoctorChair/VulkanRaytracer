#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

//shader input
layout (location = 0) in vec2 texCoord;
//output write
layout (location = 0) out vec4 outColor;

layout (location = 2) out vec4 outPriorPosition;

layout(set = 0, binding = 0, rgba32f) uniform image3D radiance;
layout(set = 0, binding = 1) uniform sampler imageSampler;
layout(set = 0, binding = 2) uniform texture2D veloctyHistory[4];

layout(set = 0, binding = 3) uniform  RenderBuffer{
	float fog;
	uint sunLightCount;
	uint pointLightCount;
	uint spotLightCount;
  	uint maxRecoursionDepth;
  	uint maxDiffuseSampleCount;
  	uint maxSpecularSampleCount;
  	uint maxShadowRaySampleCount;
  	uint noiseSampleTextureIndex;
	uint sampleSequenceLength;
	uint frameNumber;
	uint historyLength;
	uint historyIndex;
	uint nativeResolutionWidth;
	uint nativeResolutionHeight;
} globalDrawData;

layout( push_constant ) uniform constants
{
	ivec2 nativeResolution;
} PushConstants;


void main()
{
	float exposure = 0.5;
	float gamma = 2.2;
		
	vec2 uv = texCoord;
	uint index = globalDrawData.historyIndex % globalDrawData.historyLength;

	ivec2 pixCoord = ivec2((uv*PushConstants.nativeResolution));
    vec4 hdrColor = vec4(0.0);
	hdrColor = hdrColor + imageLoad(radiance, ivec3(pixCoord, index));
	index = (index - 1) % globalDrawData.historyLength;

	for(uint i = 1; i < globalDrawData.historyLength; i++)
	{ 
		vec2 velocty = texture(sampler2D(veloctyHistory[globalDrawData.historyIndex], imageSampler), uv).xy;
		uv = uv - velocty;
		pixCoord = ivec2((uv * PushConstants.nativeResolution));
		hdrColor = hdrColor + imageLoad(radiance, ivec3(pixCoord, index));
		
		index = ((index - 1) % globalDrawData.historyLength * uint(index != 0)) + ((globalDrawData.historyLength - 1)*uint(index==0));	
	}

	hdrColor = hdrColor / float(globalDrawData.historyLength);

    vec4 mapped = vec4(1.0) - exp(-hdrColor * exposure);
    mapped = pow(mapped, vec4(1.0 / gamma));

    hdrColor.w = 1.0;
	outColor = mapped;
}

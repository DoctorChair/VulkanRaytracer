#version 460
#extension GL_GOOGLE_include_directive : enable

#include "BRDF.glsl"
#include "common.glsl"

layout (location = 0) in vec4 vertexPos;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec4 tangent;
layout (location = 3) in vec4 TexCoordPair;

//output variable to the fragment shader
layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec2 outTexCoords0;
layout (location = 2) out vec2 outTexCoords1;
layout (location = 3) out vec2 outTexCoords2;
layout (location = 4) out vec2 outTexCoords3;
layout (location = 5) out flat uint instanceIndex;
layout (location = 6) out vec3 T;
layout (location = 7) out vec3 B;
layout (location = 8) out vec3 N;
layout (location = 9) out flat uint outID;
layout (location = 10) out vec4 outPreviousProjectionSpacePosition;
layout (location = 11) out vec4 outCurrentProjectionSpacePosition;


layout(set = 0, binding = 0) uniform  CameraBuffer{
	mat4 viewMatrix;
	mat4 inverseviewMatrix;
    mat4 projectionMatrix;
    mat4 inverseProjectionMatrix;
	vec3 cameraPosition;
	mat4 previousProjectionMatrix;
	mat4 previousViewMatrix;
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
	uint sampleSequenceLength;
	uint frameNumber;
	uint historyLength;
	uint historyIndex;
	uint nativeResolutionWidth;
	uint nativeResolutionHeight;
} globalDrawData;


layout(std430, set = 2, binding = 0) readonly buffer DrawInstanceBuffer{
	drawInstanceData instanceData[];
} drawData;

void main()
{
	mat4 model = drawData.instanceData[gl_InstanceIndex].modelMatrix;
	mat4 previousModel = drawData.instanceData[gl_InstanceIndex].previousModelMatrix;
	mat4 view = cameraData.viewMatrix;
	mat4 projection = cameraData.projectionMatrix;

	mat3 normalMatrix = transpose(inverse(mat3(model)));

	vec3 worldTangent =  normalize(normalMatrix * normalize(tangent.xyz));
	vec3 worldNormal = normalize(normalMatrix * normalize(normal.xyz));
	worldTangent = normalize(worldTangent - dot(worldTangent, worldNormal) * worldNormal);
	vec3 worldBitagnent =  normalize(cross(worldNormal, worldTangent));

	T = worldTangent;
	B = worldBitagnent;
	N = worldNormal;

	instanceIndex = gl_InstanceIndex;

	outTexCoords0 = TexCoordPair.xy;
	outTexCoords1 = TexCoordPair.zw;
	
	vec4 worldSpacePosition = model * vec4(vertexPos);
	vec4 projectionSpacePosition = projection * view * worldSpacePosition;
	vec4 previousProjectionSpacePosition = cameraData.previousProjectionMatrix * cameraData.previousViewMatrix * previousModel * vertexPos;

	outCurrentProjectionSpacePosition = projectionSpacePosition;
	outPreviousProjectionSpacePosition = previousProjectionSpacePosition;
	outPosition = worldSpacePosition;

	outID = drawData.instanceData[gl_InstanceIndex].ID;

	gl_Position = projectionSpacePosition;
}

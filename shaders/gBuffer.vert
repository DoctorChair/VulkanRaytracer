#version 460

layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec2 TexCoords0;
layout (location = 2) in vec2 TexCoords1;
layout (location = 3) in vec2 TexCoords2;
layout (location = 4) in vec2 TexCoords3;
layout (location = 5) in vec3 normal;
layout (location = 6) in vec3 tangent;
layout (location = 7) in vec3 bitangent;

//output variable to the fragment shader
layout (location = 0) out vec3 outPos;
layout (location = 1) out vec2 outTexCoords0;
layout (location = 2) out vec2 outTexCoords1;
layout (location = 3) out vec2 outTexCoords2;
layout (location = 4) out vec2 outTexCoords3;
layout (location = 5) out flat uint albedoIndex;
layout (location = 6) out flat uint normalIndex;
layout (location = 7) out flat uint metallicIndex;
layout (location = 8) out flat uint roughnessIndex;

layout (location = 9) out vec3 worldNormal;
layout (location = 10) out vec3 worldTangent;
layout (location = 11) out vec3 worldBitagnent;

layout(set = 0, binding = 0) uniform  CameraBuffer{
	mat4 viewMatrix;
	mat4 inverseviewMatrix;
    mat4 projectionMatrix;
    mat4 inverseProjectionMatrix;
	vec3 cameraPosition;
} cameraData;

layout(set = 0, binding = 1) uniform  RenderBuffer{
	float fog;
	uint sunLightCount;
	uint pointLightCount;
	uint spotLightCount;
} globalDrawData;


struct Material
{
	uint albedoIndex;
	uint metallicIndex;
	uint normalIndex;
	uint roughnessIndex;	
};

struct drawInstanceData
{
	mat4 modelMatrix;
	Material material;
	uint ID;
};

layout(std430, set = 2, binding = 0) readonly buffer DrawInstanceBuffer{
	drawInstanceData instanceData[];
} drawData;

void main()
{
	mat4 model = drawData.instanceData[gl_InstanceIndex].modelMatrix;
	mat4 view = cameraData.viewMatrix;
	mat4 projection = cameraData.projectionMatrix;

	albedoIndex = drawData.instanceData[gl_InstanceIndex].material.albedoIndex;
	normalIndex = drawData.instanceData[gl_InstanceIndex].material.normalIndex;
	metallicIndex = drawData.instanceData[gl_InstanceIndex].material.metallicIndex;
	roughnessIndex = drawData.instanceData[gl_InstanceIndex].material.roughnessIndex;

	mat4 viewModelMatrix = view  * model;

	worldTangent =  normalize(model * vec4(tangent, 0.0)).xyz;
	worldNormal = normalize(model * vec4(normal, 0.0)).xyz;
	worldTangent = normalize(worldTangent - dot(worldTangent, worldNormal) * worldNormal);
	worldBitagnent =  normalize(cross(worldNormal, worldTangent));

	outTexCoords0 = TexCoords0;
	outTexCoords1 = TexCoords1;
	outTexCoords2 = TexCoords2;
	outTexCoords3 = TexCoords3;
	
	gl_Position = projection * viewModelMatrix * vec4(vertexPos, 1.0f);
}

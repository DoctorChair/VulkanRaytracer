#version 460

//output variable to the fragment shader
layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

layout (location = 5) in uint instanceIndex;

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec2 texCoords;
layout (location = 2) out flat uint albedoIndex;
layout (location = 3) out flat uint normalIndex;
layout (location = 4) out flat uint metallicIndex;
layout (location = 5) out flat uint roughnessIndex;

layout(set = 0, binding = 0) uniform  CameraBuffer{
	mat4 viewMatrix;
	mat4 inverseviewMatrix;
    mat4 projectionMatrix;
    mat4 inverseProjectionMatrix;
	vec3 cameraPosition;
} cameraData;

layout(set = 0, binding = 1) uniform  RenderBuffer{
	float fog;
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
	mat4 model = mat4(1.0f);//drawData.instanceData[gl_InstanceIndex].modelMatrix;
	mat4 view = cameraData.viewMatrix;
	mat4 projection = cameraData.projectionMatrix;

	albedoIndex = drawData.instanceData[gl_InstanceIndex].material.albedoIndex;
	normalIndex = drawData.instanceData[gl_InstanceIndex].material.normalIndex;
	metallicIndex = drawData.instanceData[gl_InstanceIndex].material.metallicIndex;
	roughnessIndex = drawData.instanceData[gl_InstanceIndex].material.roughnessIndex;

	outPos = ( cameraData.projectionMatrix * cameraData.viewMatrix * vec4(vertexPos, 1.0f)).xyz;
	texCoords = texCoord;
	gl_Position = projection * view  * model * vec4(vertexPos, 1.0f);
}

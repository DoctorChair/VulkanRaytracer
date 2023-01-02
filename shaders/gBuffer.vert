#version 460

//output variable to the fragment shader
layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec2 texCoords;

layout(set = 0, binding = 0) uniform  CameraBuffer{
	mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 cameraMatrix;
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

layout(std140, set = 2, binding = 0) readonly buffer DrawInstanceBuffer{
	drawInstanceData instanceData[];
} drawData;

void main()
{
	mat4 model = drawData.instanceData[gl_InstanceIndex].modelMatrix;
	outPos = (model * vec4(vertexPos, 1.0f)).xyz;
	texCoords = texCoord;
	gl_Position = model * vec4(vertexPos, 1.0f);
}

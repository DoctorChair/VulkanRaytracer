#version 460

//output variable to the fragment shader
//layout (location = 0) in vec3 vertexPos;
//layout (location = 1) in vec2 texCoord;
//layout (location = 2) in vec3 normal;
//layout (location = 3) in vec3 tangent;
//layout (location = 4) in vec3 bitangent;

layout (location = 0) out vec3 outPos;

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
};

layout(std140, set = 2, binding = 0) readonly buffer DrawInstanceBuffer{
	drawInstanceData instanceData[];
} drawData;

void main()
{
	const vec3 positions[3] = vec3[3](
		vec3(1.f,1.f, 0.0f),
		vec3(-1.f,1.f, 0.0f),
		vec3(0.f,-1.f, 0.0f)
	);

	mat4 model = drawData.instanceData[0].modelMatrix;
	outPos = positions[gl_VertexIndex]; //vec3(model * vec4(vertexPos, 1.0f));

	gl_Position = vec4(positions[gl_VertexIndex], 1.0f); //cameraData.cameraMatrix * model * vec4(vertexPos, 1.0f);
}

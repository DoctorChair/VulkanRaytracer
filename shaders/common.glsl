struct SunLight
{
	vec3 direction;
	vec3 color;
	float strength;
};

struct PointLight
{
	vec3 position;
	float radius;
	vec3 color;
	float strength;
};

struct SpotLight
{
	vec3 position;
	vec3 direction;
	float openingAngle;
	vec4 color;
	float strength;
};

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
	uint vertexOffset;
	uint indicesOffset;
};

struct Vertex
{
	vec4 position;
	vec4 normal;
	vec4 tangent;
	vec4 texCoordPair;
};
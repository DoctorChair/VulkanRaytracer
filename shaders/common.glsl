struct SunLight
{
	vec3 direction;
	vec4 color;
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
};

struct Vertex
{
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 texCoord0;
	vec2 texCoord1;
	vec2 texCoord2;
	vec2 texCoord3;
};
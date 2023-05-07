struct SunLight
{
	vec3 direction;
	float radius;
	vec3 color;
	float strength;
};

struct PointLight
{
	vec3 position;
	float radius;
	vec3 color;
	float strength;
	uint emissionIndex;
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
	uint emissionIndex;	
};

struct drawInstanceData
{
	mat4 modelMatrix;
	mat4 previousModelMatrix;
	Material material;
	uint ID;
	uint vertexOffset;
	uint indicesOffset;
	float emissinIntensity;
	};

struct Vertex
{
	vec4 position;
	vec4 normal;
	vec4 tangent;
	vec4 texCoordPair;
};

vec3 calculateSphereHitPoint(vec3 lineOrigin, vec3 lineDirection, vec3 spherePosition, float radius)
{
	float hitVal = pow(dot(lineDirection, spherePosition - lineOrigin), 2.0) - (pow(length(spherePosition - lineOrigin), 2.0) - pow(radius, 2.0));

	if(hitVal >= 0)
	{
		float t1 = -(dot(lineDirection, (lineOrigin - spherePosition))) + hitVal;
		float t2 = -(dot(lineDirection, (lineOrigin - spherePosition))) - hitVal;

		vec3 a = lineDirection * t1;
		vec3 b = lineDirection * t2;

		return a * float(length(lineOrigin - a) <= length(lineOrigin - b)) + b * float(length(lineOrigin - b) <= length(lineOrigin - a));
	}

	return vec3(0.0, 0.0, 0.0);
}
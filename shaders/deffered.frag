#version 460

//shader input
layout (location = 0) in vec2 texCoord;

//output write
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform  CameraBuffer{
	mat4 viewMatrix;
	mat4 inverseViewMatrix;
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


layout(set = 1, binding = 0) uniform sampler2D inColor;
layout(set = 1, binding = 1) uniform sampler2D inNormal;
layout(set = 1, binding = 2) uniform sampler2D inDepth;
layout(set = 1, binding = 3) uniform sampler2D inRoughnessMetalness;
layout(set = 1, binding = 4) uniform sampler2D inID;

struct SunLight
{
	vec3 position;
	vec3 direction;
	vec4 color;
	float strength;
};

struct PointLight
{
	vec3 position;
	vec4 color;
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

layout(std140,set = 2, binding = 0) readonly buffer SunBuffer{

	SunLight sunLights[];
} sunLightBuffer;

layout(std140,set = 2, binding = 1) readonly buffer PointBuffer{

	PointLight pointLights[];
} pointLightBuffer;

layout(std140,set = 2, binding = 2) readonly buffer SpotBuffer{

	SpotLight spotLights[];
} spotLightBuffer;




#define M_PI 3.1415926535897932384626433832795

float normalDistriputionFunction(vec3 normal, vec3 halfway, float roughnessAlpha)
{
	return 0.0;
}

float schlickGGC(vec3 normal, vec3 v, float k)
{
	return (dot(normal, v) / (dot(normal, v) * (1-k) + k));
}

float geometricFunction(vec3 normal, vec3 view, vec3 light, float roughnessAlpha)
{
	return schlickGGC(normal, light, roughnessAlpha) * schlickGGC(normal, view, roughnessAlpha); 
}

vec3 fresnelSchlick(vec3 reflectivity, float cosTheata)
{
	return mix(reflectivity, vec3(1.0f), pow((1.0 - cosTheata), 5.0));
}

void main()
{
	float depth = texture(inDepth, texCoord).x;
	vec4 clipPos = vec4((texCoord * 2.0) - vec2(1.0), (depth * 2.0) - 1.0, 1.0);
    mat4 inverseViewProject = cameraData.inverseViewMatrix * cameraData.inverseProjectionMatrix;
    vec4 position = inverseViewProject * clipPos;

	position = position / position.w;

	vec3 normal = normalize(texture(inNormal, texCoord).xyz);
	vec3 color = texture(inColor, texCoord).xyz;
	vec4 roughnessMetalness = texture(inRoughnessMetalness, texCoord);

	float roughnessAlpha = roughnessMetalness.x;

	vec3 reflectivity = vec3(0.04);	
	reflectivity = mix(reflectivity, color, roughnessMetalness.y);

	for(uint i = 0; i<globalDrawData.sunLightCount; i++)
	{
		vec3 lightColor = sunLightBuffer.sunLights[i].color.xyz;
		vec3 incidentDirection = -normalize(sunLightBuffer.sunLights[i].direction);
		vec3 viewDirection = normalize(cameraData.cameraPosition - position.xyz);
		
		vec3 halfwaySum = (viewDirection + incidentDirection);
		vec3 halfwayVector = normalize(halfwaySum);
		
		float cosTheata = dot(incidentDirection, halfwayVector);

		vec3 lambert = color / M_PI;
		vec3 specular = (normalDistriputionFunction(normal, halfwayVector, roughnessAlpha) 
						* fresnelSchlick(reflectivity, cosTheata) 
						* geometricFunction(normal, viewDirection, incidentDirection, roughnessAlpha))
						/ (4 * dot(normal, incidentDirection) * dot(normal, viewDirection));
		float kS;
		float kD = 1.0-kS;
		
	}

	outColor = vec4(normal.xyz, 1.0f);
}


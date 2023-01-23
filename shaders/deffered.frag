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
layout(set = 1, binding = 5) uniform sampler2D inPosition;

struct SunLight
{
	vec3 direction;
	vec4 color;
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

float ndfGGX(vec3 normal, vec3 halfway, float alpha)
{
	float hv = dot(normal, halfway) * dot(normal, halfway);
	float alphaSquared = alpha * alpha;
	return alphaSquared / (M_PI * (((hv * hv) * (alphaSquared - 1.0) + 1.0) * ((hv * hv) * (alphaSquared - 1.0) + 1.0)));
}

float schlickGGC(vec3 normal, vec3 v, float k)
{
	return (dot(normal, v) / max((dot(normal, v) * (1-k) + k), 0.0001));
}

float geometricFunction(vec3 normal, vec3 view, vec3 light, float k)
{
	return schlickGGC(normal, light, k) * schlickGGC(normal, view, k); 
}

vec3 fresnelSchlick(vec3 r0, float cosTheata)
{
	return mix(r0, vec3(1.0f), pow((1.0 - cosTheata), 5.0));
}

vec3 BRDF(vec3 viewDirection, vec3 lightDirection, vec3 normal, vec3 color, float metallic, float roughness, float reflectance)
{
	vec3 halfwayVector = normalize(viewDirection + lightDirection);
	float alpha = roughness * roughness;

	vec3 r0 = mix(vec3(reflectance), color, metallic);

	vec3 specular = (ndfGGX(normal, halfwayVector, alpha) 
						* fresnelSchlick(r0, dot(lightDirection, halfwayVector)) 
						* geometricFunction(normal, viewDirection, lightDirection, alpha * 0.5))
						/ (4 * max(dot(normal, lightDirection), 0.0001) * max(dot(normal, viewDirection), 0.0001));

	vec3 lambert = color / M_PI;

	lambert = lambert * (vec3(1.0) -  fresnelSchlick(r0, dot(lightDirection, halfwayVector))) * (1.0 - metallic);

	return lambert + specular;
}

void main()
{
	float depth = texture(inDepth, texCoord).x;
    vec4 clipSpacePosition = vec4(texCoord * 2.0 - 1.0, depth, 1.0);
    vec4 viewSpacePosition = cameraData.inverseProjectionMatrix * clipSpacePosition;
    viewSpacePosition = viewSpacePosition / viewSpacePosition.w;
	vec4 position = cameraData.inverseViewMatrix * viewSpacePosition;
	

	vec3 normal = normalize(texture(inNormal, texCoord).xyz);
	vec3 color = texture(inColor, texCoord).xyz;
	vec4 roughnessMetalness = texture(inRoughnessMetalness, texCoord);
	
	float reflectance = 0.04;	
	float roughness = roughnessMetalness.x;
	float metallic = roughnessMetalness.y;

	vec3 viewDirection = normalize(cameraData.cameraPosition - position.xyz);

	vec3 radiance = vec3(0.0);

	for(int i = 1; i < globalDrawData.sunLightCount; i++)
	{
	vec3 lightColor = normalize(sunLightBuffer.sunLights[i].color.xyz);
	vec3 lightDirection = -normalize(sunLightBuffer.sunLights[i].direction);
	
	vec3 brdf = BRDF(viewDirection, lightDirection, normal, color, metallic, roughness, reflectance);

	float irradiance = max(dot(normal, lightDirection), 0.0001);
	
	radiance =  radiance + vec3( irradiance );//* brdf * lightColor;	
	}

	for(int i = 0; i < globalDrawData.pointLightCount; i++)
	{

	vec3 lightColor = normalize(pointLightBuffer.pointLights[i].color.xyz);
	vec3 lightDirection = normalize(pointLightBuffer.pointLights[i].position - position.xyz);
	
	vec3 brdf = BRDF(viewDirection, lightDirection, normal, color, metallic, roughness, reflectance);

	float irradiance = max(dot(normal, lightDirection), 0.0);

	radiance =  radiance + vec3( irradiance ) ;//* brdf * lightColor;
	}

	outColor = vec4(radiance, 1.0);
}


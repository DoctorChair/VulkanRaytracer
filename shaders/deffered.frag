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
	uint maxRecoursionDepth;
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

layout(std430,set = 2, binding = 0) readonly buffer SunBuffer{

	SunLight sunLights[];
} sunLightBuffer;

layout(std430,set = 2, binding = 1) readonly buffer PointBuffer{

	PointLight pointLights[];
} pointLightBuffer;

layout(std430,set = 2, binding = 2) readonly buffer SpotBuffer{

	SpotLight spotLights[];
} spotLightBuffer;


#define M_PI 3.1415926535897932384626433832795

float ndfGGX(float normalDotHalfway, float alpha)
{
	float hv = normalDotHalfway * normalDotHalfway;
	float alphaSquared = alpha * alpha;
	return alphaSquared / (M_PI * (((hv * hv) * (alphaSquared - 1.0) + 1.0) * ((hv * hv) * (alphaSquared - 1.0) + 1.0)));
}

float schlickGGX(float v, float k)
{
	return (v / max((v * (1-k) + k), 0.0001));
}

float geometricFunction(float normalDotView, float normalDotLightDir, float k)
{
	return schlickGGX(normalDotView, k) * schlickGGX(normalDotLightDir, k); 
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

	float normalDotLightDir = dot(normal, lightDirection);
	float lightDirDotHalfway = dot(lightDirection, halfwayVector);
	float normalDotView = dot(normal, viewDirection);
	float normalDotHalfway = dot(normal, halfwayVector);

	vec3 fresnel = fresnelSchlick(r0, normalDotLightDir);
	float geometric = geometricFunction(normalDotView, normalDotLightDir, alpha * 0.5);
	float ndf = ndfGGX(normalDotHalfway, alpha);

	vec3 specular = ( ndf * fresnel * geometric )
					/ (4 * max(normalDotLightDir, 0.0001) * max(normalDotView, 0.0001));

	vec3 lambert = color;
	lambert = lambert * (vec3(1.0) - fresnel);
	lambert = lambert * (1.0 - metallic);
	lambert = lambert / M_PI;

	return  lambert  + specular;
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
	vec4 roughnessMetalness = normalize(texture(inRoughnessMetalness, texCoord));
	
	float reflectance = 0.04;	
	float roughness = roughnessMetalness.x;
	float metallic = roughnessMetalness.y;

	vec3 viewDirection = normalize(cameraData.cameraPosition - position.xyz);

	vec3 radiance = vec3(0.0);

	for(int i = 0; i < globalDrawData.sunLightCount; i++)
	{
	vec3 lightColor = normalize(sunLightBuffer.sunLights[i].color.xyz);
	vec3 lightDirection = -normalize(sunLightBuffer.sunLights[i].direction);

	vec3 brdf = BRDF(viewDirection, lightDirection, normal, color, metallic, roughness, reflectance);

	float irradiance = max(dot(normal, lightDirection), 0.0);
	
	radiance =  radiance + irradiance * brdf * lightColor;	
	}

	for(int i = 0; i < globalDrawData.pointLightCount; i++)
	{
	vec3 lightColor = normalize(pointLightBuffer.pointLights[i].color.xyz);

	vec3 lightDirection = pointLightBuffer.pointLights[i].position - position.xyz;
	
	float distance = length(lightDirection);
	float attenuation = 1.0/(distance*distance);

	lightDirection = normalize(lightDirection);

	vec3 brdf = BRDF(viewDirection, lightDirection, normal, color, metallic, roughness, reflectance);

	float irradiance = max(dot(normal, lightDirection), 0.0);

	radiance =  radiance + irradiance * brdf * lightColor * 100.0 * attenuation;
	}

	outColor = vec4(radiance, 1.0f);
}


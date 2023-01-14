#version 460

//shader input
layout (location = 0) in vec2 texCoord;

//output write
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D albedo;
layout(set = 0, binding = 1) uniform sampler2D position;
layout(set = 0, binding = 2) uniform sampler2D normal;
layout(set = 0, binding = 3) uniform sampler2D ID;

struct SunLight
{
	vec3 position;
	vec3 direction;
	vec4 color;
};

struct PointLight
{
	vec3 position;
	vec4 color;
};

struct SpotLight
{
	vec3 position;
	vec3 direction;
	float openingAngle;
	vec4 color;
};

layout(std140,set = 1, binding = 0) readonly buffer SunBuffer{

	SunLight sunLights[];
} sunLightBuffer;

layout(std140,set = 1, binding = 1) readonly buffer PointBuffer{

	PointLight pointLights[];
} pointLightBuffer;

layout(std140,set = 1, binding = 2) readonly buffer SpotBuffer{

	SpotLight spotLights[];
} spotLightBuffer;


void main()
{
	//return color
    vec3 color = texture(position, texCoord).xyz;
	outColor = vec4(color, 1.0f);
}


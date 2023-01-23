#version 460

//shader input
layout (location = 0) in vec4 position;
layout (location = 1) in vec2 TexCoords0;
layout (location = 2) in vec2 TexCoords1;
layout (location = 3) in vec2 TexCoords2;
layout (location = 4) in vec2 TexCoords3;
layout (location = 5) in flat uint albedoIndex;
layout (location = 6) in flat uint normalIndex;
layout (location = 7) in flat uint metallicIndex;
layout (location = 8) in flat uint roughnessIndex;
layout (location = 9) in vec3 T;
layout (location = 10) in vec3 B;
layout (location = 11) in vec3 N;


//output write
layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outRoughnessMetallness;
layout (location = 3) out vec4 outID;
layout (location = 4) out vec4 outPosition;

layout(set = 1, binding = 0) uniform sampler albedoSampler;
layout(set = 1, binding = 1) uniform sampler metallicSampler;
layout(set = 1, binding = 2) uniform sampler normalSampler;
layout(set = 1, binding = 3) uniform sampler roughnessSampler;

layout(set = 3, binding = 0) uniform texture2D textures[1024]; 

void main()
{
	//return color
	vec4 color = texture(sampler2D(textures[albedoIndex], albedoSampler), TexCoords0);
	vec4 normal = texture(sampler2D(textures[normalIndex], normalSampler), TexCoords0);
	vec4 metallic = texture(sampler2D(textures[metallicIndex], metallicSampler), TexCoords0);
	vec4 roughness = texture(sampler2D(textures[roughnessIndex], roughnessSampler), TexCoords0);
	
	mat3 tbnMatrix = mat3(T, -B, N);
	
	normal.xyz = normalize(normal.xyz * 2.0 - 1.0); 
	normal.xyz = tbnMatrix * normal.xyz;
	outNormal =  vec4(normalize(normal.xyz ), normal.w);

	outRoughnessMetallness = metallic;

	outColor = color;
	
	outPosition = position;

	outID = vec4(0.0, 0.0, 0.0, 1.0);
}


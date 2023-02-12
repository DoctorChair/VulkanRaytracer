#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_buffer_reference2: require

#include "BRDF.glsl"

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
	vec2 texCoord0;
	vec2 texCoord1;
	vec2 texCoord2;
	vec2 texCoord3;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
};

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

layout(set = 1, binding = 0) uniform sampler2D primaryRayColorTexture;
layout(set = 1, binding = 1) uniform sampler2D primaryRayNormalTexture;
layout(set = 1, binding = 2) uniform sampler2D primaryRayDepthTexture;

layout(set = 2, binding = 0) uniform accelerationStructureEXT topLevelAS;

 layout(buffer_reference, set = 2, binding = 2) buffer vertexBuffer{
	Vertex v[];
	} vertices;
	
layout(buffer_reference, set = 2, binding = 3) buffer indexBuffer{
	uint i[];
	} indices;
 
layout(std140, set = 3, binding = 0) readonly buffer SunBuffer{

	SunLight sunLights[];
} sunLightBuffer;

layout(std140, set = 3, binding = 1) readonly buffer PointBuffer{

	PointLight pointLights[];
} pointLightBuffer;

layout(std140, set = 3, binding = 2) readonly buffer SpotBuffer{

	SpotLight spotLights[];
} spotLightBuffer; 

layout(set = 4, binding = 0) uniform sampler albedoSampler;
layout(set = 4, binding = 1) uniform sampler metallicSampler;
layout(set = 4, binding = 2) uniform sampler normalSampler;
layout(set = 4, binding = 3) uniform sampler roughnessSampler;

layout(std430, set = 5, binding = 0) readonly buffer DrawInstanceBuffer{
	drawInstanceData instanceData[];
} drawData;

layout(set = 6, binding = 0) uniform texture2D textures[1024]; 

struct hitPayload
{
  bool miss;
  vec3 color;
  vec3 normal;
  vec3 position;
};

layout(location = 1) rayPayloadInEXT hitPayload incomigPayload;
layout(location = 0) rayPayloadEXT float shadowed;

hitAttributeEXT vec2 hitAttribute;

void main()
{
	const vec3 barycentrics = vec3(1.0 - hitAttribute.x - hitAttribute.y, hitAttribute.x, hitAttribute.y);
	
	uint index0 = indices.i[gl_PrimitiveID * 3];
	uint index1 = indices.i[(gl_PrimitiveID * 3) + 1];
	uint index2 = indices.i[(gl_PrimitiveID * 3) + 2];
	
	Vertex v0 = vertices.v[index0];
	Vertex v1 = vertices.v[index1];
	Vertex v2 = vertices.v[index2];
  	
	drawInstanceData instanceData = drawData.instanceData[gl_InstanceCustomIndexEXT];

	mat4 modelMatrix = instanceData.modelMatrix;
    Material material = instanceData.material;

	vec3 position = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
	vec3 worldPosition = (gl_ObjectToWorldEXT * vec4(position, 1.0)).xyz;
    
	vec3 meshNormal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
    vec3 worldMeshNormal = normalize(vec3(gl_WorldToObjectEXT * vec4(meshNormal, 0.0)));
    
	vec3 meshTangnet = v0.tangent * barycentrics.x + v1.tangent * barycentrics.y + v2.tangent * barycentrics.z;
    vec3 worldMeshTangnet = normalize(vec3(gl_WorldToObjectEXT * vec4(meshTangnet, 0.0)));
    
	vec2 texCoord = v0.texCoord0 * barycentrics.x + v0.texCoord0 * barycentrics.y + v2.texCoord0 * barycentrics.z;
    vec3 worldMeshBitagnent = cross(worldMeshTangnet, worldMeshNormal);
    
	mat3 tbnMatrix = {worldMeshTangnet, worldMeshBitagnent, worldMeshNormal};
    
	vec4 colorTexture = texture(sampler2D(textures[material.albedoIndex], albedoSampler), texCoord);
	vec4 normalTexture = texture(sampler2D(textures[material.normalIndex], normalSampler), texCoord);
	vec4 metallicTexture = texture(sampler2D(textures[material.metallicIndex], metallicSampler), texCoord);
    vec4 roughnessTexture = texture(sampler2D(textures[material.roughnessIndex], roughnessSampler), texCoord);
    
	vec3 normal = tbnMatrix * normalTexture.xyz;
	normal = normalize(normal);

	float reflectance = 0.04;	
	float roughness = roughnessTexture.x;
	float metallic = metallicTexture.y;

	uint  shadowRayFlags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
	
	float tMin = 0.01;
	
	float shadowValue = 0.3;

	vec3 radiance = vec3(0.0, 0.0, 0.0);

    for(uint i = 0; i < globalDrawData.pointLightCount; i++)
    {
		vec3 lightDirection = pointLightBuffer.pointLights[i].position - worldPosition;
		float range = length(lightDirection);
		lightDirection = normalize(lightDirection);
		float cosTheta = dot(lightDirection, normal);

		shadowed = shadowValue;
		if(cosTheta > 0)
		{
			traceRayEXT(topLevelAS, // acceleration structure
				shadowRayFlags,       // rayFlags
				0xFF,           // cullMask
				0,              // sbtRecordOffset
				0,              // sbtRecordStride
				1,              // missIndex
				worldPosition,     // ray origin
				tMin,           // ray min range
				lightDirection,  // ray direction
				range,           // ray max range
				0               // payload (location = 0)
			);  


			if(shadowed > shadowValue)
			{
				vec3 lightColor = normalize(pointLightBuffer.pointLights[i].color.xyz);
				float distance = length(lightDirection);

				float attenuation = 1.0/(distance*distance);

				lightDirection = normalize(lightDirection);

				vec3 brdf = BRDF(gl_WorldRayDirectionEXT, lightDirection, normal, colorTexture.xyz, metallic, roughness, reflectance);
				float irradiance = max(dot(normal, lightDirection), 0.0);
				radiance =  radiance + irradiance * brdf * lightColor * 50.0 * attenuation;
			}
				
   		}
	}

    for(uint i = 0; i < globalDrawData.sunLightCount; i++)
    { 
    }

    for(uint i = 0; i < globalDrawData.spotLightCount; i++)
    { 
    }

	incomigPayload.color = radiance;
	incomigPayload.normal = normal; 
	incomigPayload.position = worldPosition;
}
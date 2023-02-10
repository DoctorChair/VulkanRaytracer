#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require


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
layout(std430, set = 2, binding = 2) buffer vertexBuffer{
	Vertex v[];
	} vertices;
	
layout(std430, set = 2, binding = 3) buffer indexBuffer{
	ivec3 i[];
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

layout(std430, set = 5, binding = 0) readonly buffer DrawInstanceBuffer{
	drawInstanceData instanceData[];
} drawData;

struct hitPayload
{
  bool shadowed;
  vec3 barycentrics;
  uint primitiveID;
  uint instanceID;
};

layout(location = 1) rayPayloadInEXT hitPayload incomigPayload;
layout(location = 0) rayPayloadEXT float shadowed;

hitAttributeEXT vec2 hitAttribute;

void main()
{
	const vec3 barycentrics = vec3(1.0 - hitAttribute.x - hitAttribute.y, hitAttribute.x, hitAttribute.y);
	
	incomigPayload.barycentrics = barycentrics;
	incomigPayload.primitiveID = gl_PrimitiveID;
	incomigPayload.instanceID = gl_InstanceCustomIndexEXT;
	
	ivec3 indexTripplet = indices.i[gl_PrimitiveID];
	
	Vertex v0 = vertices.v[indexTripplet.x];
	Vertex v1 = vertices.v[indexTripplet.y];
	Vertex v2 = vertices.v[indexTripplet.z];
  	
	vec3 position = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
	vec3 worldPosition = (gl_ObjectToWorldEXT  * vec4(position, 1.0)).xyz;
	
	vec3 meshNormal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
	vec3 worldMeshNormal = normalize(vec3(meshNormal * gl_ObjectToWorldEXT));
	
	uint  shadowRayFlags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
	
	vec3 lightDirection = pointLightBuffer.pointLights[0].position - worldPosition;
    float range = length(lightDirection);
    lightDirection = normalize(lightDirection);
    worldMeshNormal = normalize(worldMeshNormal);
    float cosTheta = dot(lightDirection, worldMeshNormal);
	
	float tMin = 0.01;

    shadowed = 0.0;

    traceRayEXT(topLevelAS, // acceleration structure
    	shadowRayFlags,       // rayFlags
    	0xFF,           // cullMask
    	0,              // sbtRecordOffset
    	0,              // sbtRecordStride
    	1,              // missIndex
    	worldPosition,     // ray origin
    	tMin,           // ray min range
    	worldMeshNormal,  // ray direction
    	range,           // ray max range
    	0               // payload (location = 0)
    ); 

	incomigPayload.shadowed = bool(int(shadowed));
}
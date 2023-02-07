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
  vec3 hitValue;
  int depth;
};

layout(location = 1) rayPayloadInEXT hitPayload incomigPayload;


hitAttributeEXT vec2 hitAttribute;

void main()
{
	if(incomigPayload.depth < globalDrawData.maxRecoursionDepth)
	{
		const vec3 barycentrics = vec3(1.0 - hitAttribute.x - hitAttribute.y, hitAttribute.x, hitAttribute.y);

		ivec3 indexTripplet = indices.i[gl_PrimitiveID];

		Vertex v0 = vertices.v[indexTripplet.x];
		Vertex v1 = vertices.v[indexTripplet.y];
		Vertex v2 = vertices.v[indexTripplet.z];

  		uint albedoIndex = drawData.instanceData[gl_InstanceCustomIndexEXT].material.albedoIndex;
		uint normalIndex = drawData.instanceData[gl_InstanceCustomIndexEXT].material.normalIndex;
		uint metallicIndex = drawData.instanceData[gl_InstanceCustomIndexEXT].material.metallicIndex;
		uint roughnessIndex = drawData.instanceData[gl_InstanceCustomIndexEXT].material.roughnessIndex;

		vec3 position = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
		vec3 worldPosition = (gl_ObjectToWorldEXT  * vec4(position, 1.0)).xyz;

		vec3 meshNormal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
		vec3 worldMeshNormal = normalize(vec3(meshNormal * gl_ObjectToWorldEXT));

		vec3 tangent = v0.tangent * barycentrics.x + v1.tangent * barycentrics.y + v2.tangent * barycentrics.z;
		vec3 worldTangent = normalize(vec3(tangent * gl_ObjectToWorldEXT));

		vec3 worldBitangent = normalize(cross(worldTangent, worldMeshNormal));

		vec2 texCoord = v0.texCoord0 * barycentrics.x + v1.texCoord0 * barycentrics.y + v2.texCoord0 * barycentrics.z;

		mat3 tbnMatrix = {worldTangent, worldBitangent, worldMeshNormal};

		vec4 abledo = texture(sampler2D(textures[albedoIndex], albedoSampler), texCoord);
		vec4 normal = texture(sampler2D(textures[normalIndex], metallicSampler), texCoord);
		vec4 metallic = texture(sampler2D(textures[metallicIndex], normalSampler), texCoord);
		vec4 roughness = texture(sampler2D(textures[roughnessIndex], roughnessSampler), texCoord);

		normal.xyz = normal.xyz * tbnMatrix;

		uint  rayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;

		vec3 incommingDirection = normalize(gl_WorldRayDirectionEXT);
    	vec3 outgoingdirection = reflect(incommingDirection, normal.xyz);

		float tMin = 0.001;
		float tMax = 10000.0;
		
		incomigPayload.depth++;

		traceRayEXT(topLevelAS, // acceleration structure
    		rayFlags,       // rayFlags
    		0xFF,           // cullMask
    		0,              // sbtRecordOffset
    		0,              // sbtRecordStride
    		0,              // missIndex
    		vec3(0.0, 0.0, 0.0),     // ray origin
    		tMin,           // ray min range
    		vec3(0.0, 1.0, 0.0),  // ray direction
    		tMax,           // ray max range
    		1               // payload (location = 1)
    		);

		incomigPayload.depth--; 
	} 
	
	incomigPayload.hitValue = vec3(1.0, 0.0, 0.0);
}
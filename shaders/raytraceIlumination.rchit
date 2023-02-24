#version 460
#extension GL_EXT_ray_tracing : require
//#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference2: require

#include "BRDF.glsl"
#include "common.glsl"

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
  	uint maxDiffuseSampleCount;
  	uint maxSpecularSampleCount;
  	uint maxShadowRaySampleCount;
  	uint noiseSampleTextureIndex;
} globalDrawData;

layout(set = 1, binding = 0) uniform sampler2D primaryRayColorTexture;
layout(set = 1, binding = 1) uniform sampler2D primaryRayNormalTexture;
layout(set = 1, binding = 2) uniform sampler2D primaryRayDepthTexture;

layout(set = 2, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices {uint i[]; }; // Triangle indices

layout(std430, set = 2, binding = 2) buffer VertexBuffer
{
	uvec2 address;
}vertexAddress;

layout(std430 ,set = 2, binding = 3) buffer IndexBuffer
{
	uvec2 address;
}indexAddress;

layout(std430, set = 3, binding = 0) readonly buffer SunBuffer{

	SunLight sunLights[];
} sunLightBuffer;

layout(std430, set = 3, binding = 1) readonly buffer PointBuffer{

	PointLight pointLights[];
} pointLightBuffer;

layout(std430, set = 3, binding = 2) readonly buffer SpotBuffer{

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
  vec3 radiance;
  uint depth;
};

layout(location = 0) rayPayloadEXT float shadowPayload;
layout(location = 1) rayPayloadInEXT hitPayload incomigPayload;

hitAttributeEXT vec2 hitAttribute;

void main()
{
	const vec3 barycentrics = vec3(1.0 - hitAttribute.x - hitAttribute.y, hitAttribute.x, hitAttribute.y);
	
	uint index0;
	uint index1;
	uint index2;

	Vertex v0;
	Vertex v1;
	Vertex v2;
  	
	Vertices vertices = Vertices(vertexAddress.address);
	Indices indices = Indices(indexAddress.address);

	index0 = indices.i[gl_PrimitiveID * 3];
	index1 = indices.i[gl_PrimitiveID * 3 + 1];
	index2 = indices.i[gl_PrimitiveID * 3 + 2];

	v0 = vertices.v[index0];
	v1 = vertices.v[index1];
	v2 = vertices.v[index2];
 
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
    vec3 worldMeshBitagnent = cross(worldMeshNormal, worldMeshTangnet);
    
	mat3 tbnMatrix = {worldMeshTangnet, -worldMeshBitagnent, worldMeshNormal};
    
	vec4 colorTexture = texture(sampler2D(textures[material.albedoIndex], albedoSampler), texCoord);
	vec4 normalTexture = texture(sampler2D(textures[material.normalIndex], normalSampler), texCoord);
	vec4 metallicTexture = texture(sampler2D(textures[material.metallicIndex], metallicSampler), texCoord);
    vec4 roughnessTexture = texture(sampler2D(textures[material.roughnessIndex], roughnessSampler), texCoord);
    
	vec3 normal = tbnMatrix * normalTexture.xyz;
	normal = normalize(normal);

	float reflectance = 0.04;	
	float roughness = roughnessTexture.x;
	float metallic = metallicTexture.y;

	uint  rayFlags = gl_RayFlagsOpaqueEXT;
	uint  shadowRayFlags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
	
	float tMin = 0.01;
	float tMax = 1000.0;

	vec3 radiance = vec3(0.0, 0.0, 0.0);

	if(incomigPayload.depth < globalDrawData.maxRecoursionDepth)
	{
		for(uint i = 0; i < globalDrawData.pointLightCount; i++)
		{
			shadowPayload = 0.0;

			vec3 lightDirection = pointLightBuffer.pointLights[i].position - worldPosition;
    	  	float distance = length(lightDirection);
    	  	lightDirection = normalize(lightDirection);
    	  	float cosTheta = dot(lightDirection, normal);

			traceRayEXT(topLevelAS, // acceleration structure
    	    shadowRayFlags,       // rayFlags
    	    0xFF,           // cullMask
    	    0,              // sbtRecordOffset
    	    0,              // sbtRecordStride
    	    1,              // missIndex
    	    worldPosition,     // ray origin
    	    tMin,           // ray min range
    	    lightDirection,  // ray direction
    	    distance,           // ray max range
    	    0               // payload (location = 0)
    	    );

			vec3 lightColor = normalize(pointLightBuffer.pointLights[i].color.xyz);

			float attenuation = 1.0/(distance*distance);

			vec3 viewDirection = worldPosition - gl_WorldRayOriginEXT;

		    vec3 brdf = cookTorranceGgxBRDF(-viewDirection, lightDirection, normal, colorTexture.xyz, metallic, roughness, reflectance);

			vec3 lightRadiance = lightColor * pointLightBuffer.pointLights[i].strength * max(cosTheta, 0.0) * attenuation * shadowPayload;

			radiance = radiance + brdf * lightRadiance;
	}

	for(uint i = 0; i < globalDrawData.sunLightCount; i++)
	{
		shadowPayload = 0.0;

		vec3 lightDirection = sunLightBuffer.sunLights[i].direction;
		float cosTheta = dot(lightDirection, normal);

		traceRayEXT(topLevelAS, // acceleration structure
        shadowRayFlags,       // rayFlags
        0xFF,           // cullMask
        0,              // sbtRecordOffset
        0,              // sbtRecordStride
        1,              // missIndex
        worldPosition,     // ray origin
        tMin,           // ray min range
        lightDirection,  // ray direction
        tMax,           // ray max range
        0               // payload (location = 0)
        );

		vec3 lightColor = normalize(sunLightBuffer.sunLights[i].color.xyz);
		
		vec3 viewDirection = worldPosition - gl_WorldRayOriginEXT;

	    vec3 brdf = cookTorranceGgxBRDF(-viewDirection, lightDirection, normal, colorTexture.xyz, metallic, roughness, reflectance);

		vec3 lightRadiance = lightColor* sunLightBuffer.sunLights[i].strength * max(cosTheta, 0.0) * shadowPayload;

		radiance = radiance + brdf * lightRadiance;
	}
		
	if(incomigPayload.depth < globalDrawData.maxRecoursionDepth+1)
	{
	
	incomigPayload.depth++;

	vec4 screenNoise = texture(sampler2D(textures[globalDrawData.noiseSampleTextureIndex], albedoSampler), vec2(float(gl_LaunchIDEXT)));

	vec3 diffuseRadiance = vec3(0.0);
	for(uint i = 0; i < globalDrawData.maxDiffuseSampleCount; i++)
	{	
		vec4 noise = texture(sampler2D(textures[globalDrawData.noiseSampleTextureIndex], albedoSampler), screenNoise.xy + vec2(1.0/float(i)));

		float pdfValue = lambertImportancePDF(noise.x);
		vec3 sampleDirection = createSampleVector(normal, 0.5 * M_PI, 2.0 * M_PI, pdfValue, noise.y);

		float cosTheta = dot(sampleDirection, normal);

		traceRayEXT(topLevelAS, // acceleration structure
        rayFlags,       // rayFlags
        0xFF,           // cullMask
        0,              // sbtRecordOffset
        0,              // sbtRecordStride
        0,              // missIndex
        worldPosition,     // ray origin
        tMin,           // ray min range
        sampleDirection,  // ray direction
        tMax,           // ray max range
        1               // payload (location = 0)
        );

		diffuseRadiance = diffuseRadiance + colorTexture.xyz * max(cosTheta, 0.0) * incomigPayload.radiance;
	}

	diffuseRadiance = diffuseRadiance / float(globalDrawData.maxDiffuseSampleCount);

	radiance = radiance + diffuseRadiance;

	/* for(uint i = 0; i < globalDrawData.maxSpecularSampleCount; i++)
	{
		vec3 reflectionDirection = reflect(gl_WorldRayDirectionEXT, normal);

		float cosTheta = dot(reflectionDirection, gl_WorldRayDirectionEXT);

		traceRayEXT(topLevelAS, // acceleration structure
        rayFlags,       // rayFlags
        0xFF,           // cullMask
        0,              // sbtRecordOffset
        0,              // sbtRecordStride
        0,              // missIndex
        worldPosition,     // ray origin
        tMin,           // ray min range
        reflectionDirection,  // ray direction
        tMax,           // ray max range
        1               // payload (location = 0)
        );
	}  */
	
	incomigPayload.depth--;
	}
	} 

	
	incomigPayload.radiance = radiance;
}
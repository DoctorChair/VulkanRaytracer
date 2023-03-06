#version 460
#extension GL_EXT_ray_tracing : require
//#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference2: require
#extension GL_EXT_buffer_reference_uvec2 : require

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
	uint sampleSequenceLength;
	uint frameNumber;
} globalDrawData;

layout(set = 1, binding = 0) uniform sampler2D primaryRayColorTexture;
layout(set = 1, binding = 1) uniform sampler2D primaryRayNormalTexture;
layout(set = 1, binding = 2) uniform sampler2D primaryRayDepthTexture;

layout(set = 2, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(buffer_reference, scalar) readonly buffer Vertices {Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer Indices {uint i[]; }; // Triangle indices

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer VertexReference {Vertex v; };
layout(buffer_reference, std430, buffer_reference_align = 4) readonly buffer IndexReference {uint i; };

layout(std430, set = 2, binding = 2) buffer VertexBuffer
{
	uvec2 vertexAddress;
	uvec2 instanceAddress;
}meshBufferAddresses;

layout(std430, set = 2, binding = 4) readonly buffer sampleSequenceBuffer{
  vec2 samples[];
} sampleSequence;

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

hitAttributeEXT vec2 attribs;

uvec2 uadd_64_32(uvec2 addr, uint offset)
{
    uint carry;
    addr.x = uaddCarry(addr.x, offset, carry);
    addr.y += carry;
    return addr;
}

void main()
{
	vec3 radiance = vec3(0.0);

	const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

	uint index0;
	uint index1;
	uint index2;

	Vertex v0;
	Vertex v1;
	Vertex v2;
	
	drawInstanceData instanceData = drawData.instanceData[gl_InstanceCustomIndexEXT];

	Vertices vertices = Vertices(meshBufferAddresses.vertexAddress);
	Indices indices = Indices(meshBufferAddresses.instanceAddress);
											
	index0 = indices.i[gl_PrimitiveID * 3 + instanceData.indicesOffset] + instanceData.vertexOffset;
	index1 = indices.i[gl_PrimitiveID * 3 + 1 + instanceData.indicesOffset] + instanceData.vertexOffset;
	index2 = indices.i[gl_PrimitiveID * 3 + 2 + instanceData.indicesOffset] + instanceData.vertexOffset;
	
	v0 = vertices.v[index0];
	v1 = vertices.v[index1];
	v2 = vertices.v[index2];
	
	mat4 modelMatrix = instanceData.modelMatrix;
    Material material = instanceData.material;

	vec3 position = v0.position.xyz * barycentrics.x + v1.position.xyz * barycentrics.y + v2.position.xyz * barycentrics.z;
	vec3 worldPosition = vec3((gl_ObjectToWorldEXT * vec4(position, 1.0)));
    
	vec3 meshNormal = v0.normal.xyz * barycentrics.x + v1.normal.xyz * barycentrics.y + v2.normal.xyz * barycentrics.z;
    vec3 worldMeshNormal = normalize(vec3(gl_WorldToObjectEXT * vec4(meshNormal, 0.0)));
    
	vec3 meshTangnet = v0.tangent.xyz * barycentrics.x + v1.tangent.xyz * barycentrics.y + v2.tangent.xyz * barycentrics.z;
    vec3 worldMeshTangnet = normalize(vec3(gl_WorldToObjectEXT * vec4(meshTangnet, 0.0)));
    
	vec2 texCoord = v0.texCoordPair.xy * barycentrics.x + v1.texCoordPair.xy * barycentrics.y + v2.texCoordPair.xy * barycentrics.z;

    vec3 worldMeshBitagnent = cross(worldMeshNormal, worldMeshTangnet);
    
	mat3 tbnMatrix = {worldMeshTangnet, -worldMeshBitagnent, worldMeshNormal};
	float tMin = 0.01;
	float tMax = 1000.0;

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

	if(incomigPayload.depth < globalDrawData.maxRecoursionDepth)
	{
		for(uint i = 0; i < globalDrawData.pointLightCount; i++)
		{
			shadowPayload = 0.0;

			vec3 lightDirection = pointLightBuffer.pointLights[i].position - worldPosition;
    	  	float distance = length(lightDirection);
    	  	lightDirection = normalize(lightDirection);
    	  	float cosTheta = dot(lightDirection, normal);
			
			float radius = pointLightBuffer.pointLights[i].radius;
			float area = 3*M_PI*pow(radius, 2.0);

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

			vec3 lightRadiance = lightColor * pointLightBuffer.pointLights[i].strength * max(cosTheta, 0.0) * attenuation * shadowPayload * area;

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
	vec3 diffuseRadiance = vec3(0.0);

	incomigPayload.depth++;

	vec4 screenNoise = texture(sampler2D(textures[globalDrawData.noiseSampleTextureIndex], albedoSampler), vec2(float(gl_LaunchIDEXT)));
	
	for(uint i = 0; i < globalDrawData.maxDiffuseSampleCount; i++)
	{	
		uint index = i % globalDrawData.sampleSequenceLength;
    	vec2 noise = sampleSequence.samples[index];
    
    	float pdfValue = lambertImportancePDF(noise.x);
    	vec3 sampleDirection = createSampleVector(normal.xyz, 0.5 * M_PI, 2.0 * M_PI, pdfValue, noise.y);

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

		diffuseRadiance = diffuseRadiance + colorTexture.xyz * max(cosTheta, 0.0) * incomigPayload.radiance / pdfValue;
	}

	diffuseRadiance = diffuseRadiance / float(globalDrawData.maxDiffuseSampleCount);
	
	radiance = radiance + diffuseRadiance; 

	}
	} 

	
	incomigPayload.radiance = radiance / 2.0;
}
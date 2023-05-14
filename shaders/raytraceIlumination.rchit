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
	uint sunLightCount;
	uint pointLightCount;
	uint maxRecoursionDepth;
	uint maxDiffuseSampleCount;
	uint maxSpecularSampleCount;
	uint maxShadowRaySampleCount;
	uint noiseSampleTextureIndex;
	uint frameNumber;
	uint historyLength;
	uint historyIndex;
	uint nativeResolutionWidth;
	uint nativeResolutionHeight;
	uint environmentTextureIndex;
	float heuristicExponent;
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

layout(std430, set = 3, binding = 0) readonly buffer SunBuffer{

	DirectionalLight sunLights[];
} sunLightBuffer;

layout(std430, set = 3, binding = 1) readonly buffer PointBuffer{

	SphereLight pointLights[];
} pointLightBuffer;

layout(set = 4, binding = 0) uniform sampler linearSampler;
layout(set = 4, binding = 1) uniform sampler nearestSampler;
layout(set = 4, binding = 2) uniform sampler lowFidelitySampler;

layout(std430, set = 5, binding = 0) readonly buffer DrawInstanceBuffer{
	drawInstanceData instanceData[];
} drawData;

layout(set = 6, binding = 0) uniform texture2D textures[1024]; 

struct hitPayload
{
	uint depth;
  	vec3 radiance;
	bool HitLightSource;
};

layout(location = 0) rayPayloadEXT float shadowPayload;
layout(location = 1) rayPayloadInEXT hitPayload incomigPayload;

hitAttributeEXT vec2 attribs;

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
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
	vec3 worldNormal = normalize(normalMatrix * normalize(meshNormal.xyz));

	vec2 texCoord = v0.texCoordPair.xy * barycentrics.x + v1.texCoordPair.xy * barycentrics.y + v2.texCoordPair.xy * barycentrics.z;

	
	vec4 colorTexture = texture(sampler2D(textures[material.albedoIndex], nearestSampler), texCoord);
	vec4 normalTexture = texture(sampler2D(textures[material.normalIndex], nearestSampler), texCoord);
	vec4 metallicTexture = texture(sampler2D(textures[material.metallicIndex], nearestSampler), texCoord);
    vec4 roughnessTexture = texture(sampler2D(textures[material.roughnessIndex], nearestSampler), texCoord);
    vec4 emissionTexture = texture(sampler2D(textures[material.emissionIndex], nearestSampler), texCoord);
	vec3 normal;

	float emissiveStrength = instanceData.emissionIntensity;
	
	if(material.normalIndex != 0)
	{
		vec3 meshTangnet = v0.tangent.xyz * barycentrics.x + v1.tangent.xyz * barycentrics.y + v2.tangent.xyz * barycentrics.z;
		vec3 worldTangent =  normalize(normalMatrix * normalize(meshTangnet.xyz));
		worldTangent = normalize(worldTangent - dot(worldTangent, worldNormal) * worldNormal);
		vec3 worldBitagnent =  normalize(cross(worldNormal, worldTangent));
		mat3 tbnMatrix = {worldTangent, -worldBitagnent, worldNormal};

		vec3 n = normalize(normalTexture.xyz * 2.0 - 1.0); 

		normal = normalize(tbnMatrix * n);
	}
	else
	{
		normal = worldNormal;
	}

	float tMin = 0.01;
	float tMax = 100.0;
	float reflectance = 0.04;	
	float roughness = roughnessTexture.y;
	float metallic = metallicTexture.z;
	
	float filterExponent = globalDrawData.heuristicExponent;

	uint  rayFlags = gl_RayFlagsOpaqueEXT;
	uint  shadowRayFlags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;

	vec2 imageDimesions = vec2(float(globalDrawData.nativeResolutionWidth), float(globalDrawData.nativeResolutionHeight));

	vec2 offset = vec2(float((globalDrawData.frameNumber) % globalDrawData.historyLength)) / imageDimesions;
  	vec4 screenNoise = texture(sampler2D(textures[globalDrawData.noiseSampleTextureIndex], linearSampler), gl_LaunchIDEXT.xy/imageDimesions * imageDimesions / 1024.0 + offset);

	if(incomigPayload.depth < globalDrawData.maxRecoursionDepth)
	{
		for(uint i = 0; i < globalDrawData.pointLightCount; i++)
		{
			shadowPayload = 0.0;

			vec3 lightDirection = pointLightBuffer.pointLights[i].position - worldPosition;
    	  	float distance = length(lightDirection);
    	  	lightDirection = normalize(lightDirection);
    	  	float cosTheta = max(dot(lightDirection, normal), 0.0);
			if(cosTheta >= 0)
			{
			float radius = pointLightBuffer.pointLights[i].radius;
			float area = 3*M_PI*pow(radius, 2.0);

			traceRayEXT(topLevelAS, // acceleration structure
    	    shadowRayFlags,       // rayFlags
    	    0x01,           // cullMask
    	    0,              // sbtRecordOffset
    	    0,              // sbtRecordStride
    	    1,              // missIndex
    	    worldPosition,     // ray origin
    	    tMin,           // ray min range
    	    lightDirection,  // ray direction
    	    distance,           // ray max range
    	    0               // payload (location = 0)
    	    );

			vec2 sphereUV = vec2(atan(-lightDirection.x, -lightDirection.z) / (2*M_PI) + 0.5, -lightDirection.y * 0.5 + 0.5);
      		vec4 lightEmission = texture(sampler2D(textures[pointLightBuffer.pointLights[i].emissionIndex], linearSampler), sphereUV); 

			float attenuation = 1.0/max((distance*distance), 0.001);

			vec3 viewDirection = normalize(worldPosition - gl_WorldRayOriginEXT);

		    vec3 brdf = cookTorranceGgxBRDF(-viewDirection, lightDirection, normal, colorTexture.xyz, metallic, roughness, reflectance);

			vec3 lightRadiance = lightEmission.xyz * pointLightBuffer.pointLights[i].strength * cosTheta * attenuation * shadowPayload * area;

			radiance = radiance + brdf * lightRadiance;
			}
		}

	for(uint i = 0; i < globalDrawData.sunLightCount; i++)
	{
		shadowPayload = 0.0;

		vec3 lightDirection = -sunLightBuffer.sunLights[i].direction;
		float area = M_PI*pow(sunLightBuffer.sunLights[i].radius, 2.0);
		float cosTheta = dot(lightDirection, normal);
		
		if(cosTheta >= 0)
		{
		traceRayEXT(topLevelAS, // acceleration structure
        shadowRayFlags,       // rayFlags
        0x01,           // cullMask
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
		
		vec3 viewDirection = normalize(worldPosition - gl_WorldRayOriginEXT);

	    vec3 brdf = cookTorranceGgxBRDF(-viewDirection, lightDirection, normal, colorTexture.xyz, metallic, roughness, reflectance);

		vec3 lightRadiance = lightColor * sunLightBuffer.sunLights[i].strength * max(cosTheta, 0.0) * shadowPayload * area;

		radiance = radiance + brdf * lightRadiance;
		}
	}


	vec3 diffuseRadiance = vec3(0.0);
	
	if(incomigPayload.depth + 1 < globalDrawData.maxRecoursionDepth)
	{
		incomigPayload.depth++;	

		incomigPayload.HitLightSource = false;

		if(metallic < 0.8)
    	{
		vec4 noise = texture(sampler2D(textures[globalDrawData.noiseSampleTextureIndex], linearSampler), screenNoise.xy);
    
		float ggxPdfValue = ggxImportanceCDF(noise.x, roughness * roughness);
		ggxPdfValue = ggxImportancePDF(ggxPdfValue, roughness * roughness);
    	vec3 sampleDirection = createCosineWeightedHemisphereSample(normal.xyz, noise.x, noise.y * 2.0 - 1.0);
    	float pdfValue = sqrt(1.0 - noise.x);

		float cosTheta =  max(dot(sampleDirection, normal), 0.0);

		float weight = veachPowerHeurisitk(pdfValue , ggxPdfValue, filterExponent);
		weight = weight / pdfValue;

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
        1               // payload (location = 1)
        );

		vec3 r0 = mix(vec3(reflectance), colorTexture.xyz, metallic);
    	vec3 fresnel = fresnelSchlick(r0, cosTheta);
		vec3 lambert = colorTexture.xyz/M_PI * (vec3(1.0) - fresnel) * (1.0 - metallic);
		
		diffuseRadiance = diffuseRadiance + weight * (lambert * cosTheta * incomigPayload.radiance / pdfValue);
		}

		incomigPayload.depth--;
	}

		vec3 specularRadiance = vec3(0.0);

		incomigPayload.HitLightSource = false;

		if(incomigPayload.depth + 1 < globalDrawData.maxRecoursionDepth)
		{
		 
		incomigPayload.depth++;
		
		vec4 noise = texture(sampler2D(textures[globalDrawData.noiseSampleTextureIndex], linearSampler), screenNoise.yx);
    
    	float pdfValue = ggxImportanceCDF(noise.x, roughness);
    	float lambertPDFValue = sqrt(1.0 - noise.x);

		pdfValue = max(pdfValue, 0.001);
		float p = ggxImportancePDF(pdfValue, roughness * roughness);
    	float weight = veachPowerHeurisitk(p ,lambertPDFValue, filterExponent);
		weight = weight / p *  (float(metallic < 0.8)) + 1.0 * (float(metallic >= 0.8));

		vec3 halfwayVector = normalize(createSampleVector(normal.xyz, 1.0, 2.0 * M_PI, pdfValue, noise.y * 2.0 - 1.0));

		vec3 incomingDirection = normalize(worldPosition - gl_WorldRayOriginEXT);
    	vec3 reflectionDirection = reflect(incomingDirection, halfwayVector);

		float cosTheta =  max(dot(reflectionDirection, normal), 0.0);

		traceRayEXT(topLevelAS, // acceleration structure
        rayFlags,       // rayFlags
        0xFF,           // cullMask
        0,              // sbtRecordOffset
        0,              // sbtRecordStride
        0,              // missIndex
        worldPosition.xyz,     // ray origin
        tMin,           // ray min range
        reflectionDirection,  // ray direction
        tMax,           // ray max range
        1               // payload (location = 0)
        );
		
		vec3 brdf = ggxSpecularBRDF(-incomingDirection, reflectionDirection, halfwayVector, normal.xyz, colorTexture.xyz, metallic, roughness, reflectance);

		specularRadiance = specularRadiance + weight * ( incomigPayload.radiance * cosTheta * brdf);

		incomigPayload.depth--; 
		}

	

	radiance = radiance + specularRadiance +  diffuseRadiance;
	
	} 
	
	radiance = radiance + emissionTexture.xyz;
	incomigPayload.HitLightSource = (instanceData.emissionIntensity>0.0);
	incomigPayload.radiance = radiance; 
}

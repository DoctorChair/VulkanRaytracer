#define M_PI 3.1415926535897932384626433832795

float ndfGGX(float normalDotHalfway, float alpha)
{
	float hv = normalDotHalfway * normalDotHalfway;
	float alphaSquared = alpha * alpha;
	return alphaSquared / (M_PI * (((hv * hv) * (alphaSquared - 1.0) + 1.0) * ((hv * hv) * (alphaSquared - 1.0) + 1.0)));
}

float ndfPhong(float normalDotHalfway, float roughness)
{
	return pow(normalDotHalfway, 32.0);
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

vec3 cookTorranceGgxBRDF(vec3 viewDirection, vec3 lightDirection, vec3 normal, vec3 color, float metallic, float roughness, float reflectance)
{
	vec3 halfwayVector = normalize(viewDirection + lightDirection);
	float alpha = roughness * roughness;

	vec3 r0 = mix(vec3(reflectance), color, metallic);

	float normalDotLightDir = dot(normal, lightDirection);
	float normalDotView = dot(normal, viewDirection);
	float normalDotHalfway = max(dot(normal, halfwayVector), 0.0);

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

vec3 cookTorrancePhongBRDF(vec3 viewDirection, vec3 lightDirection, vec3 normal, vec3 color, float metallic, float roughness, float reflectance)
{
	vec3 halfwayVector = normalize(viewDirection + lightDirection);
	float alpha = roughness * roughness;

	vec3 r0 = mix(vec3(reflectance), color, metallic);

	float normalDotLightDir = dot(normal, lightDirection);
	float normalDotView = dot(normal, viewDirection);
	float normalDotHalfway = max(dot(normal, halfwayVector), 0.0);

	vec3 fresnel = fresnelSchlick(r0, normalDotLightDir);
	float geometric = geometricFunction(normalDotView, normalDotLightDir, alpha * 0.5);
	float ndf = ndfPhong(normalDotHalfway, 1.0 - roughness);

	vec3 specular = ( ndf * fresnel * geometric )
					/ (4 * max(normalDotLightDir, 0.0001) * max(normalDotView, 0.0001));

	vec3 lambert = color;
	lambert = lambert * (vec3(1.0) - fresnel);
	lambert = lambert * (1.0 - metallic);
	lambert = lambert / M_PI;

	return  lambert  + specular;
}

vec3 createSampleVector(vec3 originVector, float maxThetaDeviationAngle, float maxPhiDeviationAngle, float randomTheta, float randomPhi)
{
	vec3 sampleVector;

	vec3 tangent = normalize(vec3(0.0, -originVector.z, originVector.y) * float(originVector.y >= originVector.x) 
	+ vec3(originVector.z, 0.0, -originVector.x) * float(originVector.x > originVector.y));
	
	originVector = normalize(originVector);
	vec3 bitangnet = cross(originVector, tangent);

	mat3 tbnMatrix = mat3(tangent, bitangnet, originVector);

	sampleVector.x = sin(maxThetaDeviationAngle * randomTheta) * cos(maxPhiDeviationAngle * randomPhi);
	sampleVector.y = cos(maxThetaDeviationAngle * randomTheta);
	sampleVector.z = sin(maxThetaDeviationAngle * randomTheta) * sin(maxPhiDeviationAngle * randomPhi);

	return tbnMatrix * normalize(sampleVector.xzy);
}
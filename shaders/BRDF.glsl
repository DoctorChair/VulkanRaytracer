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
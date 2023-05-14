#define M_PI 3.1415926535897932384626433832795

float ndfGGX(float normalDotHalfway, float alpha)
{
	float alpha2 = alpha*alpha;
    float normalDotHalfway2 = normalDotHalfway*normalDotHalfway;
	
    float a  = alpha2;
    float b  =  M_PI * (normalDotHalfway2 * (alpha2 - 1.0) + 1.0);
	return min(max(a / (b * b), 0.0), 1.0);
}

float schlickGGX(float v, float k)
{
	return (v / max((v * (1-k) + k), 0.0001));
}

float geometricFunction(float normalDotView, float normalDotLightDir, float k)
{
	return schlickGGX(normalDotView, k) * schlickGGX(normalDotLightDir, k); 
}

vec3 fresnelSchlick(vec3 r0, float cosTheta)
{
	return r0 + (1.0f - r0) * pow(1.0 - cosTheta, 5.0);
}

vec3 cookTorranceGgxBRDF(vec3 viewDirection, vec3 lightDirection, vec3 normal, vec3 color, float metallic, float roughness, float reflectance)
{
	vec3 halfwayVector = normalize(viewDirection + lightDirection);

	vec3 r0 = vec3(reflectance);
	r0 = mix(r0, color, metallic);

	float normalDotLightDir = max(dot(normal, lightDirection), 0.0);
	float normalDotView = max(dot(normal, viewDirection), 0.0);
	float normalDotHalfway = min(max(dot(normal, halfwayVector),  0.0), 0.999999999);

	vec3 fresnel = fresnelSchlick(r0, normalDotView);
	float geometric = geometricFunction(normalDotView, normalDotLightDir, pow(roughness+1.0, 2.0) / 8.0);
	float ndf = ndfGGX(normalDotHalfway, roughness * roughness);

	vec3 specular = ( ndf * fresnel * geometric )
					/ max(4 * normalDotLightDir * normalDotView, 0.00001);

	vec3 lambert = color;
	lambert = lambert * (vec3(1.0) - fresnel) * (1.0 - metallic);
	lambert = lambert / M_PI;

	return lambert + specular;
}


vec3 ggxSpecularBRDF(vec3 viewDirection, vec3 lightDirection, vec3 normal, vec3 halfwayVector, vec3 color, float metallic, float roughness, float reflectance)
{
	vec3 r0 = vec3(reflectance);
	r0 = mix(r0, color, metallic);

	float normalDotLightDir = max(dot(normal, lightDirection), 0.0);
	float normalDotView = max(dot(normal, viewDirection), 0.0);
	float normalDotHalfway = max(dot(normal, halfwayVector),  0.0);

	vec3 fresnel = fresnelSchlick(r0, normalDotView);
	float geometric = geometricFunction(normalDotView, normalDotLightDir, roughness * roughness * 0.5);
	float ndf = ndfGGX(normalDotHalfway, roughness * roughness);

	vec3 specular = ( ndf *  fresnel * geometric )
					/ max(4 * normalDotLightDir * normalDotView, 0.00001);
	
	return specular;
}

vec3 createSampleVector(vec3 originVector, float maxThetaDeviationAngle, float maxPhiDeviationAngle, float randomTheta, float randomPhi)
{
	float phi = randomPhi * maxPhiDeviationAngle;
	float theta = randomTheta * maxThetaDeviationAngle;

	vec3 sampleVector;
	sampleVector.x = cos(phi) * sin(theta);
	sampleVector.y = sin(phi) * sin(theta);
	sampleVector.z = cos(theta);

	vec3 up = float(abs(originVector.z) < 0.999) *  vec3(0.0, 0.0, 1.0) + float(abs(originVector.z) >= 0.999) * vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, originVector));
	vec3 bitanget = cross(originVector, tangent);

	mat3 tbn = mat3(tangent, bitanget, originVector);

	return tbn *  normalize(sampleVector);
}

vec3 createLightSampleVector(vec3 originVector, float radius, float randomTheta, float randomPhi)
{
	float r = sqrt(randomTheta);

	float angle = M_PI * 2.0 * randomPhi;

	vec2 diskSample = vec2(r * cos(angle) * radius, r * sin(angle)) * radius;

	vec3 tangent = normalize(cross(originVector, vec3(0.0, 1.0, 0.0)));
	
	vec3 bitanget = normalize(cross(tangent, originVector));
	
	return vec3(originVector + diskSample.x * tangent + diskSample.y * bitanget); 
}

vec3 createLightSamplePoint(vec3 orientationVector, vec3 originPoint, float radius, float randomTheta, float randomPhi)
{
	float phi = randomPhi * 2 * M_PI;
	float theta = randomTheta * M_PI * 0.5;

	vec3 sampleVector;
	sampleVector.x = cos(phi) * sin(theta);
	sampleVector.y = sin(phi) * sin(theta);
	sampleVector.z = cos(theta);

	vec3 up = float(abs(orientationVector.z) < 0.999) *  vec3(0.0, 0.0, 1.0) + float(abs(orientationVector.z) >= 0.999) * vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, orientationVector));
	vec3 bitanget = cross(orientationVector, tangent);

	mat3 tbn = mat3(tangent, bitanget, orientationVector);

	return originPoint + (tbn *  normalize(sampleVector) * radius);
}

vec3 createCosineWeightedHemisphereSample(vec3 originVector, float randomTheta, float randomPhi)
{
	float x = sqrt(randomTheta)*cos(2*M_PI*randomPhi);
	float y = sqrt(randomTheta)*sin(2*M_PI*randomPhi);
	float z = sqrt(1.0 - randomTheta);

	vec3 up = float(abs(originVector.z) < 0.999) *  vec3(0.0, 0.0, 1.0) + float(abs(originVector.z) >= 0.999) * vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, originVector));
	vec3 bitanget = cross(originVector, tangent);

	mat3 tbn = mat3(tangent, bitanget, originVector);

	return normalize(tbn * vec3(x, y, z));
}

float ggxImportanceCDF(float x, float alpha)
{
	return atan((alpha * sqrt(x / sqrt(1.0 - x))));
}

float ggxImportancePDF(float theta, float alpha)
{
	float alpha2 = max(alpha * alpha, 0.0001);
	return (alpha2 * cos(theta) * sin(theta)) / (M_PI * pow(((alpha2 - 1) * pow(cos(theta), 2) + 1), 2));
}

float veachBalanceHeuristik(float pdf, float otherPDF)
{
	return pdf / (pdf + otherPDF);
}

float veachPowerHeurisitk(float pdf, float otherPDF, float power)
{
	return pow(pdf, power) / (pow(pdf, power) + pow(otherPDF, power));
}

float veachMaximumHeuristik(float pdf, float otherPDF)
{
	return float(pdf > otherPDF);
}
#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

//shader input
layout (location = 0) in vec2 texCoord;
//output write
layout (location = 0) out vec4 outColor;

layout (location = 2) out vec4 outPriorPosition;

layout(set = 0, binding = 0, rgba32f) uniform image2D radiance;

layout( push_constant ) uniform constants
{
	ivec2 nativeResolution;
} PushConstants;

void main()
{
	float exposure = 5.0;
	float gamma = 2.2;

	ivec2 pixCoord = ivec2((texCoord*PushConstants.nativeResolution));
   
    vec4 hdrColor = imageLoad(radiance, pixCoord);
    vec4 mapped = vec4(1.0) - exp(-hdrColor * exposure);
    mapped = pow(mapped, vec4(1.0 / gamma));

    hdrColor.w = 1.0;
	outColor = mapped;
}

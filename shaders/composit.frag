#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

//shader input
layout (location = 0) in vec2 texCoord;
//output write
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0, rgba32f) uniform image2D image;

void main()
{
	//return color
	
	ivec2 pixCoord = ivec2((texCoord*1024));
    vec4 color = imageLoad(image, pixCoord);
	outColor = color;
}

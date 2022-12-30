#version 460

//shader input
layout (location = 0) in vec2 texCoord;

//output write
layout (location = 0) out vec4 outColor;


layout(set = 0, binding = 0) uniform sampler2D albedo;
layout(set = 0, binding = 1) uniform sampler2D position;
layout(set = 0, binding = 2) uniform sampler2D normal;
layout(set = 0, binding = 3) uniform sampler2D ID;

void main()
{
	//return color
    vec3 color = texture(position, texCoord).xyz;
	outColor = vec4(color, 1.0f);
}


#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(set = 2, binding = 0) uniform accelerationStructureEXT topLevelAS;


struct hitPayload
{
  vec3 hitValue;
  int depth;
};

layout(location = 1) rayPayloadInEXT hitPayload incomigPayload;


hitAttributeEXT vec2 hitAttribute;

void main()
{
	incomigPayload.hitValue = vec3(1.0, 0.0, 0.0);
}
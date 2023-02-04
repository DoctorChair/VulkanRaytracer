#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(location = 1) rayPayloadEXT float shadowed;

void main()
{
  shadowed = 0.1;
}
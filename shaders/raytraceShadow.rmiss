#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require


layout(location = 0) rayPayloadInEXT bool shadowed;

void main()
{
  shadowed = false;
}
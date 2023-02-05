#version 460
#extension GL_EXT_ray_tracing : require

struct hitPayload
{
  vec3 hitValue;
};

layout(location = 1) rayPayloadInEXT hitPayload prd;

void main()
{
  
}
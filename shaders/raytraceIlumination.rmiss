#version 460
#extension GL_EXT_ray_tracing : require


struct hitPayload
{
  bool miss;
  vec3 radiance;
  uint depth;
};

layout(location = 1) rayPayloadInEXT hitPayload incomigPayload;

void main()
{
  incomigPayload.radiance = incomigPayload.radiance;
  incomigPayload.miss = true;
}
#version 460
#extension GL_EXT_ray_tracing : require

struct hitPayload
{
  bool miss;
  vec3 color;
  vec3 normal;
  vec3 position;
};

layout(location = 1) rayPayloadInEXT hitPayload incomigPayload;

void main()
{
  incomigPayload.color = vec3(0.0, 0.0, 0.0);
  incomigPayload.miss = true;
}
#version 460
#extension GL_EXT_ray_tracing : require

struct hitPayload
{
  bool shadowed;
  vec3 barycentrics;
  uint primitiveID;
  uint instanceID;
};

layout(location = 1) rayPayloadInEXT hitPayload incomigPayload;

void main()
{
  incomigPayload.shadowed = false;
  incomigPayload.barycentrics = vec3(0.0, 0.0, 0.0);
  incomigPayload.instanceID = 0;
  incomigPayload.primitiveID = 0;
}
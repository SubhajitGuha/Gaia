#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_nonuniform_qualifier : enable

#include "../random.glsl"

layout (location = 1) rayPayloadInEXT float shadowHitVal;

void main()
{
	shadowHitVal = 1.0;
}
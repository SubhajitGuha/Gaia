#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : require

#include "../ddgi/gi_commons.glsl"

layout(location = 0) in vec2 texCoord;

layout(set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
	vec3 viewDir;
	vec3 camPos;
} cameraBuffer;

layout(set = 0, binding = 1) buffer readonly transformLayout
{
    mat4 model[];
} transforms;

//GBuffers
layout(set = 1, binding = 0) uniform sampler2D gBufferDepth;
layout(set = 1, binding = 1) uniform sampler2D gBufferAlbedo;
layout(set = 1, binding = 2) uniform sampler2D gBufferMetallicRoughness;
layout(set = 1, binding = 3) uniform sampler2D gBufferNormal;


//DDGI probe data
layout(set = 2, binding = 0) uniform sampler2D irradianceImage;
layout(set = 2, binding = 1) uniform sampler2D depthImage;

layout(set = 3, binding = 0) uniform DdgiParametersLayout{
	DdgiParameters parameters;
}ddgi;

layout(location = 0) out vec4 color;

void main()
{
	mat4 view_proj = cameraBuffer.view * cameraBuffer.projection;
	//reconstruct position from depth;
	float depth = texture(gBufferDepth, texCoord).r;
	vec2 screenPosXY = texCoord * 2.0 - vec2(1.0);
	vec4 screenSpaceVertPos = vec4(screenPosXY, depth, 1.0);
	vec4 wsVertexPos = inverse(cameraBuffer.view) * inverse(cameraBuffer.projection) * screenSpaceVertPos;
	//undo projection
	wsVertexPos /= wsVertexPos.w;

	vec3 Wo = normalize(cameraBuffer.camPos - wsVertexPos.xyz);
	vec3 N = texture(gBufferNormal, texCoord).rgb;

	vec3 out_color = 1.0 * sample_irradiance(ddgi.parameters, wsVertexPos.xyz, N, Wo, irradianceImage, depthImage);

	color = vec4(
    out_color,
    1.0);
}
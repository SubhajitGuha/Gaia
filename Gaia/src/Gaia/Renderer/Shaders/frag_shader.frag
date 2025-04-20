#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : require

#include "ddgi/gi_commons.glsl"

//shader input
flat layout (location = 0) in uint materialId;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec4 vertexPosition;
layout(location = 3) in vec3 vertexNormal;
layout(location = 4) in vec3 vertexTangent;

#define MAX_CASCADES 8

struct Material
{
	vec4 baseColorFactor;
	float metallicFactor;
	float roughnessFactor;
	float normalStrength;

	int baseColorTexture;
	int metallicRoughnessTexture;
	int normalTexture;
};

struct LightMatrices
{
	mat4 lightView;
	mat4 lightProjection;
	float farRange;
	int numCascades;
};

layout(set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
	vec3 viewDir;
	vec3 camPos;
} cameraBuffer;

layout(set = 0, binding = 2) uniform LightParameters
{
	vec3 color;
	float intensity;
	vec3 direction;
} lightParameters;

layout(set = 1, binding = 0) readonly buffer materialLayout
{
	Material materials[];
} materialBuffer;

layout(set = 1, binding = 1) uniform sampler2D textures[];

layout(set = 2, binding = 0) uniform LightMatricesLayout
{
	LightMatrices lightMatrices[MAX_CASCADES];
} lightMatrixBuffer;

layout(set = 2, binding = 1) uniform sampler2D shdowTextures[];

//DDGI probe data
layout(set = 3, binding = 0) uniform sampler2D irradianceImage;
layout(set = 3, binding = 1) uniform sampler2D depthImage;

layout(set = 4, binding = 0) uniform DdgiParametersLayout{
	DdgiParameters parameters;
}ddgi;

//output write
layout (location = 0) out vec4 outFragColor;

const float PI = 3.14159265359;
const float ONE_BY_PI = 0.31830988618;
vec3 PBR_Color = vec3(0.0);
vec3 radiance;

vec3 ks;
vec3 kd;
vec3 F0;

float roughness = 0; //Roughness value
float metallic = 0;

vec3 ACESFilm(vec3 x)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return clamp((x*(a*x+b))/(x*(c*x+d)+e),0.0,1.0);
}

float NormalDistribution_GGX(float NdotH)
{
	float a2     = roughness*roughness;
    float NdotH2 = NdotH*NdotH;
	
    float nom    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = PI * denom * denom;
	
    return nom / denom;
}

float Geometry_GGX(float dp) //dp = Dot Product
{
	float k = pow(roughness+1,2) / 8.0;
	return dp/(dp * (1-k) + k);
}

vec3 Fresnel(float VdotH)
{	
	return F0 + (1.0 - F0) * pow(clamp(1.0 - VdotH, 0.0 ,1.0) , 5.0);
}

vec3 FresnelSchlickRoughness(float VdotH, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

vec3 SpecularBRDF(vec3 LightDir,vec3 ViewDir, vec3 Normal)
{
	vec3 Half = normalize( ViewDir + LightDir);
	float NdotH = max(dot(Normal,Half) , 0.0);
	float NdotV = max(dot(Normal,ViewDir) , 0.000001);
	float NdotL = max(dot(Normal,LightDir) , 0.000001);
	float VdotH = max(dot(ViewDir,Half) , 0.0);

	float Dggx = NormalDistribution_GGX(NdotH);
	float Gggx = Geometry_GGX(NdotV) * Geometry_GGX(NdotL);
	vec3 fresnel = Fresnel(VdotH);

	float denominator = 4.0 * NdotL * NdotV + 0.0001;
	vec3 specular = (Dggx * Gggx * fresnel) / denominator;
	return specular;
}

float modifiedFresnel(vec3 omega, float h_dot_l)
{
	float f_d90 = 0.5 + 2.0 * roughness * h_dot_l;
	float f_d = (1.0 + (f_d90 -1.0) * pow(1.0 - dot(vertexNormal,omega),5) );
	return f_d;
}
vec3 DiffuseBRDF(vec3 baseColor, vec3 view, vec3 light)
{
	vec3 h = normalize(view + light);
	float h_dot_l = dot(h,light);
	
	vec3 f_diff = baseColor * ONE_BY_PI * modifiedFresnel(view, h_dot_l) * modifiedFresnel(light, h_dot_l) * dot(vertexNormal, light);
	return f_diff;
}

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

float textureProject(vec2 offset, int cascadeIndex)
{
	mat4 lightProj = lightMatrixBuffer.lightMatrices[cascadeIndex].lightProjection;
	mat4 lightView = lightMatrixBuffer.lightMatrices[cascadeIndex].lightView;

	//convert to light space

	vec4 c = biasMat * lightProj * lightView * vec4(vertexPosition.xyz, 1.0); //directional light no model matrix is required
	c /= c.w;
	float shadow = 1.0;
	float bias = 0.001;

	if(c.z > -1.0 && c.z < 1.0)
	{
		float depth = texture(shdowTextures[cascadeIndex], c.xy).r;
		if (c.w > 0.0 && c.z - bias > depth)
			return 0.0;	
	}

	return shadow;
}

float calculateShadow()
{
	vec4 vs = cameraBuffer.view * vec4(vertexPosition.xyz, 1.0);

	float z = vs.z;

	int index = 0;
	int numCascades = lightMatrixBuffer.lightMatrices[0].numCascades;
	for (int i=0;i<numCascades - 1; i++)
	{
		if(z < lightMatrixBuffer.lightMatrices[i].farRange)
		{
			index = i+1;
		}
	}

	vec2 texDim = textureSize(shdowTextures[index], 0);
	float scale = 0.75;
	vec2 texelSize = scale * 1.0/texDim;

	float shadowFactor = 0.0;
	int count = 0;
	int blockSize = 1;

	for(int i=-blockSize; i<blockSize; i++)
	{
		for(int j=-blockSize;j<blockSize;j++)
		{
			shadowFactor += textureProject(vec2(i,j)*texelSize, index);
			count ++;
		}
	}

	return shadowFactor/count;
}

const float GI_INTENSITY = 1.0;
vec3 indirect_lighting(vec3 view_dir, vec3 N, vec3 P, vec3 kD, vec3 albedo)
{
	return GI_INTENSITY * sample_irradiance(ddgi.parameters, P, N, view_dir, irradianceImage, depthImage);
}

void main() 
{
	Material mat = materialBuffer.materials[materialId];
	vec4 clipSpace = cameraBuffer.projection * cameraBuffer.view * vertexPosition;
	clipSpace.xyz /= clipSpace.w;
	
	vec2 cs_texCoord = clipSpace.xy * 0.5 + vec2(0.5);
	vec4 albedo = mat.baseColorTexture!=-1? texture(textures[mat.baseColorTexture], texCoord): vec4(mat.baseColorFactor);
	if (albedo.a < 0.02)
	{
		discard;
	}
	vec4 matRoughness = mat.metallicRoughnessTexture!=-1?texture(textures[mat.metallicRoughnessTexture], texCoord):vec4(1.0);
	roughness = matRoughness.g * mat.roughnessFactor;
	metallic = matRoughness.b * mat.metallicFactor;

	vec3 DirectionalLight_Direction = normalize(-lightParameters.direction );//for directional light as it has no concept of position
	vec3 EyeDirection = normalize( cameraBuffer.camPos - vertexPosition.xyz);

	//diffuse_environment reflections
	vec3 Light_dir_i = reflect(-EyeDirection,vertexNormal);

	//F0 is 0.04 for non-metallic and albedo for metallics 
	F0 = vec3(0.04);
	F0 = mix(F0, albedo.xyz, metallic);

	float shadow = calculateShadow();

	float vdoth = max(dot(EyeDirection, normalize(DirectionalLight_Direction + EyeDirection)),0.0);
	ks = Fresnel(vdoth);
	kd = vec3(1.0) - ks;
	kd *= (1.0 - metallic);
	PBR_Color += ( (kd * DiffuseBRDF(albedo.rgb, EyeDirection, DirectionalLight_Direction)) + SpecularBRDF(DirectionalLight_Direction , EyeDirection , vertexNormal) ) * (shadow * lightParameters.color * lightParameters.intensity) * max(dot(vertexNormal,DirectionalLight_Direction), 0.001) ; //for directional light (no attenuation)
	
	bool front_facing = dot(vertexNormal, EyeDirection) >= 0.0;
	vec3 ffnormal = front_facing? vertexNormal : -vertexNormal;
	
	vec3 GI = indirect_lighting(EyeDirection, ffnormal, vertexPosition.xyz, kd, albedo.rgb);

	PBR_Color += GI.rgb;
	//PBR_Color = pow(PBR_Color, vec3(1.0/2.2));
	PBR_Color = ACESFilm(PBR_Color);
	outFragColor = vec4(GI.rgb,1.0f);
}
#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require

#include "../random.glsl"
#include "../brdf.glsl"
#include "gi_commons.glsl"

struct BufferAddress{
	uint64_t vertexBufferAddress;
	uint64_t indexBufferAddress;
};
struct Material{
	vec4 baseColorFactor;
	float metallicFactor;
	float roughnessFactor;
	float normalStrength;

	int baseColorTexture;
	int metallicRoughnessTexture;
	int normalTexture;
};
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(set = 1, binding = 0) buffer readonly geometryBufferAddressLayout{
	BufferAddress address[];
}bufferAddress;

layout(set = 2, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
	vec3 viewDir;
	vec3 camPos;
	uint frameNumber;
} cameraBuffer;
layout(set = 2, binding = 1) buffer readonly transformLayout
{
    mat4 model[];
} transforms;
layout(set = 2, binding = 2) uniform LightParameters
{
	vec3 color;
	float intensity;
	vec3 direction;
} lightParameters;

layout(set = 3, binding = 0) readonly buffer materialLayout
{
	Material materials[];
} material;

layout(set = 3, binding = 1) uniform sampler2D textures[];

layout(set = 5, binding = 0) uniform DdgiParametersLayout{
	DdgiParameters parameters;
}ddgi;

//probe data
layout(set = 6, binding = 0) uniform sampler2D irradianceImage;
layout(set = 6, binding = 1) uniform sampler2D depthImage;

layout(push_constant) uniform pushConstantLayout
{
	mat4 randomOrientation;
	int isFirstFrame;
}pushConstant;

const float SHADING_NORMAL_VIEW_ANGLE_THRESHOLD = 0.1;

struct Hit{
	vec3 color;
	vec3 incommingLight;
	uint payloadSeed;
	float rayDistance;
	vec3 rOrigin;
	vec3 rDirection;
};
layout (location = 0) rayPayloadInEXT Hit hitValue; //incoming payload
layout (location = 1) rayPayloadEXT float shadowHitVal; //shadow payload

hitAttributeEXT vec2 attribs;

struct Vertex{
	vec4 position;
	vec2 uv;
	vec3 normal;
	vec3 tangent;
	uint material_id;
	uint mesh_index;
};
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {uint i[]; };
layout(buffer_reference, scalar) buffer Data {vec4 f[]; };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//brdf calc
uint seed = 0;

float power_heuristic(float f, float g)
{
	 return (f * f) / (f * f + g * g);
}
vec3 sampleLight(Triangle hitTriangle, vec3 V, vec3 c_diffuse)
{
	float dirLightPDF = 1.0;//pdf for sampling a directional light is always 1.0;
	BSDFEvaluate bsdf = evaluate(hitTriangle, V, -normalize(lightParameters.direction), BSDF_FLAG_ALL);
	vec3 color = lightParameters.color * lightParameters.intensity * c_diffuse;
	{
		//color *= bsdf.f / dirLightPDF;
	}
	return color;
}

Frame get_shading_frame(vec3 normal)
{
	float sgn = normal.z >=0.0 ? 1.0:-1.0;
	float a = -1.0 / (sgn + normal.z);
	float b = normal.x * normal.y * a;
	vec3 s = vec3(1.0 + sgn * normal.x * normal.x * a, sgn*b, -sgn * normal.x);
	vec3 t = vec3(b, sgn + normal.y * normal.y * a, -normal.y);

	return Frame(s, t, normal);
}

ShadingMaterial shadingMaterial;
float alpha;
// This function will unpack our vertex buffer data into a single triangle and calculates uv coordinates
Triangle unpackTriangle(uint index) {
	Triangle tri;
	const uint triIndex = index * 3;

	BufferAddress address = bufferAddress.address[gl_InstanceID];

	Indices indices   = Indices(address.indexBufferAddress);
	Vertices vertices = Vertices(address.vertexBufferAddress);

	Vertex vert_attribs[3];
	for (uint i = 0; i < 3; i++) {
		const uint index = indices.i[triIndex + i];//14 4byte variables are there
		
		Vertex v = vertices.v[index];
		////apply the transformations
		mat4 modelMatrix = transforms.model[v.mesh_index];
		
		v.position = modelMatrix * v.position;
		v.normal = normalize(modelMatrix * vec4(v.normal, 0.0)).xyz;
		v.tangent = normalize(modelMatrix * vec4(v.tangent, 0.0)).xyz;

		vert_attribs[i] = v;
	}
	// Calculate values at barycentric coordinates
	vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	tri.uv = vert_attribs[0].uv * barycentricCoords.x + vert_attribs[1].uv * barycentricCoords.y + vert_attribs[2].uv * barycentricCoords.z;
	tri.normal = normalize(vert_attribs[0].normal * barycentricCoords.x + vert_attribs[1].normal * barycentricCoords.y + vert_attribs[2].normal * barycentricCoords.z);
	tri.position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;//vert_attribs[0].position.xyz * barycentricCoords.x + vert_attribs[1].position * barycentricCoords.y + vert_attribs[2].position * barycentricCoords.z;
	uint matId = vert_attribs[0].material_id;
	
	vec3 viewDir = -gl_WorldRayDirectionEXT;
	bool front_facing = dot(tri.normal, viewDir) >= 0.0;
	tri.front_facing = front_facing;
	vec3 ffnormal = front_facing? tri.normal : -tri.normal;
	
	Frame f = get_shading_frame(ffnormal);
	tri.frame = f;

	Material mat = material.materials[matId];
	vec4 color = mat.baseColorTexture!= -1 ? texture(textures[mat.baseColorTexture], tri.uv) : mat.baseColorFactor;
	vec3 textureNormal = mat.normalTexture != -1 ? texture(textures[mat.normalTexture], tri.uv).rgb : vec3(0.0);
	vec3 metallicRoughness = mat.metallicRoughnessTexture != -1 ? texture(textures[mat.metallicRoughnessTexture], tri.uv).rgb : vec3(1.0);
	float roughness =  metallicRoughness.g * mat.roughnessFactor;
	float metalness = metallicRoughness.b * mat.metallicFactor;

	shadingMaterial.base_color = color.rgb;
	alpha = color.a;
	shadingMaterial.metallic = metalness;
	shadingMaterial.roughness = roughness;

	tri.material = shadingMaterial;
	//tri.material.metallic = 0.0;
	//tri.material.roughness = 0.98;

	if(length(textureNormal) > 0.0)
	{
		textureNormal = textureNormal*2.0 - vec3(1.0);
		textureNormal *= vec3(mat.normalStrength, mat.normalStrength, 1.0);
		textureNormal = shading_local_to_world(tri.frame, textureNormal);
	
		// Move the texture normal towards geometric normal if the angle between
        // view direction and texture normals is too large.
        float sgn = dot(tri.frame.n, textureNormal) < 0.0? -1 : 1.0;
        textureNormal = sgn * textureNormal;
        float cos_theta = dot(viewDir, textureNormal);
        if (cos_theta <= SHADING_NORMAL_VIEW_ANGLE_THRESHOLD) {
            float t = clamp(cos_theta * (1.0 / SHADING_NORMAL_VIEW_ANGLE_THRESHOLD),0.0,1.0);
            textureNormal = sgn * normalize(mix(tri.frame.n, textureNormal, t));
        }
        tri.front_facing = dot(textureNormal, viewDir) > 0.0;
        textureNormal = tri.front_facing ? textureNormal : -textureNormal;
        tri.frame = get_shading_frame(textureNormal);
	}
	return tri;
}

vec3 fresnel_schlick_roughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

vec3 indirect_lighting(vec3 view_dir, vec3 N, vec3 P, vec3 F0, vec3 c_diffuse)
{
	//vec3 F = fresnel_schlick_roughness(max(dot(N, view_dir), 0.0), F0, shadingMaterial.roughness);
	//
	//vec3 kS = F;
	//vec3 kD = vec3(1.0) - kS;
	//kD *= (1.0 - shadingMaterial.metallic);
	GBufferData gBufferData = GBufferData(P, shadingMaterial.base_color, N, shadingMaterial.metallic, shadingMaterial.roughness);
	return diffuseBRDFForGI(view_dir, gBufferData) * sample_irradiance(ddgi.parameters, P, N, view_dir, irradianceImage, depthImage);
}

void main()
{
	seed = hitValue.payloadSeed;
	vec3 samples = vec3(rnd(seed), rnd(seed), rnd(seed));
	BSDFSample bsdf_info;
	Triangle tri = unpackTriangle(gl_PrimitiveID);
	vec3 view_dir = normalize(-gl_WorldRayDirectionEXT);
    const vec3 F0 = mix(vec3(0.04f), shadingMaterial.base_color, shadingMaterial.metallic);
	GBufferData gBufferData = GBufferData(tri.position, shadingMaterial.base_color, tri.frame.n, shadingMaterial.metallic, shadingMaterial.roughness);
	const vec3 c_diffuse = diffuseBRDFForGI(view_dir, gBufferData);
	//const vec3 c_diffuse = mix(shadingMaterial.base_color * (vec3(1.0f) - F0), vec3(0.0f), shadingMaterial.metallic);

	//bsdf_info = Sample(tri, view_dir, samples);

	//if(bsdf_info.pdf > 0.0)
		//hitValue.color *= bsdf_info.f/bsdf_info.pdf;
	
	shadowHitVal = 0.0;
	float tmin = 0.001;
	float tmax = 10000;
	vec3 origin = tri.position + tri.normal * 0.001;
	vec3 Ldir =  -normalize(lightParameters.direction);
	
	//hitValue.incommingLight += sampleLight(tri, view_dir, c_diffuse) * shadowHitVal;
	hitValue.incommingLight += lightParameters.color * lightParameters.intensity * shadingMaterial.base_color;
	if(alpha > 0.9) //dont do shadow testing with alpha geometry
	{
		traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsOpaqueEXT,
		0xff, 0,0,1, origin, tmin, Ldir, tmax, 1);
		hitValue.incommingLight *= shadowHitVal;
	}
	

	if(pushConstant.isFirstFrame == 0) //if not first frame
	{
		hitValue.incommingLight += indirect_lighting(view_dir, normalize(tri.frame.n), tri.position, F0, c_diffuse);
	}
	hitValue.rayDistance = gl_RayTminEXT + gl_HitTEXT;
	hitValue.rOrigin = origin;
	hitValue.rDirection = bsdf_info.direction;
}
#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require

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

#include "random.glsl"
#include "brdf.glsl"
const float SHADING_NORMAL_VIEW_ANGLE_THRESHOLD = 0.1;

struct Ray{
	vec3 origin;
	vec3 direction;
};

struct Hit{
	vec3 color;
	vec3 incommingLight;
	Ray ray;
	uint payloadSeed;
	bool terminate;
};
layout (location = 0) rayPayloadInEXT Hit hitValue; //incoming payload
layout (location = 1) rayPayloadEXT bool isShadow;
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
vec3 sampleLight(Triangle hitTriangle, vec3 V)
{
	float dirLightPDF = 1.0;//pdf for sampling a directional light is always 1.0;
	BSDFEvaluate bsdf = evaluate(hitTriangle, V, -normalize(lightParameters.direction), BSDF_FLAG_ALL);
	vec3 color = lightParameters.color * lightParameters.intensity;
	{
		color *= bsdf.f / dirLightPDF;
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
	vec3 color = mat.baseColorTexture!= -1 ? texture(textures[mat.baseColorTexture], tri.uv).rgb : mat.baseColorFactor.xyz;
	vec3 metallicRoughness = mat.metallicRoughnessTexture != -1 ? texture(textures[mat.metallicRoughnessTexture], tri.uv).rgb : vec3(1.0);
	vec3 textureNormal = mat.normalTexture != -1 ? texture(textures[mat.normalTexture], tri.uv).rgb : vec3(0.0);
	float roughness =  metallicRoughness.g * mat.roughnessFactor;
	float metalness = metallicRoughness.b * mat.metallicFactor;

	ShadingMaterial smat;
	smat.base_color = color;
	smat.metallic = metalness;
	smat.roughness = roughness;

	tri.material = smat;
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

const float OFFSET_RAY_INT_SCALE = 256.0;
const float OFFSET_RAY_FLOAT_SCALE = 1.0 / 65536.0;
const float OFFSET_RAY_ORIGIN = 1.0 / 32.0;

vec3 generate_offset_ray(vec3 pos, vec3 offset_normal) 
{
	ivec3 i_off = ivec3(offset_normal * OFFSET_RAY_INT_SCALE);
	ivec3 off_sgn = ivec3(pos.x < 0.0 ? -i_off.x : i_off.x,
						pos.y < 0.0 ? -i_off.y : i_off.y,
						pos.z < 0.0 ? -i_off.z : i_off.z);
	vec3 i_pos = intBitsToFloat(floatBitsToInt(pos) + off_sgn);

	vec3 f_off = offset_normal * OFFSET_RAY_FLOAT_SCALE;

	vec3 abs_pos = abs(pos);
	vec3 output_ = vec3(abs_pos.x < OFFSET_RAY_ORIGIN? pos.x + f_off.x : i_pos.x,
						abs_pos.y < OFFSET_RAY_ORIGIN? pos.y + f_off.y : i_pos.y,
						abs_pos.z < OFFSET_RAY_ORIGIN? pos.z + f_off.z : i_pos.z);
	return output_;
}

void main()
{
	seed = hitValue.payloadSeed;
	vec3 samples = vec3(rnd(seed), rnd(seed), rnd(seed));
	BSDFSample bsdf_info;
	Triangle tri = unpackTriangle(gl_PrimitiveID);
	vec3 view_dir = -normalize(gl_WorldRayDirectionEXT);
	
	bsdf_info = Sample(tri, view_dir, samples);

	if(bsdf_info.pdf > 0.0)
		hitValue.color *= bsdf_info.f/bsdf_info.pdf;
	else
		hitValue.terminate = true;

	
	vec3 offset_normal = dot(tri.normal,normalize(bsdf_info.direction)) >= 0.0 ? tri.normal : -tri.normal;
	hitValue.ray.origin = generate_offset_ray(tri.position, offset_normal);
	isShadow = true;
	float tmin = 0.001;
	float tmax = 10000;
	traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
	0xff, 0,0,1, hitValue.ray.origin, tmin, -normalize(lightParameters.direction), tmax, 1);
	if(!isShadow)
		hitValue.incommingLight += sampleLight(tri, view_dir) * hitValue.color;

	hitValue.ray.direction = normalize(bsdf_info.direction);
}
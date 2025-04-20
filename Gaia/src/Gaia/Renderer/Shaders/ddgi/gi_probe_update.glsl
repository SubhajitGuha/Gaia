
#define CACHE_SIZE 64
const uint RAYS_PER_PROBE = 256;

#if defined(DEPTH_PROBE_UPDATE)
	#define NUM_THREADS_X 16
    #define NUM_THREADS_Y 16
    #define TEXTURE_WIDTH ddgi.parameters.depthTextureWidth
    #define TEXTURE_HEIGHT ddgi.parameters.depthTextureHeight
    #define PROBE_SIDE_LENGTH ddgi.parameters.depthProbeSideLength
#else
	#define NUM_THREADS_X 8
    #define NUM_THREADS_Y 8
    #define TEXTURE_WIDTH ddgi.parameters.irradianceTextureWidth
    #define TEXTURE_HEIGHT ddgi.parameters.irradianceTextureHeight
    #define PROBE_SIDE_LENGTH ddgi.parameters.irradianceProbeSideLength
#endif

layout(local_size_x = NUM_THREADS_X, local_size_y = NUM_THREADS_Y, local_size_z = 1) in;

const float FLT_EPS = 0.00000001;

 //ray trace radiance data
 //stored as width = num rays per probe, height = num probes;
layout(set = 0, binding = 0, rgba16f) uniform image2D radianceImage;
layout(set = 0, binding = 1, rgba16f) uniform image2D directionDepthImage;

layout(set = 1, binding = 0, rgba16f) uniform image2D inputIrradianceImage;
layout(set = 1, binding = 1, rgba16f) uniform image2D inputDepthImage;

layout(set = 2, binding = 0) uniform DdgiParametersLayout{
	DdgiParameters parameters;
}ddgi;

layout(push_constant) uniform pushConstantLayout
{
	mat4 randomOrientation;
	int isFirstFrame;
}pushConstant;

//SHARED MEMORY---------------------------------------------------------------------
shared vec4 g_ray_direction_depth[CACHE_SIZE];
shared vec3 g_ray_hit_radiance[CACHE_SIZE];


void populate_cache(int relative_probe_id, uint offset, uint num_rays)
{
	if(gl_LocalInvocationIndex < num_rays)
	{
		ivec2 C = ivec2(offset + uint(gl_LocalInvocationIndex), relative_probe_id);
			g_ray_direction_depth[gl_LocalInvocationIndex] = imageLoad(directionDepthImage, C);
		#if !defined(DEPTH_PROBE_UPDATE)
			g_ray_hit_radiance[gl_LocalInvocationIndex] = imageLoad(radianceImage, C).rgb;
		#endif
	}
}

void gather_rays(ivec2 current_coord, uint num_rays, inout vec3 result, inout float total_weight)
{
	const float energy_conservation = 0.95f;

	//for each RAY
	for(int r = 0;r<num_rays; ++r)
	{
		vec4 ray_direction_depth = g_ray_direction_depth[r];
        vec3  ray_direction      = ray_direction_depth.xyz;

		#if defined(DEPTH_PROBE_UPDATE)
			float ray_probe_distance = min(ddgi.parameters.maxDistance, ray_direction_depth.w - 0.01f);
			
			//detect misses and force depth
			if(ray_probe_distance == -1.0)
				ray_probe_distance = ddgi.parameters.maxDistance;
		#else
			vec3  ray_hit_radiance   = g_ray_hit_radiance[r] * energy_conservation;
		#endif

		vec3 texel_dir = oct_decode(normalized_oct_coord(current_coord, PROBE_SIDE_LENGTH));

		float weight = 0.0f;

		#if defined(DEPTH_PROBE_UPDATE)
			weight = pow(max(0.0, dot(texel_dir, ray_direction)), ddgi.parameters.depthSharpness);
		#else
			weight = max(0.0, dot(texel_dir, ray_direction));
		#endif
		
		if(weight>= FLT_EPS)
		{
			#if defined(DEPTH_PROBE_UPDATE)
				result += vec3(ray_probe_distance * weight, square(ray_probe_distance) * weight, 0.0);
			#else
				result += vec3(ray_hit_radiance * weight);
			#endif

			total_weight += weight;
		}
	}
}

void main()
{
	const ivec2 current_coord = ivec2(gl_GlobalInvocationID.xy) + (ivec2(gl_WorkGroupID.xy) * ivec2(2)) + ivec2(2);
	const int relative_probe_id = probe_id(current_coord, TEXTURE_WIDTH, PROBE_SIDE_LENGTH);

	vec3 result  =  vec3(0.0);
	float total_weight = 0.0f;

	uint remaining_rays = RAYS_PER_PROBE;
	uint offset = 0;

	while(remaining_rays > 0)
	{
		uint num_rays = min(CACHE_SIZE, remaining_rays);

		populate_cache(relative_probe_id, offset, num_rays);

		barrier();

		gather_rays(current_coord, num_rays, result, total_weight);
		
		barrier();

		remaining_rays -= num_rays;
		offset += num_rays;
	}

	if(total_weight > FLT_EPS)
	{
		result /= total_weight;
	}

	//temporal accumulation
	vec3 prev_result;

	#if defined(DEPTH_PROBE_UPDATE)
		prev_result = imageLoad(inputDepthImage, current_coord).rgb;
	#else
		prev_result = imageLoad(inputIrradianceImage, current_coord).rgb;
	#endif

	if(pushConstant.isFirstFrame == 0)
		result = mix(result, prev_result, ddgi.parameters.hysteresis);

	#if defined(DEPTH_PROBE_UPDATE)
		imageStore(inputDepthImage, current_coord, vec4(result, 1.0));
	#else
		imageStore(inputIrradianceImage, current_coord, vec4(result, 1.0));
	#endif
}


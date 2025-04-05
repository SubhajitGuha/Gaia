const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;
const float ONE_BY_PI = 0.31830988618;
const float ONE_BY_TWO_PI = 0.15915494309;
const float ONE_BY_FOUR_PI = 0.079577471545948;
const float PI_BY_TWO = 1.57079632679;
const float PI_BY_FOUR = 0.78539816339;

const float transmission_wt = 0.0;

struct ShadingMaterial{
	vec3 base_color;
	float metallic;
	float roughness;
};
//these are modified values we get from the mesh
struct Frame{
	vec3 s;
	vec3 t;
	vec3 n;
};
struct Triangle {
	vec3 position;
	Frame frame;
	vec3 normal;
	vec2 uv;
	ShadingMaterial material;
    bool front_facing;
};

const uint BSDF_FLAG_DIFFUSE = 1<<0;
const uint BSDF_FLAG_GLOSSY = 1<<1;
const uint BSDF_FLAG_REFLECTION = 1<<2;
const uint BSDF_FLAG_SPECULAR = 1<<3;

const uint BSDF_FLAG_ALL = BSDF_FLAG_DIFFUSE | BSDF_FLAG_GLOSSY | BSDF_FLAG_SPECULAR | BSDF_FLAG_REFLECTION;

const uint BSDF_LOBE_DIFFUSE_REFLECTION = 0;
const uint BSDF_LOBE_SPECULAR_REFLECTION = 1;
const uint BSDF_LOBE_SPECULAR_TRANSMISSION = 2;
const uint BSDF_LOBE_COUNT = 3;

struct BSDFLobe
{
	uint lobe;
	float weight;
	float cum_weight;
};

struct BSDFSample
{
	vec3 f;
	float pdf;
	vec3 direction;
	uint flags;
};

struct BSDFEvaluate
{
	vec3 f;
	float pdf;
};

float luminance(vec3 color)
{
	return dot(vec3(0.3,0.6,0.1),color);
}

float[BSDF_LOBE_COUNT] calculate_lobe_weights(ShadingMaterial mat)
{
	float wt_dielectric = (1.0 - transmission_wt) * (1.0 - mat.metallic);
	float wt_metal = mat.metallic;

	float wt_diffuse = wt_dielectric;
	float wt_specular = wt_metal + wt_dielectric;
	float wt_transmission = (1.0 - mat.metallic) * transmission_wt;

	float norm_factor = 1.0 / (wt_diffuse + wt_specular + wt_transmission);

	wt_diffuse *= norm_factor;
	wt_specular *= norm_factor;
    wt_transmission *= norm_factor;

	float lobe_weights[BSDF_LOBE_COUNT];

	lobe_weights[BSDF_LOBE_SPECULAR_REFLECTION] = wt_specular;
    lobe_weights[BSDF_LOBE_SPECULAR_TRANSMISSION] = wt_transmission;
    lobe_weights[BSDF_LOBE_DIFFUSE_REFLECTION] = wt_diffuse;
    return lobe_weights;
}

BSDFLobe select_lobe(ShadingMaterial mat, float sample_)
{
	float lobe_weights[BSDF_LOBE_COUNT] = calculate_lobe_weights(mat);
	float wt_curr = lobe_weights[BSDF_LOBE_SPECULAR_TRANSMISSION];
	float wt_cum = wt_curr;
	if(sample_ < wt_cum)
	{
		return BSDFLobe(BSDF_LOBE_SPECULAR_TRANSMISSION, wt_curr, wt_cum);
	}
	wt_curr = lobe_weights[BSDF_LOBE_SPECULAR_REFLECTION];
    wt_cum += wt_curr;
    if (sample_ < wt_cum)
	{
        return BSDFLobe(BSDF_LOBE_SPECULAR_REFLECTION, wt_curr, wt_cum);
    }
	wt_curr = lobe_weights[BSDF_LOBE_DIFFUSE_REFLECTION];
    wt_cum += wt_curr;
    return BSDFLobe(BSDF_LOBE_DIFFUSE_REFLECTION, wt_curr, wt_cum);
}

// Returns true if both the directions are in the same hemisphere
bool is_same_hemisphere(vec3 w_o,vec3 w_i) {
    return w_o.z * w_i.z > 0.0;
}

vec2 sample_uniform_disk_concentric(vec2 uv) {
    vec2 uv_offset = 2.0 * uv - vec2(1.0, 1.0);
    if (uv_offset.x == 0.0 || uv_offset.y == 0.0) {
        return vec2(0.0, 0.0);
    }

    float r = abs(uv_offset.x)> abs(uv_offset.y) ? uv_offset.x: uv_offset.y;
    float theta  = abs(uv_offset.x) > abs(uv_offset.y) ? PI_BY_FOUR * (uv_offset.y / uv_offset.x) : PI_BY_TWO - PI_BY_FOUR * (uv_offset.x / uv_offset.y);

    return r * vec2(cos(theta), sin(theta));
}
vec3 cosine_sample_hemisphere(vec2 uv) {
    vec2  d = sample_uniform_disk_concentric(uv);
    float z = sqrt(1.0 - (d.x * d.x) - (d.y * d.y));
    if (z < 0.0) {
        z *= -1.0;
    }
    return vec3(d.x, d.y, z);
}

float fresnel_schlick(float cos_theta) {
    float m_cos_theta = clamp((1.0 - cos_theta), 0.0,1.0);
    float  m_cos_theta_sq = m_cos_theta * m_cos_theta;
    return m_cos_theta * m_cos_theta_sq * m_cos_theta_sq;
}

vec2 fresnel_dielectric_coefficent(float c_theta_i, float eta)
{
    float cos_theta_i = clamp(c_theta_i, -1.0 , 1.0);

    float sin_2_theta_t = (1.0 - cos_theta_i * cos_theta_i) * (eta * eta);
    // Total Internal Reflection
    if (sin_2_theta_t > 1.0) {
        return vec2(1.0);
    }
    float cos_theta_t = sqrt(max(1.0 - sin_2_theta_t, 0.0));

    float r_parl = (eta * cos_theta_t - cos_theta_i) / (eta * cos_theta_t + cos_theta_i);
    float r_perp = (eta * cos_theta_i - cos_theta_t) / (eta * cos_theta_i + cos_theta_t);
    return vec2(r_parl, r_perp);
}

float fresnel_dielectric(float c_theta_i,float  eta)
{
    vec2 coeff = fresnel_dielectric_coefficent(c_theta_i, eta);
    return 0.5 * dot(coeff, coeff);
}

vec3 fresnel_schlick_eval(vec3 f0,float f90, float v_dot_h)
{
    return f0 + (vec3(f90) - f0) * fresnel_schlick(v_dot_h);
}

vec3 fresnel_f82_tint(vec3 f0, vec3 f82_tint, float v_dot_h)
{
    float mu_bar = 1.0 / 7.0;
    float denom = mu_bar * pow(1.0 - mu_bar, 6.0);
    vec3 f_bar = fresnel_schlick_eval(f0, 1.0, mu_bar);
    vec3 f = fresnel_schlick_eval(f0, 1.0, v_dot_h);
    return f - v_dot_h * pow(1.0 - v_dot_h, 6.0) * (vec3(1.0) - f82_tint) * f_bar / denom;
}

vec3 diffuse_burley(ShadingMaterial mat, vec3 view, vec3 light, vec3 h) {
    float l_dot_h = dot(light, h);
    float l_dot_h_sq = l_dot_h * l_dot_h;
    float f_l = fresnel_schlick(light.z);
    float f_v = fresnel_schlick(view.z);
    float f_90= 0.5 + 2.0 * l_dot_h_sq * mat.roughness;
    float f_d = mix(1.0, f_90, f_l) * mix(1.0, f_90, f_v);

    // Sub-surface scattering
    float f_ss_90 = l_dot_h_sq * mat.roughness;
    float f_ss = mix(1.0, f_ss_90, f_l) * mix(1.0, f_ss_90, f_v);
    float ss = 1.25 * (f_ss * (1.0 / (abs(light.z) * abs(view.z)) - 0.5) + 0.5);

    // Fuzz (Disney Sheen)
     //float f_h = fresnel_schlick(l_dot_h);
     //vec3 sheen = mat.fuzz_weight * f_h * mat.fuzz_roughness * mat.fuzz_color;

	 float ss_weight = 0.0; //rn I dont support subsurface weight in my material system
    return mat.base_color.rgb * ONE_BY_PI * mix(f_d, ss, ss_weight);
}

BSDFEvaluate evaluate_diffuse(ShadingMaterial mat, vec3 view, vec3 light, vec3 h)
{
	if(!is_same_hemisphere(view, light))
	{
		return BSDFEvaluate(vec3(0.0), 0.0);
	}
	BSDFEvaluate eval;

	//cosine sample hemisphere pdf
	eval.pdf = abs(light.z) * ONE_BY_PI;
	eval.f = diffuse_burley(mat, view, light, h);
	return eval;
}

BSDFSample sample_diffuse(ShadingMaterial mat, vec3 view, vec2 sample_)
{
	vec3 light = cosine_sample_hemisphere(sample_);
	vec3 h = normalize(view + light);
	BSDFEvaluate eval = evaluate_diffuse(mat, view, light, h);
	
	return BSDFSample(eval.f, eval.pdf, light, BSDF_LOBE_DIFFUSE_REFLECTION);
}

// Generalized Trowbridge-Reitz (Gamma=2) (Smith-GGX) Normal Distribution Function
float ndf_gtr2_aniso(vec3 h, vec2 a) {
    vec2 inv_a = vec2(1.0) / a;
    float x = h.x * inv_a.x;
    float y = h.y * inv_a.y;
    float z = x * x + y * y + h.z * h.z;
    return ONE_BY_PI * inv_a.x * inv_a.y * h.z / (z * z);
}

// Smith-GGX Shadow-Masking Function (Anisotropic)
float shadow_mask_smithg_ggx_aniso(vec3 v, vec2 a) {
    float v_z2 = v.z * v.z;
    if (v_z2 == 0.0) {
        return 0.0;
    }
    float a_x = v.x * a.x;
    float a_y = v.y * a.y;
    float inv_a2 = (a_x * a_x + a_y * a_y) / v_z2;
    return 2.0 / (1.0 + sqrt(1.0 + inv_a2));
}

vec3 calculate_specular_f(ShadingMaterial mat, float v_dot_h)
{
	vec3 specular_color = vec3(1.0);
	float eta = 1.0;// two mediums are same for now
	vec3 spec_f = specular_color * fresnel_dielectric(v_dot_h, eta);
	vec3 metallic_f = fresnel_f82_tint(mat.base_color.rgb, specular_color, v_dot_h);
	return mix(spec_f, metallic_f, mat.metallic);
}

// Smith-GGX NDF Sampling of Visible Normals
vec3 sample_vndf_aniso(vec3 view, vec2 roughness,vec2 samples) {
    vec3 v = normalize(vec3(roughness.x * view.x, roughness.y * view.y, view.z));
    vec3 t1 =  v.z < 0.99999 ? normalize(cross(v, vec3(0.0, 0.0, 1.0))) :vec3(1.0, 0.0, 0.0);
    vec3 t2 = cross(t1, v);

    float a = 1.0 / (1.0 + v.z);
    float r = sqrt(samples.x);

    float phi = samples.y < a ? samples.y / a * PI : PI + (samples.y - a) / (1.0 - a) * PI;
    float sin_phi = sin(phi);
    float cos_phi = cos(phi);
    float p1 = r * cos_phi;
    float p2 = r * sin_phi * (samples.y < a? 1.0 : v.z);

    vec3 h = p1 * t1 + p2 * t2 + sqrt(max(0.0, 1.0 - p1 * p1 - p2 * p2)) * v;
    h *= vec3(roughness, 1.0);
    h.z = max(0.0, h.z);
    return normalize(h);
}

BSDFEvaluate evaluate_specular(ShadingMaterial mat, vec2 roughness,
                     vec3 view, vec3 light, vec3 h)
					 {
    BSDFEvaluate eval = BSDFEvaluate(vec3(0.0), 0.0);
    if (!is_same_hemisphere(view, light)) {
        return eval;
    }
    float v_dot_h = abs(dot(view, h));

    float g1 = shadow_mask_smithg_ggx_aniso(view, roughness);
    float g2 = shadow_mask_smithg_ggx_aniso(light, roughness);
    float d = ndf_gtr2_aniso(h, roughness);
    vec3 f = calculate_specular_f(mat, v_dot_h);

    eval.f = f * d * g1 * g2 / abs(4.0 * view.z * light.z);

    float jacobian = 1.0 / (4.0 * v_dot_h);
    eval.pdf = d * g1 * (v_dot_h / abs(view.z)) * jacobian;
    return eval;
}

BSDFSample sample_specular(ShadingMaterial mat, vec2 roughness, vec3 view, vec2 samples) {
    BSDFSample s = BSDFSample(vec3(0.0), 0.0, vec3(0.0), 0u);
    vec3 h = sample_vndf_aniso(view, roughness, samples);
    vec3 light = reflect(-view, h);
    BSDFEvaluate eval = evaluate_specular(mat, roughness, view, light, h);

    s.f = eval.f;
    s.pdf = eval.pdf;
    s.direction = light;
    s.flags = BSDF_FLAG_REFLECTION | BSDF_FLAG_GLOSSY;
    return s;
}

vec3 shading_world_to_local(Frame frame, vec3 dir)
{
	return normalize(vec3(dot(dir, frame.s), dot(dir, frame.t), dot(dir,frame.n)));
}

vec3 shading_local_to_world(Frame frame, vec3 dir)
{
	return normalize(dir.x * frame.s + dir.y * frame.t + dir.z * frame.n);
}

vec2 calculate_roughness(ShadingMaterial mat)
{
	bool is_thin = transmission_wt > 0.0;
	float alpha = mat.roughness;
	float aspect = 1.0; // sqrt(1.0 - mat.specular_roughness_anisotropy * 0.9);
	float a2 = alpha * alpha;
	vec2 aniso = vec2(max(0.001, a2 / aspect), max(0.001, a2 * aspect));
	return aniso;
}
BSDFSample Sample(Triangle hitTriangle, vec3 view, vec3 samples)
{
	BSDFSample s;
	s.f = vec3(0.0);
	s.pdf = 0.0;
	s.direction = vec3(0.0);
	s.flags = 0u;

	vec3 w_o = shading_world_to_local(hitTriangle.frame, view);
	if(w_o.z == 0.0)
	{
		return s;
	}
	vec2 roughness = calculate_roughness(hitTriangle.material);
	BSDFLobe sample_lobe = select_lobe(hitTriangle.material, samples.z);
	
	if (sample_lobe.lobe == BSDF_LOBE_DIFFUSE_REFLECTION)
	{
		 s = sample_diffuse(hitTriangle.material, w_o, samples.xy);
	}
	else if(sample_lobe.lobe == BSDF_LOBE_SPECULAR_REFLECTION)
	{
		s = sample_specular(hitTriangle.material, roughness, w_o, samples.xy);
	}

	s.f *= abs(s.direction.z);
	s.direction = shading_local_to_world(hitTriangle.frame, s.direction);
	return s;
}

const float SHADOW_TERMINATION_FACTOR = 0.04;
float shift_light_cos_theta(float n_dot_l) 
{
    // Appleseed renderer low-poly shadow termination
    float cos_t = min(n_dot_l, 1.0);
    float angle = acos(cos_t);
    float f = clamp(0.5 * SHADOW_TERMINATION_FACTOR, 0.0,1.0);
    float freq = 1.0 / (1.0 - f);
    float val = f == 0.0 ? 1.0 : max(cos(angle * freq), 0.0) / cos_t;
    return val;
}

float get_shadowing_factor(vec3 w_i, vec3 geo_n, vec3 shade_n) {
    // Taming the shadow terminator
    // https://www.yiningkarlli.com/projects/shadowterminator/shadow_terminator_v1_1.pdf
    float cos_s_i = dot(w_i, shade_n);
    vec3 n = cos_s_i < 0.0 ? -geo_n : geo_n;
    float g = min(dot(n, w_i) / cos_s_i * dot(n, shade_n), 1.0);

    if (g >= 1.0) {
        return 1.0;
    }
    if (g < 0.0) {
        return 0.0;
    }
    float g2 = g * g;
    return -g2 * g + g2 + g;
}

BSDFEvaluate evaluate(Triangle hitTriangle, vec3 view, vec3 light, uint sample_flags)
{
    BSDFEvaluate total_eval = BSDFEvaluate(vec3(0.0), 0.0);
    vec3 w_o = shading_world_to_local(hitTriangle.frame, view);
    vec3 w_i = shading_world_to_local(hitTriangle.frame, light);
    vec3 h = normalize(w_o + w_i);

    if (w_o.z == 0)
    {
        return total_eval;
    }

    vec2 roughness = calculate_roughness(hitTriangle.material);
    float[BSDF_LOBE_COUNT] lobe_weights = calculate_lobe_weights(hitTriangle.material);
	float wt_dielectric = (1.0 - hitTriangle.material.metallic) * (1.0 - transmission_wt);
	float wt_transmission = (1.0 - hitTriangle.material.metallic) * transmission_wt;

    if(lobe_weights[BSDF_LOBE_SPECULAR_REFLECTION] > 0.0)
    {
        BSDFEvaluate eval = evaluate_specular(hitTriangle.material, roughness, w_o, w_i, h);
        total_eval.f += eval.f;
        total_eval.pdf += lobe_weights[BSDF_LOBE_SPECULAR_REFLECTION] * eval.pdf;
    }
    if(lobe_weights[BSDF_LOBE_DIFFUSE_REFLECTION] > 0.0)
    {
        BSDFEvaluate eval = evaluate_diffuse(hitTriangle.material, w_o, w_i, h);
        total_eval.f += wt_dielectric * eval.f;
        total_eval.pdf += lobe_weights[BSDF_LOBE_DIFFUSE_REFLECTION] * eval.pdf;
    }

    float cos_theta = abs(w_i.z);
    total_eval.f *= shift_light_cos_theta(cos_theta) * cos_theta;

    vec3 geo_normal_ff = !hitTriangle.front_facing ? -hitTriangle.normal : hitTriangle.normal;
    total_eval.f *= get_shadowing_factor(light, geo_normal_ff, hitTriangle.frame.n);
    return total_eval;
}

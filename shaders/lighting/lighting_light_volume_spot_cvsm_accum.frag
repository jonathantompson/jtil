#version 150
#extension GL_EXT_texture_array : enable
#include "./shaders/lighting/lighting_include.frag"

uniform sampler2D f_depth_normal_view_stencil;
uniform sampler2D f_albedo_spec_intensity;
uniform sampler2D f_spec_power_vel;
uniform SpotLight f_light;
uniform float f_camera_far;

// Shadowmap specific uniforms
uniform mat4 f_vsm_split_pv_camviewinv[max_sm_count];
uniform float f_vsm_split_depths[max_sm_count + 1];
uniform int f_vsm_count;
uniform sampler2DArray f_vsm_array;
uniform isampler2D f_vsm_splits;
uniform float f_vsm_min_variance;
uniform vec2 f_light_near_far;
uniform float f_vsm_blend_zone;
uniform float f_lbr_amount;

out vec4 frag_color; 

// Per-pixel Shadow.
float PerPixelShadow(in vec3 pos, 
					 in float dist_to_light,
					 in int split) { 
  // Project using the associated matrix
  vec4 pos_light = f_vsm_split_pv_camviewinv[split] * vec4(pos, 1);
  vec2 light_tex_coord = (pos_light.xy / pos_light.w) * vec2(0.5, 0.5) + 
    vec2(0.5, 0.5);
  
  int split_safe = min(max(split, 0), f_vsm_count-1);
  vec2 moments = texture2DArray(f_vsm_array, vec3(light_tex_coord, 
    float(split_safe))).xy;
  
  float dist_to_light_scaled = linstep(-f_light_near_far.x, 
    -f_light_near_far.y, dist_to_light);
	
  float shadow_coeff = ChebyshevUpperBound(moments, dist_to_light_scaled, 
    f_vsm_min_variance, f_lbr_amount);

  // Now get the next split so we can blend...
  if(split < (f_vsm_count - 1)) {
    vec4 pos_light = f_vsm_split_pv_camviewinv[split_safe + 1] * vec4(pos, 1);
    vec2 light_tex_coord = (pos_light.xy / pos_light.w) * vec2(0.5, 0.5) + 
      vec2(0.5, 0.5);

	vec2 moments_next = texture2DArray(f_vsm_array, vec3(light_tex_coord, 
      float(split_safe + 1))).xy;

	float shadow_coeff_next = ChebyshevUpperBound(moments_next, 
	  dist_to_light_scaled, f_vsm_min_variance, f_lbr_amount);

    float s0 = f_vsm_split_depths[split_safe];
    float s1 = f_vsm_split_depths[split_safe + 1];
    float blend_zone_size = abs(s1 - s0) * f_vsm_blend_zone;
    float lerp_factor = abs(pos.z - s1) / blend_zone_size;
    lerp_factor = 1 - clamp(abs(lerp_factor), 0, 1); // Clamp between 0 and 1
    shadow_coeff = lerp(shadow_coeff, shadow_coeff_next, lerp_factor);
  }
  return shadow_coeff;
}

void main() {
  vec2 tex_coord = CalcTexCoord();

  vec3 pos, norm;
  float lighting_stencil;
  SampleGBufferPosNormStencil(tex_coord, f_depth_normal_view_stencil, pos, 
    norm, lighting_stencil);

  float diffuse_light_to_eye = 0;
  float specular_light_to_eye = 0;
  float attenuation = 0;
  float shadow_coeff = 1;

  if (pos.z > f_camera_far && lighting_stencil == 1.0f) {  
    // point is in front of the far plane and is stenciled for lighting
    // Calculate the lighting vectors
    vec3 light_vec = f_light.pos_view - pos;

    float dist_to_light = length(light_vec);  // Needed for SMAP depth compare
    // Note: "clamp(X, 0, 1)" in GLSL is equivilent to "saturate(x)" in HLSL
    attenuation = clamp(1.0f - (dist_to_light*dist_to_light)/
	  (f_light.near_far.y * f_light.near_far.y), 0.0, 1.0);

    if (attenuation > 0) {
	  // Look up the SM split (geometry independant)
      int split = texture(f_vsm_splits, tex_coord).x;
	  // Calculate shadow coefficient
      // vec4 posH = gCameraProj * vec4(pos, 1);
      shadow_coeff = PerPixelShadow(pos, dist_to_light, split);

      // dir_dot_light = cosine of the angle between the direction and the 
      // light vector
      float dir_dot_light = dot(f_light.dir_view, -normalize(light_vec)); 
      if(dir_dot_light > f_light.outer_angle_cosine) {
        vec3 albedo; 
        float spec_intensity, spec_power; 
        SampleGBufferAlbedoSpecular(tex_coord, f_albedo_spec_intensity, 
          f_spec_power_vel, albedo, spec_intensity, spec_power);

        light_vec = normalize(light_vec);

        // pos is in view space: from origin to point... So just negate and 
		// normalize
        vec3 to_eye = normalize(-pos);

        // Linear decay from (fov - decay angle) to (fov)
        float light_func = abs(dir_dot_light - f_light.outer_angle_cosine) / 
		  f_light.inner_minus_outer_cosine;
        float spot_intensity = clamp(light_func, 0.0, 1.0);
		spot_intensity *= spot_intensity;

        attenuation *= spot_intensity;

        // Perform the core phong lighting calculations
        diffuse_light_to_eye = max(0, dot(light_vec, norm));
        vec3 reflection_vec = normalize(reflect(-light_vec, norm));
        specular_light_to_eye  = f_light.spec_intensity * spec_intensity * 
          pow(clamp(dot(reflection_vec, to_eye), 0.0, 1.0), spec_power);
      }
    } 
  }
  // Combine the phong results with light properties
  // Diffuse + Specular
  frag_color = vec4(f_light.diffuse_intensity * attenuation * 
    diffuse_light_to_eye * f_light.diffuse_color, 
	attenuation * specular_light_to_eye);
  frag_color *= lighting_stencil * shadow_coeff;
}

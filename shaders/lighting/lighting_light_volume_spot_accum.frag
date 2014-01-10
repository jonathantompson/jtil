#version 150
#include "./shaders/lighting/lighting_include.frag"

uniform sampler2D f_depth_normal_view_stencil;
uniform sampler2D f_albedo_spec_intensity;
uniform sampler2D f_spec_power_vel;
uniform SpotLight f_light;
uniform float f_camera_far;

out vec4 frag_color; 

void main() {
  vec2 tex_coord = CalcTexCoord();

  vec3 pos, norm;
  float lighting_stencil;
  SampleGBufferPosNormStencil(tex_coord, f_depth_normal_view_stencil, pos, norm, 
    lighting_stencil);

  float diffuse_light_to_eye = 0;
  float specular_light_to_eye = 0;
  float attenuation = 0;

  if (pos.z > f_camera_far && lighting_stencil == 1.0f) {  
    // point is in front of the far plane and is stenciled for lighting
    // Calculate the lighting vectors
    vec3 light_vec = f_light.pos_view - pos;

    float dist_to_light = length(light_vec);  // Needed for SMAP depth comparison
    // Note: "clamp(X, 0, 1)" in GLSL is equivilent to "saturate(x)" in HLSL
    attenuation = clamp(1.0f - (dist_to_light*dist_to_light)/
	  (f_light.near_far.y * f_light.near_far.y), 0.0, 1.0);

    if (attenuation > 0) {
      // dir_dot_light = cosine of the angle between the direction and the 
      // light vector
      float dir_dot_light = dot(f_light.dir_view, -normalize(light_vec)); 
      if(dir_dot_light > f_light.outer_angle_cosine) {
        vec3 albedo; 
        float spec_intensity, spec_power; 
        SampleGBufferAlbedoSpecular(tex_coord, f_albedo_spec_intensity, 
          f_spec_power_vel, albedo, spec_intensity, spec_power);

        light_vec = normalize(light_vec);

        // pos is in view space: from origin to point... So just negate and normalize
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
  frag_color *= lighting_stencil;
}

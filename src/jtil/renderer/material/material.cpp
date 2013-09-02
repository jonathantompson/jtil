#include "jtil/renderer/material/material.h"
#include "jtil/renderer/shader/shader_program.h"
#include "jtil/renderer/gl_include.h"
#include "jtil/settings/settings_manager.h"

namespace jtil {
namespace renderer {
  void Material::setUniforms() const {
    if (QUERY_UNIFORM("f_spec_power")) {
      BIND_UNIFORM("f_spec_power", &spec_power);
    }
    if (QUERY_UNIFORM("f_spec_intensity")) {
      BIND_UNIFORM("f_spec_intensity", &spec_intensity);
    }
    if (QUERY_UNIFORM("f_const_albedo")) {
      BIND_UNIFORM("f_const_albedo", albedo.m);
    }
    if (QUERY_UNIFORM("te_disp_factor")) {
      BIND_UNIFORM("te_disp_factor", &displacement_factor);
    }
  }

  Material::Material() : spec_power(32), spec_intensity(1), albedo(black) { 
    GET_SETTING("tess_default_disp_factor", float, displacement_factor);
  }
}  // namespace renderer
}  // namespace jtil

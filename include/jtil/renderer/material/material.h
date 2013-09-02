//
//  material.h
//
//  Created by Jonathan Tompson on 12/15/12.
//
//  Very simple material type.  Just blinn-phong shading specular parameters.
//

#pragma once

#include "jtil/math/math_types.h"
#include "jtil/renderer/colors/colors.h"

#define MATERIAL_FILE_SIZE_BYTES (4 + 4 + 4*3)

namespace jtil {
namespace renderer {
  struct Material {
    float spec_power;
    float spec_intensity;
    float displacement_factor;
    math::Float3 albedo;
    Material();
    void setUniforms() const;
  };
};  // namespace renderer
};  // namespace jtil

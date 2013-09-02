//
//  light_dir.h
//
//  Created by Jonathan Tompson on 12/25/12.
//

#pragma once

#include "jtil/renderer/lights/light.h"
#include "jtil/math/math_types.h"

namespace jtil {
namespace renderer {
  class ShaderProgram;

  struct LightDirData {
    math::Float3 diffuse_color;
    math::Float3 dir_view;
    float diffuse_intensity;
    float spec_intensity;
    LightDirData();
  };

  class LightDir : public Light {
  public:
    LightDir();
    ~LightDir();

    inline virtual LightType type() const { return LIGHT_DIR; }

    // Getters and setters
    inline math::Float3& diffuse_color() { return light_data_.diffuse_color; }
    inline float& diffuse_intensity() { return light_data_.diffuse_intensity; }
    inline float& spec_intensity() { return light_data_.spec_intensity; }
    inline math::Float3& dir_world() { return dir_world_; }

    virtual void update();

    // Set the necessary handles in the shader programs
    virtual void setHandles() const;

  protected:
    LightDirData light_data_;
    math::Float3 dir_world_;

    // Non-copyable, non-assignable.
    LightDir(LightDir&);
    LightDir& operator=(const LightDir&);
  };

};  // namespace renderer
};  // namespace jtil

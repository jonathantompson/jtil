//
//  light_point.h
//
//  Created by Jonathan Tompson on 6/15/12.
//

#pragma once

#define LIGHT_POINT_MODEL_INSIDE_RADIUS 1.0f
#define LIGHT_POINT_MODEL_SLICES 11
#define LIGHT_POINT_MODEL_STACKS 11

#include "jtil/renderer/lights/light.h"
#include "jtil/math/math_types.h"

namespace jtil {
namespace renderer {
  class ShaderProgram;
  class Camera;

  struct LightPointData {
    math::Float3 diffuse_color;
    math::Float3 pos_view;
    float diffuse_intensity;
    math::Float2 near_far;
    float spec_intensity;
    LightPointData();
  };

  class LightPoint : public Light {
  public:
    LightPoint();
    ~LightPoint();

    inline virtual LightType type() const { return LIGHT_POINT; }
    virtual bool cameraInside(const Camera* camera) const;

    // Getters and setters
    inline math::Float3& pos_world() { return pos_world_; }
    inline math::Float3& pos_view() { return light_data_.pos_view; }
    inline float outside_rad() const { return outside_rad_; }
    inline math::Float4x4& mat_world() { return mat_world_; }
    inline math::Float3& diffuse_color() { return light_data_.diffuse_color; }
    inline const math::Float3& diffuse_color() const { return light_data_.diffuse_color; }
    inline float& diffuse_intensity() { return light_data_.diffuse_intensity; }
    inline math::Float2& near_far() { return light_data_.near_far; }
    inline float& spec_intensity() { return light_data_.spec_intensity; }

    inline const math::Float4x4& pvw_mat() const { return pvw_mat_; }
    inline const math::Float4x4& pvm_prev_frame() const { return pvm_prev_frame_; }
    inline const math::Float4x4& vw_mat() const { return vw_mat_; }
    inline const math::Float4x4& normal_mat() const { return normal_mat_; }

    virtual void update();
    virtual void updatePVW();

    void scalePVWMat(const float scale);

    // Set the necessary handles in the shader programs
    virtual void setHandles() const;

  protected:
    math::Float3 pos_world_;
    LightPointData light_data_;
    math::Float4x4 mat_world_;
    float outside_rad_;
    math::Float4x4 vw_mat_;
    math::Float4x4 pvw_mat_;
    math::Float4x4 pvm_prev_frame_;
    math::Float4x4 normal_mat_;
  
    static float outside_model_rad_scale_;

    void calcMatWorld(math::Float4x4& ret, const float model_radius, 
      const float desired_radius) const;

    // Non-copyable, non-assignable.
    LightPoint(LightPoint&);
    LightPoint& operator=(const LightPoint&);
  };

};  // namespace renderer
};  // namespace jtil

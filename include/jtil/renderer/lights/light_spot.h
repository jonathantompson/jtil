//
//  light_spot.h
//
//  Created by Jonathan Tompson on 6/15/12.
//

#pragma once

#define LIGHT_SPOT_MODEL_INSIDE_RADIUS 1.0f
#define LIGHT_SPOT_MODEL_HEIGHT 1.0f
#define LIGHT_SPOT_MODEL_SLICES 11

#include "jtil/renderer/lights/light.h"
#include "jtil/math/math_types.h"

namespace jtil {
namespace renderer {

  struct LightSpotData {
    math::Float3 diffuse_color;
    math::Float3 pos_view;
    math::Float3 dir_view;
    float outer_angle_cosine;
    float inner_minus_outer_cosine;
    float diffuse_intensity;
    math::Float2 near_far;
    float spec_intensity;
    LightSpotData();
    LightSpotData& operator=(const LightSpotData &rhs);
  };

  class LightSpot : public Light {
  public:
    LightSpot();
    ~LightSpot();

    inline virtual LightType type() const { return LIGHT_SPOT; }
    virtual bool cameraInside(const Camera* camera) const;

    // Getters and setters
    inline math::Float3& pos_world() { return pos_world_; }
    inline math::Float3& pos_view() { return light_data_.pos_view; }
    inline math::Float3& cone_center_world() { return cone_center_world_; }
    inline float cone_outside_radius() const { return cone_outside_radius_; }
    inline math::Float3& dir_world() { return dir_world_; }
    inline math::Float4x4& mat_world() { return mat_world_; }
    inline math::Float3& diffuse_color() { return light_data_.diffuse_color; }
    inline const math::Float3& diffuse_color() const { return light_data_.diffuse_color; }
    inline float& diffuse_intensity() { return light_data_.diffuse_intensity; }
    inline math::Float2& near_far() { return light_data_.near_far; }
    inline float& spec_intensity() { return light_data_.spec_intensity; }
    inline float& outer_fov_deg() { return outer_fov_deg_; }
    inline float& inner_fov_deg() { return inner_fov_deg_; }

    inline const math::Float4x4& pvw_mat() const { return pvw_mat_; }
    inline const math::Float4x4& pvm_prev_frame() const { return pvm_prev_frame_; }
    inline const math::Float4x4& vw_mat() const { return vw_mat_; }
    inline const math::Float4x4& normal_mat() const { return normal_mat_; }

    // View matrix from the light's perspective (in world coords)
    void calcViewMat(math::Float4x4* view);  

    virtual void update();
    virtual void updatePVW();

    void scalePVWMat(const float scale);

    // Set the necessary handles in the shader programs
    virtual void setHandles() const;

  protected:
    math::Float3 pos_world_;
    math::Float3 dir_world_;
    math::Float3 cone_center_world_;  // Center of illuminated cone (far end)
    float cone_outside_radius_;
    LightSpotData light_data_;
    math::Float4x4 mat_world_;  // For light volume optimizations
    float outer_fov_deg_;
    float inner_fov_deg_;
    math::Float4x4 vw_mat_;
    math::Float4x4 pvw_mat_;
    math::Float4x4 pvm_prev_frame_;
    math::Float4x4 normal_mat_;

    float static outside_model_rad_scale_;

    void calcMatWorld(math::Float4x4& ret);

    // Non-copyable, non-assignable.
    LightSpot(LightSpot&);
    LightSpot& operator=(const LightSpot&);
  };

};  // namespace renderer
};  // namespace jtil

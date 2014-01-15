//
//  camera.h
//
//  Created by Jonathan Tompson on 6/3/12.
//

#pragma once

#include "jtil/math/math_types.h"

namespace jtil {
namespace renderer {
  class Renderer;
  class Frustum;

  class Camera {
  public:
    Camera(const math::FloatQuat& eye_rot, const math::Float3& eye_pos_world, 
      const int screen_width, const int screen_height, 
      const float fov_deg, const float near, const float far);
    ~Camera();

    void updateView();
    void updateProjection();
    void rotateCamera(const float theta_x, const float theta_y);
    void moveCamera(const math::Float3& dir_eye_space);
    void resize(const uint32_t screen_width, const uint32_t screen_height);

    // getter / setter methods
    // Two set camera position and orientation, etc use these methods:
    inline math::Float3& eye_pos_world() { return eye_pos_world_; }
    inline const math::Float3& eye_pos_world() const { return eye_pos_world_; }
    inline math::FloatQuat& eye_rot() { return eye_rot_; }
    inline math::Float2& near_far() { return near_far_; }
    inline const math::Float2& near_far() const { return near_far_; }
    inline math::Float2& screen_size() { return screen_size_; }
    inline float& fov_deg() { return fov_deg_; }

    // These are mostly for internal usage only
    inline const math::Float4x4& view() { return view_; }
    inline const math::Float4x4& view_inv() { return view_inv_; }
    inline const math::Float4x4& proj() { return proj_; }
    inline const math::Float4x4& proj_view() { return proj_view_; }
    inline math::Float2& inv_focal_length() { return inv_focal_length_; }
    inline const Frustum* frustum() const { return frustum_; }

    void calcDirWorld(math::Float3& dir_world) const;

  private:
    math::FloatQuat eye_rot_;
    math::FloatQuat eye_rot_inv_;
    math::Float3 eye_pos_world_;
    float x_axis_rot_;
    float y_axis_rot_;
    math::Float2 near_far_;
    math::Float2 screen_size_;
    float fov_deg_;
    math::Float2 inv_focal_length_;
    Frustum* frustum_;

    math::Float4x4 view_;
    math::Float4x4 view_prev_frame_;
    math::Float4x4 proj_;
    math::Float4x4 proj_prev_frame_;  // Used for motion blur    
    math::Float4x4 proj_view_;
    math::Float4x4 view_inv_;

    // Non-copyable, non-assignable.
    Camera(Camera&);
    Camera& operator=(const Camera&);
  };

};  // namespace renderer
};  // namespace jtil

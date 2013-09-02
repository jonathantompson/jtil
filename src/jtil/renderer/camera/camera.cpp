#include "jtil/renderer/camera/camera.h"
#include "jtil/renderer/camera/frustum.h"
#include "jtil/renderer/gl_include.h"
#include "jtil/math/math_types.h"
#include "jtil/data_str/vector_managed.h"

namespace jtil {

using math::Float3;
using math::Float4x4;
using math::FloatQuat;

namespace renderer {

  Camera::Camera(const math::FloatQuat& eye_rot, 
    const math::Float3& eye_pos_world, const int screen_width, 
    const int screen_height, const float fov_deg, const float near, 
    const float far) {
    proj_.identity();
    eye_rot_.set(eye_rot);
    math::FloatQuat::inverse(eye_rot_inv_, eye_rot_);
    eye_pos_world_.set(eye_pos_world);
    x_axis_rot_ = 0;
    y_axis_rot_ = 0;
    
    screen_size_.set(static_cast<float>(screen_width),
                     static_cast<float>(screen_height));
    fov_deg_ = fov_deg;
    near_far_[0] = near;
    near_far_[1] = far;
    frustum_ = new Frustum();
  }

  Camera::~Camera() {
    if (frustum_) {
      delete frustum_;
      frustum_ = NULL;
    }
  }

  void Camera::updateView() {
    // Before updating view, record the old value
    view_prev_frame_.set(view_);
    
    // Find the inverse rotation using quaternion
    math::FloatQuat::inverse(eye_rot_inv_, eye_rot_);  // Very fast
    math::FloatQuat::quat2Mat4x4(view_, eye_rot_inv_);
    
    view_[2] *= -1.0f;
    view_[6] *= -1.0f;
    view_[10] *= -1.0f;

    view_.rightMultTranslation(-eye_pos_world_[0], -eye_pos_world_[1], 
      -eye_pos_world_[2]);
    Float4x4::affineRotationTranslationInverse(view_inv_, view_);

    // inv_focal_length is used for calculating screen space pos from depth
    // http://www.horde3d.org/forums/viewtopic.php?f=1&t=569
    float fov_rad = fov_deg_ * (float)PI_OVER_180;
    inv_focal_length_[1] = tanf(fov_rad * 0.5f);
    inv_focal_length_[0] = inv_focal_length_[1] * screen_size_[0] / 
      screen_size_[1];

  }
  
  void Camera::resize(const uint32_t screen_width,
    const uint32_t screen_height) {
    screen_size_.set(static_cast<float>(screen_width), 
      static_cast<float>(screen_height));
  }

  void Camera::updateProjection() {
    // Before updating projection, record the old value
    proj_prev_frame_.set(proj_);

    // Recall: OpenGL convention is to look down the negative Z axis,
    //         therefore, more negative values are actually further away.
    proj_.glProjection(-near_far_[0], -near_far_[1], fov_deg_,
      screen_size_[0], screen_size_[1]);

    // Perform frustum setup with the view projection matrix:
    Float4x4::multSIMD(proj_view_, proj_, view_ );
    frustum_->Set(proj_view_.m);
  }
  
  void Camera::rotateCamera(float theta_x, float theta_y) {
    y_axis_rot_ += theta_x;
    // Keep between -pi and +pi (allow wrap around rotations)
    y_axis_rot_ = (y_axis_rot_ > static_cast<float>(M_PI)) ?
      y_axis_rot_ - 2.0f*static_cast<float>(M_PI) : y_axis_rot_;
    y_axis_rot_ = (y_axis_rot_ < -static_cast<float>(M_PI)) ?
      y_axis_rot_ + 2.0f*static_cast<float>(M_PI) : y_axis_rot_;
    // Clamp between -pi_2 and +pi_2
    x_axis_rot_ += theta_y;
    x_axis_rot_ = (x_axis_rot_ > static_cast<float>(M_PI_2)) ?
      static_cast<float>(M_PI_2) : x_axis_rot_;
    x_axis_rot_ = (x_axis_rot_ < -static_cast<float>(M_PI_2)) ?
      -static_cast<float>(M_PI_2) : x_axis_rot_;

    // Rotate by y-axis first
    eye_rot_.identity();
    math::FloatQuat rot_mouse;
    rot_mouse.yAxisRotation(y_axis_rot_);
    math::FloatQuat rot_tmp;
    math::FloatQuat::mult(rot_tmp, eye_rot_, rot_mouse);
    // Now rotate by x-axis
    rot_mouse.xAxisRotation(x_axis_rot_);
    math::FloatQuat::mult(eye_rot_, rot_tmp, rot_mouse);
  }
  
  void Camera::moveCamera(const Float3& dir_eye_space) {
    // First calcuate the direction in world coords
    Float3 dir_world_coords;
    Float3::affineTransformVec(dir_world_coords, view_inv_, 
      dir_eye_space);
    eye_pos_world_.accum(dir_world_coords.m);
  }

  void Camera::calcDirWorld(math::Float3& dir_world) const {
    const Float3 forward(0, 0, 1);
    FloatQuat::mult(dir_world, eye_rot_inv_, forward);
  }

}  // namespace renderer
}  // namespace jtil

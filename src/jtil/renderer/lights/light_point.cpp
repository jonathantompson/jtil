#include <iostream>
#include "jtil/renderer/lights/light_point.h"
#include "jtil/renderer/renderer.h"
#include "jtil/renderer/camera/camera.h"
#include "jtil/renderer/geometry/geometry.h"
#include "jtil/renderer/geometry/geometry_manager.h"
#include "jtil/renderer/shader/shader_program.h"
#include "jtil/data_str/hash_funcs.h"
#include "jtil/renderer/objects/aabbox.h"

namespace jtil {

using math::Float4x4;
using math::Float3;

namespace renderer {

  LightPointData::LightPointData() {
    diffuse_color.set(1, 1, 1);
    diffuse_intensity = 1.0f;
    near_far.set(0.1f, 10.0f);
    spec_intensity = 0.2f;
  }

  float LightPoint::outside_model_rad_scale_ = 0;

  LightPoint::LightPoint() : Light()  {
    pos_world_.set(0, 0, 0);

    if (outside_model_rad_scale_ == 0) {
      float out_rad = GeometryManager::calcSphereOutsideRadius(
        LIGHT_POINT_MODEL_STACKS, LIGHT_POINT_MODEL_SLICES,
        LIGHT_POINT_MODEL_INSIDE_RADIUS);
      outside_model_rad_scale_ = out_rad / LIGHT_POINT_MODEL_INSIDE_RADIUS;
    }

    float out_rad = outside_model_rad_scale_ * LIGHT_POINT_MODEL_INSIDE_RADIUS;
    Float3 min_bound(-out_rad, -out_rad, -out_rad);
    Float3 max_bound(out_rad, out_rad, out_rad);
    aabbox_ = new objects::AABBox();
    aabbox_->init(min_bound, max_bound);

    update();
  }

  LightPoint::~LightPoint() {
    if (aabbox_) {
      delete aabbox_;
      aabbox_ = NULL;
    }
  }

  void LightPoint::update() {
    // Update the world view matrix for the sphere geometry
    float desired_radius = light_data_.near_far[1];
    calcMatWorld(mat_world_, LIGHT_POINT_MODEL_INSIDE_RADIUS, desired_radius);

    // Update the light position in view space:
    Float3::affineTransformPos(light_data_.pos_view, 
      Renderer::g_renderer()->camera()->view(), pos_world_);
    
    outside_rad_ = desired_radius * outside_model_rad_scale_;

    aabbox_->update(mat_world_);
  }

  void LightPoint::updatePVW() {
    pvm_prev_frame_.set(pvw_mat_);

    Float4x4::multSIMD(vw_mat_, Renderer::g_renderer()->camera()->view(), 
      mat_world_);

    // Calculate the model view normal matrix and bind it to the shader
    // Normal matrix is the (M_modelview^-1)^T:
    // --> http://www.songho.ca/opengl/gl_transform.html
    Float4x4::affineInverse(normal_mat_, vw_mat_);
    normal_mat_.transpose();

    Float4x4::multSIMD(pvw_mat_, Renderer::g_renderer()->camera()->proj(), 
      vw_mat_ );
  }

  bool LightPoint::cameraInside(const Camera* camera) const {
    // This is just a test for intersection with the near clip plane --> It's
    // only approximate and conservative.  If inside we will turn off depth
    // culling.
    if ((light_data_.pos_view[2] +
      light_data_.near_far[1] * outside_model_rad_scale_ + LOOSE_EPSILON) < 
      camera->near_far()[0]) {
      return false;
    } else {
      return true;
    }
  }

  void LightPoint::scalePVWMat(const float scale) {
    vw_mat_.rightMultScale(scale, scale, scale);
    normal_mat_.rightMultScale(1.0f/scale, 1.0f/scale, 1.0f/scale);
    pvw_mat_.rightMultScale(scale, scale, scale);
    pvm_prev_frame_.rightMultScale(scale, scale, scale);
  }

  void LightPoint::calcMatWorld(Float4x4& ret, const float model_radius, 
    const float desired_radius) const {
    // Before updating world matrix for this frame, save it for next frame 
    // (to generate velocity data later on)
    ret.identity();

    float scale =  desired_radius / model_radius;
    ret.m[0] = scale;  // (0,0)
    ret.m[5] = scale;  // (1,1)
    ret.m[10] = scale;  // (2,2);

    // Now offset by the light's postion
#ifdef COLUMN_MAJOR
    ret.m[12] = pos_world_[0];
    ret.m[13] = pos_world_[1];
    ret.m[14] = pos_world_[2];
#else
    ret.m[3] = pos_world_[0];
    ret.m[7] = pos_world_[1];
    ret.m[11] = pos_world_[2];
#endif
  }

  void LightPoint::setHandles() const {
    BIND_UNIFORM("f_light.diffuse_color", light_data_.diffuse_color.m);
    BIND_UNIFORM("f_light.pos_view", light_data_.pos_view.m);
    BIND_UNIFORM("f_light.diffuse_intensity", 
      &light_data_.diffuse_intensity);
    BIND_UNIFORM("f_light.near_far", light_data_.near_far.m);
    BIND_UNIFORM("f_light.spec_intensity", &light_data_.spec_intensity);
  }

}  // namespace renderer
}  // namespace jtil

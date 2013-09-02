#include "jtil/renderer/lights/light_spot.h"
#include "jtil/renderer/renderer.h"
#include "jtil/renderer/camera/camera.h"
#include "jtil/renderer/geometry/geometry.h"
#include "jtil/renderer/geometry/geometry_manager.h"
#include "jtil/renderer/shader/shader_program.h"
#include "jtil/data_str/hash_funcs.h"
#include "jtil/renderer/objects/aabbox.h"

namespace jtil {

using math::Float4x4;
using math::Float2;
using math::Float3;
using math::Float4;

namespace renderer {

  LightSpotData::LightSpotData() {
    diffuse_color.set(1, 1, 1);
    diffuse_intensity = 1.0f;
    near_far.set(0.1f, 10.0f);
    spec_intensity = 0.2f;
  }

  LightSpotData& LightSpotData::operator=(const LightSpotData &rhs) {
    if (this != &rhs) {
      diffuse_color.set(rhs.diffuse_color);
      pos_view.set(rhs.pos_view);
      dir_view.set(rhs.dir_view);
      outer_angle_cosine = rhs.outer_angle_cosine;
      inner_minus_outer_cosine = rhs.inner_minus_outer_cosine;
      diffuse_intensity = rhs.diffuse_intensity;
      near_far.set(rhs.near_far);
      spec_intensity = rhs.spec_intensity;
    }
    return *this;
  }

  float LightSpot::outside_model_rad_scale_ = 0;

  LightSpot::LightSpot() : Light() {
    outer_fov_deg_ = 25.0f;
    inner_fov_deg_ = 20.0f;
    pos_world_.set(0, 0, 0);
    dir_world_.set(0, -1, 0);

    if (outside_model_rad_scale_ == 0) {
      float out_rad = GeometryManager::calcConeOutsideRadius(
        LIGHT_SPOT_MODEL_SLICES, LIGHT_SPOT_MODEL_INSIDE_RADIUS);
      outside_model_rad_scale_ = out_rad / LIGHT_SPOT_MODEL_INSIDE_RADIUS;
    }

    float out_rad = outside_model_rad_scale_ * LIGHT_SPOT_MODEL_INSIDE_RADIUS;
    Float3 min_bound(-out_rad, 0, -out_rad);
    Float3 max_bound(out_rad, LIGHT_SPOT_MODEL_HEIGHT, out_rad);
    aabbox_ = new objects::AABBox();
    aabbox_->init(min_bound, max_bound);

    update();
  }

  LightSpot::~LightSpot() {
    if (aabbox_) {
      delete aabbox_;
      aabbox_ = NULL;
    }
  }

  void LightSpot::update() {
    light_data_.outer_angle_cosine = cosf(outer_fov_deg_ * (float)PI_OVER_180);
    if (inner_fov_deg_ > outer_fov_deg_) {
      throw std::wruntime_error("LightSpot::update() - inner_fov > outer_fov");
    }
    float inner_angle_cosine = cosf(inner_fov_deg_ * (float)PI_OVER_180);
    light_data_.inner_minus_outer_cosine = inner_angle_cosine - 
      light_data_.outer_angle_cosine;

    // Calculate position in view space
    Float3::affineTransformPos(light_data_.pos_view, 
      Renderer::g_renderer()->camera()->view(), pos_world_);

    // Calculate direction in view space
    dir_world_.normalize();
    Float3::affineTransformVec(light_data_.dir_view, 
      Renderer::g_renderer()->camera()->view(), dir_world_);
    light_data_.dir_view.normalize();  // Just in case

    // Calculate the object volume matrix
    calcMatWorld(mat_world_);

    aabbox_->update(mat_world_);
    
    Float3 dir_world_scaled(dir_world_);
    Float3::scale(dir_world_scaled, light_data_.near_far[1]);
    Float3::add(cone_center_world_, dir_world_scaled, pos_world_);
  }

  void LightSpot::updatePVW() {
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

  // cameraInside --> Assume update has been called!
  bool LightSpot::cameraInside(const Camera* camera) const {
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

  void LightSpot::scalePVWMat(const float scale) {
    vw_mat_.rightMultScale(scale, scale, scale);
    normal_mat_.rightMultScale(1.0f/scale, 1.0f/scale, 1.0f/scale);
    pvw_mat_.rightMultScale(scale, scale, scale);
    pvm_prev_frame_.rightMultScale(scale, scale, scale);
  }

  void LightSpot::calcViewMat(math::Float4x4* view_return) {
    const Float3 up(0.0f, 1.0f, 0.0f);
    view_return->glLookAt(up, dir_world_, pos_world_);
  }

  void LightSpot::calcMatWorld(math::Float4x4& ret) {
    Float4x4 scale_mat;
    scale_mat.identity();

    // if the light's cone height was model_height, calculate what the base 
    // radius would be:
    float cone_inside_radius_ = tanf(outer_fov_deg_ *  (float)PI_OVER_180) * 
      LIGHT_SPOT_MODEL_HEIGHT;
    cone_outside_radius_ = cone_inside_radius_ * outside_model_rad_scale_;

    // from this work out the X and Z direction scale factors
    scale_mat[0] = cone_inside_radius_ / LIGHT_SPOT_MODEL_INSIDE_RADIUS;
    scale_mat[5] = 1.0f;  // (1,1)
    scale_mat[10] = cone_inside_radius_ / LIGHT_SPOT_MODEL_INSIDE_RADIUS;

    // Now, since the spot light has a height, scale accordingly
    float scale = light_data_.near_far[1] / LIGHT_SPOT_MODEL_HEIGHT;
    scale_mat[0] *= scale;
    scale_mat[5] *= scale;
    scale_mat[10] *= scale;
    cone_outside_radius_ *= scale;

    // The cone will also have some orientation so calc the appropriate matrix
    Float3 dir(dir_world_);
    dir.normalize();  // Just in case

    // recall: acosine of dot product will give angle between vectors if their 
    // magnitudes are 1
    static const math::Float3 model_forward(0, 1, 0);
    float rot_angle = acosf(Float3::dot(dir, model_forward));

    // Check that up is not on the same axis as direction vector
    while (fabsf(rot_angle-static_cast<float>(M_PI)) < 0.0001f || 
        fabsf(rot_angle) < LOOSE_EPSILON) {
      // If it is, preturb the direction vector slightly
      dir[0] += 0.0001f;
      dir.normalize();
      rot_angle = acosf(Float3::dot(dir, model_forward));
    }

    Float3 rot_axis;
    // cross product of up and direction will give axis of rotation
    Float3::cross(rot_axis, model_forward, dir); 
    rot_axis.normalize();

    Float4x4 rot_mat;
    rot_mat.rotateMatAxisAngle(rot_axis, rot_angle);

    Float4x4::multSIMD(ret, rot_mat, scale_mat);
    ret.leftMultTranslation(pos_world_[0], pos_world_[1], pos_world_[2]);
  }

  void LightSpot::setHandles() const {
    BIND_UNIFORM("f_light.diffuse_color", light_data_.diffuse_color.m);
    BIND_UNIFORM("f_light.pos_view", light_data_.pos_view.m);
    BIND_UNIFORM("f_light.dir_view", light_data_.dir_view.m);
    BIND_UNIFORM("f_light.outer_angle_cosine", 
      &light_data_.outer_angle_cosine);
    BIND_UNIFORM("f_light.inner_minus_outer_cosine", 
      &light_data_.inner_minus_outer_cosine);
    BIND_UNIFORM("f_light.diffuse_intensity", 
      &light_data_.diffuse_intensity);
    BIND_UNIFORM("f_light.near_far", light_data_.near_far.m);
    BIND_UNIFORM("f_light.spec_intensity", &light_data_.spec_intensity);
  }

}  // namespace renderer
}  // namespace jtil

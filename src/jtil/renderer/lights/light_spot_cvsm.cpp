#include <iostream>
#include "jtil/renderer/lights/light_spot_cvsm.h"
#include "jtil/renderer/lighting.h"
#include "jtil/renderer/renderer.h"
#include "jtil/renderer/camera/camera.h"
#include "jtil/settings/settings_manager.h"

namespace jtil {

using math::Float4x4;
using math::Float3;

namespace renderer {

  const Float3 LightSpotCVSM::up_(0.0f, 1.0f, 0.0f);

  LightSpotCVSM::LightSpotCVSM(Renderer* renderer) : LightSpot() {
    renderer_ = renderer;
    cvsm_count_ = 1;
    GET_SETTING("vsm_default_softness", float, softness_);
  }

  LightSpotCVSM::~LightSpotCVSM() {

  }

  void LightSpotCVSM::update() {
    LightSpot::update();
  }

  void LightSpotCVSM::updatePVW() {
    LightSpot::updatePVW();
  }

  void LightSpotCVSM::cvsm_count(const uint32_t cvsm_count) {
    if (cvsm_count < 1 || cvsm_count > LIGHTING_CVSM_MAX_COUNT) {
      throw std::wruntime_error("LightSpotCVSM::cvsm_count() - ERROR: "
        "cvsm_count is out of bounds!");
    }
    cvsm_count_ = cvsm_count;
  }

  void LightSpotCVSM::updateVSMMatrices() {
    // Calculate the View matrix
    // Point the up towards the camera
    Float3 up_world;
    Float3::sub(up_world, renderer_->camera()->eye_pos_world(), pos_world_);
    up_world.normalize();
    if (abs(abs(Float3::dot(dir_world_, up_world)) - 1.0f) < LOOSE_EPSILON) {
      // Preturb the up vector
      up_world[0] += 0.001f;
      up_world.normalize();
    }
    Float4x4::glLookAt(v_mat_vsm_, up_world, dir_world_, pos_world_);

    // Update the render data structure
    light_data_vsm_ = light_data_;
    light_data_vsm_.pos_view.set(pos_world_);
    light_data_vsm_.dir_view.set(dir_world_);

    // Fit near and far to scene objects's world bounds. Important for cascaded
    // shadow maps to reduce aliasing.
    float near_min = -light_data_.near_far[0];
    float far_max = -light_data_.near_far[1];
    renderer_->fitViewNearFarToObjects(light_data_vsm_.near_far, v_mat_vsm_, 
      near_min, far_max, false);

    // Calculate the Projection Matrix
    int vsm_res_enum;
    GET_SETTING("vsm_resolution_enum", int, vsm_res_enum);
    float res = static_cast<float>(VSMResEnum2Res((VSM_RES)vsm_res_enum));

    p_mat_vsm_.glProjection(-light_data_vsm_.near_far[0],
      -light_data_vsm_.near_far[1], outer_fov_deg_ * 2.0f, res, res);

    // Rebuild Projection * View matrix
    Float4x4::multSIMD(pv_mat_vsm_, p_mat_vsm_, v_mat_vsm_);
  }

  uint32_t LightSpotCVSM::VSMResEnum2Res(VSM_RES sm_res_enum) {
    switch (sm_res_enum) {
    case VSM_RES_256:
      return 256;
     case VSM_RES_512:
       return 512;
     case VSM_RES_1024:
       return 1024;
     case VSM_RES_2048:
       return 2048;
     default:
       throw std::wruntime_error("VSMResEnum2Res() - Undefined enum value!");
     }
   }

   void LightSpotCVSM::setHandles() const {
     LightSpot::setHandles();
     // Nothing else to do for now.
   }
}  // namespace renderer
}  // namespace jtil

//
//  light_spot_vsm.h
//
//  Created by Jonathan Tompson on 6/15/12.
//
//  This class is pretty heavy (lots of data), but we only expect one or two
//  of them so I judge that it's OK.
// 

#pragma once

#include "jtil/renderer/lights/light_spot.h"
#include "jtil/math/math_types.h"

namespace jtil {
namespace renderer {
  class Renderer;

  typedef enum {
    VSM_RES_256,
    VSM_RES_512,
    VSM_RES_1024,
    VSM_RES_2048,
    NUM_VSM_RES,
  } VSM_RES;


  class LightSpotCVSM : public LightSpot {
  public:
    LightSpotCVSM(Renderer* renderer);
    ~LightSpotCVSM();

    // To avoid dynamic_cast when dealing with polymorphism
    // Instead: check type and call reinterpret_cast
    inline virtual LightType type() const { return LIGHT_SPOT_VSM; }

    inline const math::Float2& near_far_vsm() const { return light_data_vsm_.near_far; }

    inline const math::Float4x4& pv_mat_vsm() const { return pv_mat_vsm_; }
    inline const math::Float4x4& p_mat_vsm() const { return p_mat_vsm_; }
    inline const math::Float4x4& v_mat_vsm() const { return v_mat_vsm_; }
    inline const uint32_t& cvsm_count() const { return cvsm_count_; }
    inline float& softness() { return softness_; }
    inline const float& softness() const { return softness_; }

    void cvsm_count(const uint32_t cvsm_count);

    virtual void update();
    virtual void updatePVW();

    void updateVSMMatrices();
    void initLightSpotCVSM();
    void setLightingVSMNearFar();

    static uint32_t VSMResEnum2Res(const VSM_RES sm_res_enum);

    // Set the necessary handles in the shader programs
    virtual void setHandles() const;

  private:
    // Structures also used for VSM rendering
    Renderer* renderer_;  // Not owned here!
    LightSpotData	light_data_vsm_;  // For Internal usage (use parent's data)
    static const math::Float3 up_;
    math::Float4x4 v_mat_vsm_;
    math::Float4x4 p_mat_vsm_;
    math::Float4x4 pv_mat_vsm_;
    uint32_t cvsm_count_;
    float softness_;

    // Non-copyable, non-assignable.
    LightSpotCVSM(LightSpotCVSM&);
    LightSpotCVSM& operator=(const LightSpotCVSM&);
  };

};  // namespace renderer
};  // namespace jtil


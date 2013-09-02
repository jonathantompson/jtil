//
//  light.h
//
//  Created by Jonathan Tompson on 6/15/12.
//

#pragma once

#include "jtil/math/math_types.h"

namespace jtil {
namespace renderer {

  namespace objects { class AABBox; }

  class ShaderProgram;
  class Camera;

  typedef enum {
    LIGHT_BASE,
    LIGHT_SPOT,
    LIGHT_SPOT_VSM,
    LIGHT_DIR,
    LIGHT_POINT,
  } LightType;

  class Light {
  public:
    Light();
    virtual ~Light();

    inline virtual LightType type() const { return LIGHT_BASE; }

    inline virtual void update() { }
    virtual void updatePVW() { }
    virtual bool cameraInside(const Camera* camera) const {
      static_cast<void>(camera); return true; }
    bool& on() { return on_; }
    const bool& on() const { return on_; }

    inline objects::AABBox* aabbox() { return aabbox_; }

    // Set the necessary handles in the shader programs
    virtual void setHandles(ShaderProgram* sp) const { static_cast<void>(sp); }

  protected:
    objects::AABBox* aabbox_;  // Bounding volume for frustrum tests
                               // Not all lights will have one (NULL)
    bool on_;

    // Non-copyable, non-assignable.
    Light(Light&);
    Light& operator=(const Light&);
  };

};  // namespace renderer
};  // namespace jtil

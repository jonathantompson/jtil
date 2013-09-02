//
//  bsphere.h
//
//  Created by Jonathan Tompson on 5/22/12.
//
//  Unlike the AABBox class, BSphere objects are not part of the scene graph
//  and instead just attach onto objects.  I use them for various projects,
//  outside of the renderer.
//

#pragma once

#include "jtil/math/math_types.h"

namespace jtil {
namespace renderer {
  class GeometryInstance;

namespace objects {
  class BSphere {
  public:
    // Constructor / Destructor
    BSphere(float radius, jtil::math::Float3& center, 
      jtil::renderer::GeometryInstance* parent_node);
    ~BSphere();

    inline jtil::renderer::GeometryInstance* parent_node() { return parent_node_; }

    void transform();  // transform center and radius
    void transformCenter();  // just transform center
    jtil::math::Float3* transformed_center() { return &transformed_center_; }
    float transformed_radius() { return transformed_radius_; }
    inline const jtil::math::Float3& center() { return center_; }

  protected:
    float radius_;
    jtil::math::Float3 center_;
    jtil::math::Float3 transformed_rad_vec_;  // Just to avoid putting it on the stack
    jtil::math::Float3 transformed_center_;
    float transformed_radius_;

    jtil::renderer::GeometryInstance* parent_node_;

    // Non-copyable, non-assignable.
    BSphere(BSphere&);
    BSphere& operator=(const BSphere&);
  };
  
};  // namespace objects
};  // namespace renderer
};  // namespace jtil

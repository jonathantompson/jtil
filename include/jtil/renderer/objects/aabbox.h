//
//  aabbox.h
//
//  Created by Jonathan Tompson on 6/24/12.
//

#pragma once

#include "jtil/math/math_types.h"

namespace jtil {

namespace data_str { template <typename T> class Vector; }

namespace renderer {

  class Frustum;

namespace objects {
  class AABBox {
  public:
    AABBox();
    AABBox(const AABBox& other);  // copy constructor
    ~AABBox();

    void init(const data_str::Vector<math::Float3>& vertices);
    void init(const math::Float3& min, const math::Float3& max);
    void update(const math::Float4x4& mat_world);

    void calcMinMaxZBoundInViewSpace(float& min_z, float& max_z, 
      const math::Float4x4& view_mat);

    inline const math::Float3& min_bounds() const { return min_; }
    inline const math::Float3& max_bounds() const { return max_; }
    inline const math::Float3& center() const { return center_; }
    inline const math::Float3& half_lengths() const { return half_lengths_; }

    bool frustumCullTest(const Frustum* frustum) const;

  private:
    math::Float3 min_;  // Min world coord --> updated once per frame
    math::Float3 max_;  // Max world coord
    math::Float3 object_bounds_[8];  // Bounds initialized during startup
    math::Float3 world_bounds_[8];  // World bounds caculated each frame
    math::Float3 center_;  // Center of the box
    math::Float3 half_lengths_;  // Half lengths of each of the dimenions

    // Expand(), check current min/max value against input vector and set new 
    // min/max if appropriate
    void expand(const math::Float3& vec);
  };
};  // namespace objects
};  // namespace renderer
};  // namespace jtil

#include <limits>
#include "jtil/renderer/objects/aabbox.h"
#include "jtil/data_str/vector.h"
#include "jtil/renderer/camera/frustum.h"

namespace jtil {

using math::Float3;
using math::Float4x4;
using data_str::Vector;

namespace renderer {
namespace objects {

  AABBox::AABBox() {
    min_.set(0,0,0);
    max_.set(0,0,0);
  }

  AABBox::~AABBox() {

  }

  AABBox::AABBox(const AABBox& other) {
    min_.set(other.min_);
    max_.set(other.max_);
    center_.set(other.center_);
    half_lengths_.set(other.half_lengths_);
    for (uint32_t i = 0; i < 8; i++) {
      object_bounds_[i].set(other.object_bounds_[i]);
      world_bounds_[i].set(other.world_bounds_[i]);
    }
  }

  void AABBox::init(const Vector<Float3>& vertices) {
    min_.set(std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::infinity());
    max_.set(-std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity());
    for (uint32_t i = 0; i < vertices.size(); i++) {
      if (vertices[i][0] < min_[0]) {
        min_[0] = vertices[i][0];
      }
      if (vertices[i][1] < min_[1]) {
        min_[1] = vertices[i][1];
      }
      if (vertices[i][2] < min_[2]) {
        min_[2] = vertices[i][2];
      }
      if (vertices[i][0] > max_[0]) {
        max_[0] = vertices[i][0];
      }
      if (vertices[i][1] > max_[1]) {
        max_[1] = vertices[i][1];
      }
      if (vertices[i][2] > max_[2]) {
        max_[2] = vertices[i][2];
      }
    }
    init(min_, max_);
  }

  void AABBox::init(const Float3& min, const Float3& max) {
    min_.set(min);
    max_.set(max);
    // Now fill up the AABBox coordinates
    object_bounds_[0].set(min_[0], max_[1], min_[2]);  // top front left
    object_bounds_[1].set(min_[0], max_[1], max_[2]);  // top back left
    object_bounds_[2].set(max_[0], max_[1], max_[2]);  // top back right
    object_bounds_[3].set(max_[0], max_[1], min_[2]);  // top front right
    object_bounds_[4].set(min_[0], min_[1], min_[2]);  // bottom front left
    object_bounds_[5].set(min_[0], min_[1], max_[2]);  // bottom back left
    object_bounds_[6].set(max_[0], min_[1], max_[2]);  // bottom back right
    object_bounds_[7].set(max_[0], min_[1], min_[2]);  // bottom front right
  }

  // Check Min/Max against input vector and update
  void AABBox::expand(const Float3& vec) {
    if (min_[0] > vec[0]) {
      min_[0] = vec[0];
    }
    if (min_[1] > vec[1]) {
      min_[1] = vec[1];
    }
    if (min_[2] > vec[2]) {
      min_[2] = vec[2];
    }
    if (max_[0] < vec[0]) {
      max_[0] = vec[0];
    }
    if (max_[1] < vec[1]) {
      max_[1] = vec[1];
    }
    if (max_[2] < vec[2]) {
      max_[2] = vec[2];
    }
  }

  void AABBox::update(const Float4x4& mat_world) {
    // Get bounding box in world coordinates
    for (uint32_t i = 0; i < 8; i++) {
      Float3::affineTransformPos(world_bounds_[i], mat_world,
        object_bounds_[i]);
    }

    // Now get bounding box in axis aligned world coordinates --> Box area 
    // will grow --> ie, not necessarily an optimal bounding volume.
    min_[0] = world_bounds_[0][0]; 
    min_[1] = world_bounds_[0][1]; 
    min_[2] = world_bounds_[0][2]; 
    max_.set(min_);

    for (uint32_t i = 1; i < 8; i++) {
      expand(world_bounds_[i]);
    }

    // Calculate the center (used by most of the collision query routines)
    Float3::add(center_, min_, max_);
    Float3::scale(center_, 0.5f);
    Float3::sub(half_lengths_, max_, min_);
    Float3::scale(half_lengths_, 0.5f);
  }

  void AABBox::calcMinMaxZBoundInViewSpace(float& min_z, float& max_z, 
    const math::Float4x4& view_mat) {
      // Recall: OpenGL convention is to look down the negative Z axis,
      //         therefore, more negative values are actually further away.
      Float3 view_bound;
      Float3::affineTransformPos(view_bound, view_mat, world_bounds_[0]);
      min_z = view_bound[2];
      max_z = view_bound[2];

      for (uint32_t i = 1; i < 8; i++) {
        Float3::affineTransformPos(view_bound, view_mat, world_bounds_[i]);
        if (view_bound[2] > min_z) {
          min_z = view_bound[2];
        }
        if (view_bound[2] < max_z) {
          max_z = view_bound[2];
        }
      }
  }

  bool AABBox::frustumCullTest(const Frustum* frustum) const {
    float bounds[6];
    bounds[0] = min_[0];
    bounds[1] = min_[1];
    bounds[2] = min_[2];
    bounds[3] = max_[0];
    bounds[4] = max_[1];
    bounds[5] = max_[2];
    ViewTest result = frustum->ViewTestAABB(bounds, (ViewTest)0);
    return !(result & VT_OUTSIDE);
  }

}  // namespace objects
}  // namespace renderer
}  // namespace jtil
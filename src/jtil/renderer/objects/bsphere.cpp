#include <fstream>
#include <iostream>
#include "jtil/data_str/pair.h"
#include "jtil/renderer/objects/bsphere.h"
#include "jtil/renderer/geometry/geometry_instance.h"
#include "jtil/math/math_types.h"

using jtil::math::Float3;
using std::wstring;
using std::runtime_error;
using std::string;
using std::cout;
using std::endl;
using jtil::data_str::Pair;

namespace jtil {
namespace renderer {
namespace objects {
  BSphere::BSphere(float rad, Float3& center, GeometryInstance* parent_node) {
    radius_ = rad;
    center_ = center;
    parent_node_ = parent_node;
  }

  BSphere::~BSphere() {
    // parent destructor will be called automatically
  }

  void BSphere::transform() {
    transformCenter();

    // Figure out the transformed radius.  The following WONT work if there is
    // sqew.  It's a bit of a hack...
    jtil::math::Float3 rad_vec_(radius_, 0, 0);
    Float3::affineTransformVec(transformed_rad_vec_, 
      parent_node_->mat_hierarchy(), rad_vec_);
    transformed_radius_ = transformed_rad_vec_.length();
    float rad_sq_ = Float3::dot(transformed_rad_vec_, transformed_rad_vec_);

    rad_vec_.set(0, radius_, 0);
    Float3::affineTransformVec(transformed_rad_vec_, 
      parent_node_->mat_hierarchy(), rad_vec_);
    rad_sq_ = std::max<float>(Float3::dot(transformed_rad_vec_,
      transformed_rad_vec_), rad_sq_);

    rad_vec_.set(0, 0, radius_);
    Float3::affineTransformVec(transformed_rad_vec_, 
      parent_node_->mat_hierarchy(), rad_vec_);
    rad_sq_ = std::max<float>(Float3::dot(transformed_rad_vec_,
      transformed_rad_vec_), rad_sq_);
    transformed_radius_ = sqrtf(rad_sq_);
  }

  void BSphere::transformCenter() {
    Float3::affineTransformPos(transformed_center_, 
      parent_node_->mat_hierarchy(), center_);
  }

}  // namespace objects
}  // namespace renderer
}  // namespace jtil


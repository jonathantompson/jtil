//
//  geometry_instance.h
//
//  Created by Jonathan Tompson on 6/1/12.
//
//  An instance of a Geometry class element.  Has an affine transform, a 
//  pointer to the geometry (mesh) and some material properties.
//

#pragma once

#include "jtil/math/math_types.h"
#include "jtil/renderer/geometry/geometry.h"  // for GeometryType
#include "jtil/renderer/material/material.h"
#include "jtil/data_str/vector_managed.h"
#include "jtil/data_str/vector.h"

namespace jtil {
namespace data_str { template <typename TFirst, typename TSecond> class Pair; }

namespace renderer {
  
  class GeometryInstance;
  class GeometryManager;

  namespace objects { class AABBox; }

  class GeometryInstance {
  public:
    // Constructor / Destructor
    GeometryInstance(const Geometry* geom = NULL);
    virtual ~GeometryInstance();
    void addChild(GeometryInstance* child);

    void draw() const;
    GeometryType type() const;
    void bindRGBTexture(const GLenum target_id) const;
    void bindBumpTexture(const GLenum target_id) const;
    void bindDispTexture(const GLenum target_id) const;
    void bindBoneMatrices(const bool hq_motion_blur);
    
    // getter setter methods
    inline GeometryInstance* parent() { return parent_; }
    inline math::Float4x4& mat() { return mat_; }
    inline math::Float4x4& mat_hierarchy() { return mat_hierarchy_; }
    inline math::Float4x4*& mat_hierarchy_inv() { return mat_hierarchy_inv_; }
    inline math::Float4x4& pvm_prev_frame() { return pvm_prev_frame_; }
    inline math::Float4x4*& bone_transform() { return bone_transform_; }
    inline uint32_t numChildren() const { return children_.size(); }
    inline const data_str::VectorManaged<GeometryInstance*>& children() const { 
      return children_; }
    inline GeometryInstance* getChild(uint32_t i) const {return children_[i];}
    inline objects::AABBox* aabbox() { return aabbox_; }
    inline const Geometry* geom() const { return geom_; }
    inline Geometry* geom() { return const_cast<Geometry*>(geom_); }  // TODO: fix this
    inline Material& mtrl() { return mtrl_; }
    inline std::string& name() { return name_; }
    inline const std::string& name() const { return name_; }
    inline bool& render() { return render_; }
    inline const bool& render() const { return render_; }
    inline data_str::Vector<GeometryInstance*>& bone_nodes() { 
      return bone_nodes_; }
    inline Bone*& bone() { return bone_; }
    inline GeometryInstance*& bone_root_node() { return bone_root_node_; }
    inline bool& apply_lighting() { return apply_lighting_; }
    inline float& point_size() { return point_size_; }

    void setRenderHierarchy(const bool render);

    // Methods for saving to and from file
    data_str::Pair<uint8_t*,uint32_t> saveToArray() const;
    static GeometryInstance* loadFromArray(GeometryManager* gm, 
      const data_str::Pair<uint8_t*,uint32_t>& data);

  protected:
    const Geometry* geom_;  // NOT OWNED HERE
    math::Float4x4 mat_;
    math::Float4x4 mat_hierarchy_;  // Updated when rendering
    math::Float4x4 pvm_prev_frame_;  // Expensive to keep, but used for vel buf
    math::Float4x4* bone_transform_;  // Only allocated if needed
    GeometryInstance* parent_;
    data_str::VectorManaged<GeometryInstance*> children_;
    data_str::Vector<GeometryInstance*> bone_nodes_;  // nonempty if geom boned
    objects::AABBox* aabbox_;  // Bounding volume for frustrum tests
    Material mtrl_;
    Bone* bone_;  // Only non-NULL for bone nodes
    GeometryInstance* bone_root_node_;  // Only non-NULL for bone nodes
    math::Float4x4* mat_hierarchy_inv_;  // Only allocated if needed
    float* concat_bone_trans_prev_frame_;  // Only allocated if needed
    bool apply_lighting_;  // true by default
    float point_size_;  // 10 by default (only applies to point primatives)

    static float concat_bone_trans_[MAX_BONE_COUNT * 16 * 4];

    std::string name_;
    bool render_;

    // non-assignable and non-copyable
    GeometryInstance& operator=(const GeometryInstance&);
    GeometryInstance(const GeometryInstance& other);
  };
};  // namespace renderer
};  // namespace jtil


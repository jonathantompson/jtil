#include "jtil/renderer/geometry/geometry_instance.h"
#include "jtil/renderer/geometry/geometry.h"
#include "jtil/renderer/geometry/geometry_manager.h"
#include "jtil/renderer/shader/shader_program.h"
#include "jtil/renderer/objects/aabbox.h"
#include "jtil/data_str/pair.h"
#include "jtil/ucl/ucl_helper.h"
#include "jtil/file_io/data_str_serialization.h"

using std::wstring;
using std::wruntime_error;

namespace jtil {

using math::Float3;
using math::Float2;
using math::Float4x4;
using renderer::objects::AABBox;
using data_str::Pair;
using ucl::UCLHelper;

#ifndef BUFFER_OFFSET
	#define BUFFER_OFFSET(bytes) ((GLubyte*) NULL + bytes)
#endif
#ifndef SAFE_DELETE
  #define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }
#endif
#ifndef SAFE_DELETE_ARR
  #define SAFE_DELETE_ARR(x) if (x != NULL) { delete[] x; x = NULL; }
#endif

namespace renderer {

  float GeometryInstance::concat_bone_trans_[MAX_BONE_COUNT * 16 * 4];

  GeometryInstance::GeometryInstance(const Geometry* geom) {
    mat_.identity(); 
    parent_ = NULL;
    aabbox_ = NULL;
    geom_ = geom;
    if (geom_) {
      aabbox_ = new AABBox();
      aabbox_->init(geom->pos());
    }
    render_ = true;
    bone_ = NULL;
    bone_root_node_ = NULL;
    mat_hierarchy_inv_ = NULL;
    bone_transform_ = NULL;
    concat_bone_trans_prev_frame_ = NULL;
  }

  GeometryInstance::~GeometryInstance() {
    SAFE_DELETE_ARR(concat_bone_trans_prev_frame_);
    SAFE_DELETE(aabbox_);
    SAFE_DELETE(mat_hierarchy_inv_);
    SAFE_DELETE(bone_transform_);
    children_.clear();  // Explicitly delete all the children (calling destrs)
  }

  void GeometryInstance::addChild(GeometryInstance* child) {
    children_.pushBack(child);
    child->parent_ = this;
  }

  void GeometryInstance::draw() const {
    if (geom_) {
      geom_->draw();
    }
  }

  GeometryType GeometryInstance::type() const {
    if (geom_) {
      return geom_->type();
    } else {
      return GEOMETRY_BASE;
    }
  }

  void GeometryInstance::bindRGBTexture(const GLenum target_id) const {
    if (geom_) {
      geom_->bindRGBTexture(target_id);
    }
  }

  void GeometryInstance::bindBumpTexture(const GLenum target_id) const {
    if (geom_) {
      geom_->bindBumpTexture(target_id);
    }
  }

  void GeometryInstance::bindDispTexture(const GLenum target_id) const {
    if (geom_) {
      geom_->bindDispTexture(target_id);
    }
  }

  void GeometryInstance::bindBoneMatrices(const bool hq_motion_blur) {
    if (geom_ && bone_nodes_.size() > 0) {

      // Copy the bone transforms into a flat array and send it to the shader
      for (uint32_t i = 0; i < bone_nodes_.size(); i++) {
        memcpy(&concat_bone_trans_[i*16], bone_nodes_[i]->bone_transform_->m, 
          16 * sizeof(concat_bone_trans_[0]));
      }
      BIND_UNIFORM("bone_trans", concat_bone_trans_);

      if (hq_motion_blur && QUERY_UNIFORM("bone_trans_prev_frame")) {
        if (concat_bone_trans_prev_frame_ == NULL) {
          concat_bone_trans_prev_frame_ = new float[MAX_BONE_COUNT * 16 * 4];
          memcpy(concat_bone_trans_prev_frame_, concat_bone_trans_, 
            16 * sizeof(concat_bone_trans_[0]) * bone_nodes_.size());
        }
        BIND_UNIFORM("bone_trans_prev_frame", concat_bone_trans_prev_frame_);
        memcpy(concat_bone_trans_prev_frame_, concat_bone_trans_, 
            16 * sizeof(concat_bone_trans_[0]) * bone_nodes_.size());
      }
    }
  }

  Pair<uint8_t*,uint32_t> GeometryInstance::saveToArray() const {
    Pair<uint8_t*,uint32_t> data;
    data.first = NULL;
    data.second = 0;

    char char_dummy;
    float float_dummy;
    static_cast<void>(char_dummy);
    static_cast<void>(float_dummy);
    if (sizeof(char_dummy) != 1 || sizeof(float_dummy) != 4) {
      throw std::runtime_error("saveToArray - basic types are the wrong size!");
    }
    
    // Calculate the total size of the geometry data
    uint32_t size = 0;  // in bytes
    // size of the name string AND the length of the name
    size += 4;  // num_children
    size += (uint32_t)(name_.size() + 1) + 4;
    if (geom_) {
      size += (uint32_t)(geom_->name().size() + 1) + 4;
    } else {
      size += 1 + 4;
    }
    size += MATERIAL_FILE_SIZE_BYTES;
    size += 4 * 16;  // matrix

    data.second = size;

    uint32_t overlap_compression_size = 
      UCLHelper::calcInPlaceCompressSizeRequirement(size);
    data.first = (uint8_t*)malloc(overlap_compression_size);
    uint8_t* arr_ptr = data.first;

    // Now start saving the data
    file_io::uInt32ToArray(children_.size(), arr_ptr);
    file_io::stringToArray(name_, arr_ptr);  // Name
    if (geom_) {
      file_io::stringToArray(geom_->name(), arr_ptr);
    } else {
      static const std::string empty_str("");
      file_io::stringToArray(empty_str, arr_ptr);
    }
    file_io::materialToArray(mtrl_, arr_ptr);
    file_io::float4x4ToArray(mat_, arr_ptr);

    // double check we got the size right
    if (&data.first[size] != arr_ptr) {
      throw std::wruntime_error("Geometry::saveToArray() - ERROR: "
        "Array size was not correct (data might be corrupt!)");
    }

    return data;
  }

  GeometryInstance* GeometryInstance::loadFromArray(GeometryManager* gm, 
    const Pair<uint8_t*,uint32_t>& data) {
    const uint8_t* arr_ptr = data.first;
    uint32_t num_children;
    file_io::arrayToUInt32(num_children, arr_ptr);
    std::string name;
    file_io::arrayToString(name, arr_ptr);
    std::string geom_name;
    file_io::arrayToString(geom_name, arr_ptr);

    GeometryInstance* ret_instance;
    if (geom_name.size() == 0) {
      ret_instance = new GeometryInstance(NULL);
    } else {
      Geometry* geom = gm->findGeometryByName(geom_name);
      if (geom == NULL) {
        throw std::wruntime_error("GeometryInstance::loadFromArray() - ERROR: "
          "Couldn't find geometry by name for this instance node!");
      }
      ret_instance = new GeometryInstance(geom);
    }
    ret_instance->name_ = name;
    ret_instance->children_.capacity(num_children);
    file_io::arrayToMaterial(ret_instance->mtrl_, arr_ptr);
    file_io::arrayToFloat4x4(ret_instance->mat_, arr_ptr);

    // double check we got the size right
    if (&data.first[data.second] != arr_ptr) {
      throw std::wruntime_error("GeometryInstance::loadFromArray() - ERROR: "
        "Array size was not correct (data might be corrupt!)");
    }

    return ret_instance;
  }

  void GeometryInstance::setRenderHierarchy(const bool render) {
    render_ = render;
    for (uint32_t i = 0; i < children_.size(); i++) {
      children_[i]->setRenderHierarchy(render);
    }
  }

}  // namespace renderer
}  // namespace jtil

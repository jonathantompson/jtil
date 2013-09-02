//
//  bone.h
//
//  Created by Jonathan Tompson on 15/03/13.
//
//  This Bone struct is NOT the scene graph node that represents the actual
//  bone transform.  It is simply a collection of rest pose matricies which are
//  used when calculating a final bone node transform (which is sent to vertex
//  shader).
//
//  Many Geometry meshes may share bones.
//

#pragma once

#include <string>
#include "jtil/data_str/vector_managed.h"
#include "jtil/math/math_types.h"

namespace jtil {
namespace data_str { template <typename TFirst, typename TSecond> class Pair; }
namespace data_str { template <typename TKey, typename TValue> class HashMap; }

namespace renderer {

  struct Bone {
    Bone(const std::string& bone_name, float* bone_transform);
    Bone();
    virtual ~Bone() { }

    data_str::Pair<uint8_t*,uint32_t> saveToArray() const;
    static Bone* loadFromArray(const data_str::Pair<uint8_t*,uint32_t>& data);

    math::Float4x4 bone_offset;
    math::Float4x4 rest_transform;  // Default pose rest transform, not saved 
                                    // to disk but encoded in GeometryInstance
    std::string bone_name;  // Bone ids are the concatenation <path, file, name>
    uint32_t bone_index;  // into global bone array (avoids hash table overhead)

    Bone& operator=(const Bone &rhs);
  };

};  // namespace renderer
};  // namespace jtil

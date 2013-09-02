#include "jtil/renderer/geometry/bone.h"
#include "jtil/math/math_types.h"
#include "jtil/data_str/pair.h"
#include "jtil/data_str/hash_funcs.h"
#include "jtil/data_str/hash_map.h"
#include "jtil/ucl/ucl_helper.h"
#include "jtil/file_io/data_str_serialization.h"

using std::wruntime_error;
using std::wstring;

namespace jtil {

using math::Float4x4;
using data_str::Pair;
using ucl::UCLHelper;

#define BONE_FILE_INFO_START_HM_SIZE 7

namespace renderer {

  Bone& Bone::operator=(const Bone &rhs) {
    // Only do assignment if RHS is a different object from this.
    if (this != &rhs) {
      this->bone_offset.set(rhs.bone_offset);
      this->bone_name = rhs.bone_name;
    }
    return *this;
  }

  Bone::Bone(const std::string& bone_name, float* bone_transform) {
    this->bone_name = bone_name;
    bone_offset.set(bone_transform);
#ifdef COLUMN_MAJOR
    // assimp is row major, we are column major, matrix needs transpose
    bone_offset.transpose();
#endif
  }

  Bone::Bone() {
    this->bone_name = std::string();
    bone_offset.identity();
  }

  Pair<uint8_t*,uint32_t> Bone::saveToArray() const {
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
    size += (uint32_t)(bone_name.size() + 1) + 4;
    size += 4 * 16;  // matrix

    data.second = size;

    uint32_t overlap_compression_size = 
      UCLHelper::calcInPlaceCompressSizeRequirement(size);
    data.first = (uint8_t*)malloc(overlap_compression_size);
    uint8_t* arr_ptr = data.first;

    // Now start saving the data
    file_io::stringToArray(bone_name, arr_ptr);  // Name
    file_io::float4x4ToArray(bone_offset, arr_ptr);

    // double check we got the size right
    if (&data.first[size] != arr_ptr) {
      throw std::wruntime_error("Bone::saveToArray() - ERROR: "
        "Array size was not correct (data might be corrupt!)");
    }

    return data;
  }

  Bone* Bone::loadFromArray(const Pair<uint8_t*,uint32_t>& data) {
    const uint8_t* arr_ptr = data.first;

    Bone* ret_instance = new Bone();
    ret_instance->bone_index = MAX_UINT32;  // Needs to be calculated later

    file_io::arrayToString(ret_instance->bone_name, arr_ptr);
    file_io::arrayToFloat4x4(ret_instance->bone_offset, arr_ptr);

    // double check we got the size right
    if (&data.first[data.second] != arr_ptr) {
      throw std::wruntime_error("Bone::loadFromArray() - ERROR: "
        "Array size was not correct (data might be corrupt!)");
    }

    return ret_instance;
  }

}  // namespace renderer
}  // namespace jtil

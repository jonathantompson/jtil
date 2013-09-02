#include <cstring>
#include <sstream>
#include <string>
#include "jtil/file_io/data_str_serialization.h"
#include "jtil/data_str/vector.h"
#include "jtil/renderer/material/material.h"
#include "jtil/exceptions/wruntime_error.h"

using std::wruntime_error;
using std::string;

namespace jtil {

using data_str::Vector;
using math::Float2;
using math::Float3;
using math::Float4;
using math::Float4x4;
using math::Int4;
using renderer::Material;

namespace file_io {

  void float3ToArray(const Vector<Float3>& data, 
    uint8_t*& parr) {
    *((uint32_t*)parr) = data.size();
    parr += 4;
    for (uint32_t i = 0; i < data.size(); i++) {
      float3ToArray(data[i], parr);
    }
  }

  void float3ToArray(const Float3& data, uint8_t*& parr) {
    for (uint32_t i = 0; i < 3; i++) {
      *((float*)parr) = data.m[i];
      parr += 4;
    }
  }

  void float2ToArray(const Vector<Float2>& data, 
    uint8_t*& parr) {
    *((uint32_t*)parr) = data.size();
    parr += 4;
    for (uint32_t i = 0; i < data.size(); i++) {
      float2ToArray(data[i], parr);
    }
  }

  void float2ToArray(const Float2& data, uint8_t*& parr) {
    for (uint32_t i = 0; i < 2; i++) {
      *((float*)parr) = data.m[i];
      parr += 4;
    }
  }

  void float4ToArray(const Vector<Float4>& data, 
    uint8_t*& parr) {
    *((uint32_t*)parr) = data.size();
    parr += 4;
    for (uint32_t i = 0; i < data.size(); i++) {
      float4ToArray(data[i], parr);
    }
  }

  void float4ToArray(const Float4& data, uint8_t*& parr) {
    for (uint32_t i = 0; i < 4; i++) {
      *((float*)parr) = data.m[i];
      parr += 4;
    }
  }

  void int4ToArray(const Vector<Int4>& data, 
    uint8_t*& parr) {
    *((uint32_t*)parr) = data.size();
    parr += 4;
    for (uint32_t i = 0; i < data.size(); i++) {
      int4ToArray(data[i], parr);
    }
  }

  void int4ToArray(const Int4& data, uint8_t*& parr) {
    for (uint32_t i = 0; i < 4; i++) {
      *((int32_t*)parr) = data[i];
      parr += 4;
    }
  }

  void uInt32ToArray(const Vector<uint32_t>& data, 
    uint8_t*& parr) {
    *((uint32_t*)parr) = data.size();
    parr += 4;
    for (uint32_t i = 0; i < data.size(); i++) {
      uInt32ToArray(data[i], parr);
    }
  }

  void stringToArray(const string& str, uint8_t*& parr) {
    *((uint32_t*)parr) = (uint32_t)str.size();
    parr += 4;
    char* name_c_str = (char*)(parr);
    strncpy(name_c_str, str.c_str(), str.size() + 1);
    parr += str.size() + 1;
  }

  void uInt32ToArray(const uint32_t val, uint8_t*& parr) {
    *((uint32_t*)parr) = val;
    parr += 4;
  }

  void materialToArray(const Material& val, 
    uint8_t*& parr) {
    *((float*)parr) = val.spec_power;
    parr += 4;
    *((float*)parr) = val.spec_intensity;
    parr += 4;
    float3ToArray(val.albedo, parr);
  }

  void float4x4ToArray(const math::Float4x4& mat, 
    uint8_t*& parr) {
    for (uint32_t i = 0; i < 16; i++) {
      *((float*)parr) = mat.m[i];
      parr += 4;
    }
  }

  void arrayToFloat3(Vector<Float3>& data, 
    const uint8_t*& parr) {
    uint32_t size = *((uint32_t*)parr);
    parr += 4;
    data.capacity(size);
    data.resize(size);
    for (uint32_t i = 0; i < data.size(); i++) {
      arrayToFloat3(data[i], parr);
    }
  }

  void arrayToFloat3(Float3& data, const uint8_t*& parr) {
    for (uint32_t i = 0; i < 3; i++) {
      data.m[i] = *((float*)parr);
      parr += 4;
    }
  }

  void arrayToFloat2(Vector<Float2>& data, 
    const uint8_t*& parr) {
    uint32_t size = *((uint32_t*)parr);
    parr += 4;
    data.capacity(size);
    data.resize(size);
    for (uint32_t i = 0; i < data.size(); i++) {
      arrayToFloat2(data[i], parr);
    }
  }

  void arrayToFloat2(Float2& data, const uint8_t*& parr) {
    for (uint32_t i = 0; i < 2; i++) {
      data.m[i] = *((float*)parr);
      parr += 4;
    }
  }

  void arrayToFloat4(Vector<Float4>& data, 
    const uint8_t*& parr) {
    uint32_t size = *((uint32_t*)parr);
    parr += 4;
    data.capacity(size);
    data.resize(size);
    for (uint32_t i = 0; i < data.size(); i++) {
      arrayToFloat4(data[i], parr);
    }
  }

  void arrayToFloat4(Float4& data, const uint8_t*& parr) {
    for (uint32_t i = 0; i < 4; i++) {
      data.m[i] = *((float*)parr);
      parr += 4;
    }
  }

  void arrayToInt4(Vector<Int4>& data, 
    const uint8_t*& parr) {
    uint32_t size = *((uint32_t*)parr);
    parr += 4;
    data.capacity(size);
    data.resize(size);
    for (uint32_t i = 0; i < data.size(); i++) {
      arrayToInt4(data[i], parr);
    }
  }

  void arrayToInt4(Int4& data, const uint8_t*& parr) {
    for (uint32_t i = 0; i < 4; i++) {
      data[i] = *((int32_t*)parr);
      parr += 4;
    }
  }

  void arrayToUInt32(Vector<uint32_t>& data, 
    const uint8_t*& parr) {
    uint32_t size = *((uint32_t*)parr);
    parr += 4;
    data.capacity(size);
    data.resize(size);
    for (uint32_t i = 0; i < data.size(); i++) {
      arrayToUInt32(data[i], parr);
    }
  }

  void arrayToString(string& str, const uint8_t*& parr) {
    uint32_t size = *((uint32_t*)parr);
    parr += 4;
    char* name_c_str = (char*)(parr);
    if (name_c_str[size] != '\0') {
      throw wruntime_error("arrayToString() - ERROR: "
        "string from array is not null terminated!");
    }
    if (size > 0) {
      str = name_c_str;
    } else {
     }
    parr += size + 1;
  }

  void arrayToUInt32(uint32_t& val, const uint8_t*& parr) {
    val = *((uint32_t*)parr);
    parr += 4;
  }

  void arrayToMaterial(Material& val, 
    const uint8_t*& parr) {
    val.spec_power = *((float*)parr);
    parr += 4;
    val.spec_intensity = *((float*)parr);
    parr += 4;
    arrayToFloat3(val.albedo, parr);
  }

  void arrayToFloat4x4(math::Float4x4& mat, 
    const uint8_t*& parr) {
    for (uint32_t i = 0; i < 16; i++) {
      mat.m[i] = *((float*)parr);
      parr += 4;
    }
  }

}  // namespace file_io
}  // namespace jtil

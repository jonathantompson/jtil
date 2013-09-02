//
//  data_str_serialization.h
//
//  Created by Jonathan Tompson on 2/26/13.
//
//  These functions copy to and from an array into various data structures
//  

#pragma once

#include <string>
#include "jtil/math/math_types.h"

namespace jtil {

namespace data_str { template <typename T> class Vector; }
namespace renderer { struct Material; }

namespace file_io {
  // Helpers for various functions that need to save data to disk
  void float3ToArray(const data_str::Vector<math::Float3>& data,
    uint8_t*& parr);
  void float3ToArray(const math::Float3& data, uint8_t*& parr);
  void float2ToArray(const data_str::Vector<math::Float2>& data,
    uint8_t*& parr);
  void float2ToArray(const math::Float2& data, uint8_t*& parr);
  void float4ToArray(const data_str::Vector<math::Float4>& data,
    uint8_t*& parr);
  void float4ToArray(const math::Float4& data, uint8_t*& parr);
  void int4ToArray(const data_str::Vector<math::Int4>& data,
    uint8_t*& parr);
  void int4ToArray(const math::Int4& data, uint8_t*& parr);
  void uInt32ToArray(const data_str::Vector<uint32_t>& data, 
    uint8_t*& parr);
  void stringToArray(const std::string& str, uint8_t*& parr);
  void uInt32ToArray(const uint32_t val, uint8_t*& parr);
  void materialToArray(const renderer::Material& val, uint8_t*& parr);
  void float4x4ToArray(const math::Float4x4& mat, uint8_t*& parr);

  void arrayToFloat3(data_str::Vector<math::Float3>& data,
    const uint8_t*& parr);
  void arrayToFloat3(math::Float3& data, const uint8_t*& parr);
  void arrayToFloat2(data_str::Vector<math::Float2>& data,
    const uint8_t*& parr);
  void arrayToFloat2(math::Float2& data, const uint8_t*& parr);
  void arrayToFloat4(data_str::Vector<math::Float4>& data,
    const uint8_t*& parr);
  void arrayToFloat4(math::Float4& data, const uint8_t*& parr);
  void arrayToInt4(data_str::Vector<math::Int4>& data,
    const uint8_t*& parr);
  void arrayToInt4(math::Int4& data, const uint8_t*& parr);
  void arrayToUInt32(data_str::Vector<uint32_t>& data, 
    const uint8_t*& parr);
  void arrayToString(std::string& str, const uint8_t*& parr);
  void arrayToUInt32(uint32_t& val, const uint8_t*& parr);
  void arrayToMaterial(renderer::Material& val, const uint8_t*& parr);
  void arrayToFloat4x4(math::Float4x4& mat, const uint8_t*& parr);

};  // namespace file_io
};  // namespace jtil

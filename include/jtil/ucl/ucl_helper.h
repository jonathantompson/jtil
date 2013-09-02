//
//  ucl_healper.h
//
//  Created by Jonathan Tompson on 1/5/13.
//
//  Wrapper to make using UCL (which is a compression and 
//  decompression library) easier.  Compression ratios are OK, but it is able
//  to perform in-place decompression.
// 
//  Note: In-place decompression uses very little overhead memory however,
//  in-place compression uses quite a lot of memory (2x).
//
//  WARNING ALLOCATED ARRAY MUST BE LONGER THAN data_size_bytes. Use 
//  calcInPlaceDe/CompressSizeRequirement to determine required size.

#pragma once

#include <mutex>
#include "jtil/math/math_types.h"

namespace jtil {
namespace ucl {

  class UCLHelper {
  public:
    static uint32_t inPlaceCompress(uint8_t* data, 
      const uint32_t data_size_bytes, const uint32_t level);  // level 1-10
    static uint32_t inPlaceDecompress(uint8_t* data, 
      const uint32_t compressed_size, const uint32_t decompressed_size);
    static const uint32_t calcInPlaceCompressSizeRequirement(
      const uint32_t data_size_bytes);
    static const uint32_t calcInPlaceDecompressSizeRequirement(
      const uint32_t data_size_bytes);
    static const uint32_t calcInPlaceDecompressOffset(
      const uint32_t data_size_bytes);

  private:
    UCLHelper() { }
    ~UCLHelper() { }
    static bool init_called_;
    static std::mutex init_lock_;

    static const uint32_t calcCompressOverhead(const uint32_t size);
    static const uint32_t calcDecompressOverhead(const uint32_t size);
  };

};  // unnamed namespace
};  // namespace jtil

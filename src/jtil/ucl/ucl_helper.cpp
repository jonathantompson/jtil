#include <cstring>
#include <stdlib.h>
#include "jtil/ucl/ucl_helper.h"
#include "jtil/exceptions/wruntime_error.h"
#define UCL_USE_ASM
extern "C" {
  #include "jtil/ucl/ucl.h"
}

namespace jtil {
namespace ucl {

  bool UCLHelper::init_called_ = false;
  std::mutex UCLHelper::init_lock_;

  // inPlaceCompress: WARNING ALLOCATED ARRAY MUST BE LONGER THAN 
  //                  data_size_bytes. Use calcInPlaceCompressSizeRequirement
  //                  to determine required size.
  uint32_t UCLHelper::inPlaceCompress(uint8_t* data, 
    const uint32_t data_size_bytes, const uint32_t level) {
    if (level < 1 || level > 10) {
      throw std::wruntime_error("UCLHelper::inPlaceCompress() - ERROR: "
        "comrpression level is outside the valid range.");
    }

    // Some default parameters
    uint32_t offset = calcCompressOverhead(data_size_bytes);
    
    // Move the data from the beginning of the array with some offset (note
    // offset is longer than the size so memcpy here is safe, otherwise we 
    // would need to use memmove).
    memcpy(data + offset + data_size_bytes, data, data_size_bytes);

    init_lock_.lock();
    if (!init_called_) {
      if (ucl_init() != UCL_E_OK) {
        throw std::wruntime_error("internal error - ucl_init() failed!!!"
          "(this usually indicates a compiler bug)");
      }
      init_called_ = true;
    }
    init_lock_.unlock();

    ucl_uint new_len = 0;
    int r = ucl_nrv2b_99_compress(data + offset + data_size_bytes,
      data_size_bytes, data, &new_len, NULL, level, NULL, NULL);
    if (r != UCL_E_OK) {
      throw std::wruntime_error("overlapping compression failed");
    }

    return static_cast<uint32_t>(new_len);
  }

  // Temp
  uint32_t UCLHelper::inPlaceDecompress(uint8_t* data, 
    const uint32_t compressed_size, const uint32_t decompressed_size) {
    uint32_t block_size = 
      calcInPlaceDecompressSizeRequirement(decompressed_size);

    // Move data to the end of the memory chunk
    for (int src = compressed_size-1, dst = block_size-1; src >=0; 
      src--, dst--) {
      data[dst] = data[src];
    }

    init_lock_.lock();
    if (!init_called_) {
      if (ucl_init() != UCL_E_OK) {
        throw std::wruntime_error("internal error - ucl_init() failed!!!"
          "(this usually indicates a compiler bug)");
      }
      init_called_ = true;
    }
    init_lock_.unlock();

    ucl_uint new_len = decompressed_size;
    uint8_t* in = data + block_size - compressed_size;
    int r = ucl_nrv2b_decompress_safe_8(in, compressed_size, data, &new_len, 
      NULL);
    switch (r) {
    case UCL_E_OUTPUT_OVERRUN:
      throw std::wruntime_error("ucl_nrv2b_decompress_safe_8 returned "
        "UCL_E_OUTPUT_OVERRUN");
    case UCL_E_OUT_OF_MEMORY:
      throw std::wruntime_error("ucl_nrv2b_decompress_safe_8 returned "
        "UCL_E_OUT_OF_MEMORY");
    case UCL_E_OK:
      break;
    case UCL_E_LOOKBEHIND_OVERRUN:
      throw std::wruntime_error("ucl_nrv2b_decompress_safe_8 returned "
        "UCL_E_LOOKBEHIND_OVERRUN");
    default:
      throw std::wruntime_error("ucl_nrv2b_decompress_safe_8 returned "
        "an error.");
    }

    if (new_len != decompressed_size) {
      throw std::wruntime_error("UCLHelper::inPlaceDecompress - ERROR: "
        "decompressed size is not what we expected!");
    }

    return static_cast<uint32_t>(new_len);
  }

  const uint32_t UCLHelper::calcInPlaceCompressSizeRequirement(
    const uint32_t data_size_bytes) {
    const uint32_t overhead = calcCompressOverhead(data_size_bytes);
    return overhead + 2 * data_size_bytes;
  }

  const uint32_t UCLHelper::calcInPlaceDecompressSizeRequirement(
    const uint32_t data_size_bytes) {
    const uint32_t overhead = calcDecompressOverhead(data_size_bytes);
    return overhead + data_size_bytes;
  }

  const uint32_t UCLHelper::calcInPlaceDecompressOffset(
    const uint32_t data_size_bytes) {
    return calcDecompressOverhead(data_size_bytes);
  }

  const uint32_t UCLHelper::calcCompressOverhead(const uint32_t size) {
    return size / 8 + 256;
  }

  const uint32_t UCLHelper::calcDecompressOverhead(const uint32_t size) {
    return size / 8 + 256;
  }

}  // namespace minilzo
}  // namespace jtil
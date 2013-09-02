#include <sstream>
#include <string>
#include "jtil/file_io/file_io.h"
#include "jtil/math/math_types.h"  // for uint

namespace jtil {
namespace file_io {
  bool fileExists(const std::string& filename) {
    std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
    bool ret_val = false;
    if (file.is_open()) {
      ret_val = true;
      file.close();
    }
    return ret_val;
  }

}  // namespace file_io
}  // namespace jtil

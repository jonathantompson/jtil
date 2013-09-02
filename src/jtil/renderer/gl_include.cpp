#include <sstream>
#include <string>
#include "jtil/string_util/string_util.h"
#include "jtil/exceptions/wruntime_error.h"
#include "jtil/renderer/gl_include.h"
#include "jtil/renderer/gl_state.h"

namespace jtil {
namespace renderer {
  void CheckOpenGLError() {
    GLenum err = GLState::glsGetError();
    if (err != GL_NO_ERROR) {
      std::stringstream ss;
      uint32_t error_num = 0;
      ss << "Renderer::checkOpenGLError() - ERROR: " << error_num << ": '";
      ss << (const char*)(GLState::glsuErrorString(err));
      ss << "' (error code: " << (int)err << ") ";
      while((err = GLState::glsGetError()) != GL_NO_ERROR && error_num < 10) {
        error_num++;
        ss <<  error_num << ": '";
        ss << (const char*)(GLState::glsuErrorString(err));
        ss << "' (error code: " << (int)err << ")";
      }
      // Catch as many errors as we can...
      throw std::wruntime_error(ss.str());
    }
  }
}  // namespace renderer
}  // namespace jtil


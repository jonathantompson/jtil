#include <string>
#include <sstream>
#include "jtil/exceptions/wruntime_error.h"
#include "jtil/renderer/texture/texture_utils.h"
#include "jtil/renderer/gl_include.h"

namespace jtil {
namespace renderer {
  uint32_t ElementSizeOfGLType(const GLint gl_type) {
    GLint dummy_int;
    static_cast<void>(dummy_int);
    GLshort dummy_short;
    static_cast<void>(dummy_short);
    GLbyte dummy_byte;
    static_cast<void>(dummy_byte);
    GLulong dummy_long;
    static_cast<void>(dummy_long);
    GLfloat dummy_float;
    static_cast<void>(dummy_float);
    GLdouble dummy_double;
    static_cast<void>(dummy_double);
    GLboolean dummy_bool;
    static_cast<void>(dummy_bool);

    // This code is borrowed and adapted from:
    // https://github.com/sgothel/jogl/blob/master/src/jogl/classes/com/jogamp/opengl/util/GLBuffers.java
    switch (gl_type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case GL_UNSIGNED_BYTE_3_3_2:
    case GL_UNSIGNED_BYTE_2_3_3_REV:
      return static_cast<uint32_t>(sizeof(dummy_byte));

    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_5_6_5_REV:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
    case GL_HALF_FLOAT:
      return static_cast<uint32_t>(sizeof(dummy_short));

    case GL_FIXED:
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_UNSIGNED_INT_8_8_8_8:
    case GL_UNSIGNED_INT_8_8_8_8_REV:
    case GL_UNSIGNED_INT_10_10_10_2:
    case GL_UNSIGNED_INT_2_10_10_10_REV:                
    case GL_UNSIGNED_INT_24_8:
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
    case GL_UNSIGNED_INT_5_9_9_9_REV:
    case GL_HILO16_NV:
    case GL_SIGNED_HILO16_NV:
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_1D_ARRAY:
    case GL_SAMPLER_2D_ARRAY:
    case GL_INT_SAMPLER_2D:
      return static_cast<uint32_t>(sizeof(dummy_int));
    case GL_INT_VEC2:
    case GL_UNSIGNED_INT_VEC2:
      return 2 * static_cast<uint32_t>(sizeof(dummy_int));
    case GL_INT_VEC3:
    case GL_UNSIGNED_INT_VEC3:
      return 3 * static_cast<uint32_t>(sizeof(dummy_int));
    case GL_INT_VEC4:
    case GL_UNSIGNED_INT_VEC4:
      return 4 * static_cast<uint32_t>(sizeof(dummy_int));

    case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
      return static_cast<uint32_t>(sizeof(dummy_long));

    case GL_FLOAT:
      return static_cast<uint32_t>(sizeof(dummy_float));
    case GL_FLOAT_VEC2:
      return 2 * static_cast<uint32_t>(sizeof(dummy_float));
    case GL_FLOAT_VEC3:
      return 3 * static_cast<uint32_t>(sizeof(dummy_float));
    case GL_FLOAT_VEC4:
      return 4 * static_cast<uint32_t>(sizeof(dummy_float));

    case GL_FLOAT_MAT2:
      return 4 * static_cast<uint32_t>(sizeof(dummy_float));
    case GL_FLOAT_MAT3:
      return 9 * static_cast<uint32_t>(sizeof(dummy_float));
    case GL_FLOAT_MAT4:
      return 16 * static_cast<uint32_t>(sizeof(dummy_float));

    case GL_DOUBLE:
      return static_cast<uint32_t>(sizeof(dummy_double));

    case GL_BOOL:
      return static_cast<uint32_t>(sizeof(dummy_bool));

    }
    std::stringstream ss;
    ss << "ElementSizeOfGLType() - ERROR: type not recognized (or just not ";
    ss << "added yet): hex - "  << std::hex << gl_type << " dec - ";
    ss << std::dec << gl_type;
    throw std::wruntime_error(ss.str());
  }

  uint32_t NumElementsOfGLFormat(const GLint gl_format) {
    switch (gl_format) /* 26 */ {
    case GL_COLOR_INDEX:
    case GL_STENCIL_INDEX:
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_STENCIL:
    case GL_RED:
    case GL_RED_INTEGER:
    case GL_GREEN:
    case GL_GREEN_INTEGER:
    case GL_BLUE:
    case GL_BLUE_INTEGER:
    case GL_ALPHA:
    case GL_LUMINANCE:
      return 1;
    case GL_LUMINANCE_ALPHA:
    case GL_RG:
    case GL_RG_INTEGER:
    case GL_HILO_NV:
    case GL_SIGNED_HILO_NV:
      return 2;
    case GL_RGB:
    case GL_RGB_INTEGER:
    case GL_BGR:
    case GL_BGR_INTEGER: 
      return 3;
    case GL_RGBA:
    case GL_RGBA_INTEGER:
    case GL_BGRA:
    case GL_BGRA_INTEGER:
    case GL_ABGR_EXT:
      return 4;
    }
    throw std::wruntime_error("SizeOfGLType() - ERROR: Unrecognized format!");
  }

}  // namespace renderer
}  // namespace jtil

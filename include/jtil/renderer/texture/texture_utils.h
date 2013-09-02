//
//  texture_utils.h
//
//  Created by Jonathan Tompson on 10/7/12.
//
//  Just a few simple utility functions

#pragma once

#include "jtil/renderer/gl_include.h"
#include "jtil/math/math_types.h"

namespace jtil {
namespace renderer {
  uint32_t ElementSizeOfGLType(const GLint gl_type);
  uint32_t NumElementsOfGLFormat(const GLint gl_format);
};  // namespace renderer
};  // namespace jtil

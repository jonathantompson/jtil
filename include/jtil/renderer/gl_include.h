//
//  gl_include.h
//
//  Created by Jonathan Tompson on 5/29/12.
//

#pragma once

#ifndef GLEW_STATIC
  #define GLEW_STATIC
#endif
#include "jtil/glew/glew.h" 

#if defined(_DEBUG) || defined(DEBUG)
#define ERROR_CHECK renderer::CheckOpenGLError()
#else
#define ERROR_CHECK
#endif

// Shaders and VBOs need to agree on a position for the various common
// attribute locations - This is the central link between Geometry and Shaders
// (must be ordered 0->n-1)
#define VERTEX_POS_LOC 0
#define VERTEX_NOR_LOC 1
#define VERTEX_COL_LOC 2
#define VERTEX_TEX_COORD_LOC 3
#define VERTEX_BONEW_LOC 4
#define VERTEX_BONEI_LOC 5
#define VERTEX_TANGENT_LOC 6
#define VERTEX_RGB_TEX 7   // TEX are just for identifying vertex attributes 
#define VERTEX_BUMP_TEX 8  // (not shader locations)
#define VERTEX_DISP_TEX 9

#define VERTEX_POS_NAME "v_pos"
#define VERTEX_NOR_NAME "v_nor"
#define VERTEX_COL_NAME "v_col"
#define VERTEX_TEX_COORD_NAME "v_tex_coord"
#define VERTEX_BONEW_NAME "v_bonew"
#define VERTEX_BONEI_NAME "v_bonei"
#define VERTEX_TANGENT_NAME "v_tangent"

#define FRAGMENT_OUT0_LOC 0
#define FRAGMENT_OUT1_LOC 1
#define FRAGMENT_OUT2_LOC 2
#define FRAGMENT_OUT3_LOC 3

#define FRAGMENT_OUT0_NAME "f_out_0"
#define FRAGMENT_OUT1_NAME "f_out_1"
#define FRAGMENT_OUT2_NAME "f_out_2"
#define FRAGMENT_OUT3_NAME "f_out_3"

#define MAX_BONES 4  // Unfortunately this is difficult to change.  However,
                     // assimp will truncate and rescale models with high bone
                     // counts, so you shouldn't have to worry too much.

#define MAX_BONE_COUNT 64  // Max total bones in a model
                           // also defined in g_buffer_include.frag

namespace jtil {
namespace renderer {
  void CheckOpenGLError();
};  // namespace renderer
};  // namespace jtil

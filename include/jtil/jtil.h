//
//  jtil.h
//
//  Created by Jonathan Tompson on 2/23/13.
//  
//  Jonathan's Utilities (jtil)
//
//  Top level header file.  Includes most of the useful utilities.
//  Everything is in the jtil namespace!
//  

#pragma once

#include "jtil/math/math_types.h"  // Lots of default matrix and vector types
#include "jtil/math/noise.h"  // Generate continuously varying noisy keyframes
#include "jtil/clk/clk.h"  // A generic ns precision clock
#include "jtil/fastlz/fastlz_helper.h"  // Compression utility - Realtime
#include "jtil/ucl/ucl_helper.h"  // Compression utility - Offline
#include "jtil/settings/settings_manager.h"  // Settings utility framework
#include "jtil/string_util/string_util.h"  // Common string functions
#include "jtil/file_io/file_io.h"  // Some file io utility functions
#include "jtil/exceptions/wruntime_error.h"  // Breakpoint in DEBUG builds
#if defined( _WIN32 )
  #include "jtil/string_util/win32_debug_buffer.h"
#endif
#include "jtil/ui/ui.h"
#include "jtil/math/perlin_noise.h"

// Useful renderer classes
#include "jtil/renderer/renderer.h"  // Top level renderer interface
#include "jtil/renderer/camera/camera.h"
#include "jtil/renderer/colors/colors.h"
#include "jtil/renderer/texture/texture.h"  // Geometry creation
#include "jtil/renderer/geometry/geometry_manager.h"  // Geometry creation
#include "jtil/renderer/geometry/geometry_instance.h"  // Geometry
#include "jtil/renderer/geometry/bone.h"
#include "jtil/renderer/lights/lights.h"  // Point, Spot, Dir and SM

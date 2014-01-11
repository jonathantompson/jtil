//
//  geometry_render_pass.h
//
//  Created by Jonathan Tompson on 1/1/13.
//
//  Class to hold the state to draw all the geometry instances in the
//  geometry manager.  It means we don't need to have the same scene graph
//  render code in lots of places.
//

#pragma once

#include <string>
#include "jtil/math/math_types.h"
#include "jtil/renderer/geometry/geometry.h"  // For GeometryType

namespace jtil {
namespace renderer {
  namespace objects { class AABBox; }
  class LightSpot;
  class LightPoint;
  class Renderer;
  class Frustum;

  // The user can specify a callback that is called after loading each shader,
  // so that any shader specific uniforms can be set.

  typedef void (*ShaderUniformCBFunc)();
  
  class GeometryRenderPass {
  public:

    GeometryRenderPass(Renderer* renderer);
    ~GeometryRenderPass();

    void setShader(const VertexPrimative primative, 
      const GeometryType geom_type, const char* vshader, 
      const char* fshader, const char* gshader = NULL, 
      const char* tcsshader = NULL, const char* tesshader = NULL);

    void render();

    // Getters and setters
    bool& render_light_volumes() { return render_light_volumes_; }
    bool& render_light_sources() { return render_light_sources_; }
    bool& render_aabboxes() { return render_aabboxes_; }
    ShaderUniformCBFunc& shader_uniform_cb() { return shader_uniform_cb_; }
    const math::Float4x4*& view() { return view_; }
    const math::Float4x4*& proj() { return proj_; }
    const math::Float2*& screen_size() { return screen_size_; }
    const Frustum*& frustum() { return frustum_; }

  private:
    typedef enum {
      INDEX_NORM_COLR = 0,
      INDEX_NORM_COLR_BONED = 1,
      INDEX_NORM_CONST_COLR = 2,
      INDEX_NORM_CONST_COLR_BONED = 3,
      INDEX_NORM_TEXT = 4,
      INDEX_NORM_TEXT_BONED = 5,
      INDEX_NORM_TEXT_DISP = 6,
      NUM_GEOMETRY_TYPES_SUPORTED = 7,
    } GeometryTypeIndex;

    Renderer* renderer_;  // Not owned here
    ShaderUniformCBFunc shader_uniform_cb_;

    // Buffer of shaders to use for each of the geometry types
    data_str::VectorManaged<const char*> mesh_v_shaders_;
    data_str::VectorManaged<const char*> mesh_f_shaders_;
    data_str::VectorManaged<const char*> mesh_g_shaders_;
    data_str::VectorManaged<const char*> mesh_tcs_shaders_;  // tes. control shader
    data_str::VectorManaged<const char*> mesh_tes_shaders_;  // tes. eval. shader

    data_str::VectorManaged<const char*> points_v_shaders_;
    data_str::VectorManaged<const char*> points_f_shaders_;
    data_str::VectorManaged<const char*> points_g_shaders_;
    data_str::VectorManaged<const char*> points_tcs_shaders_;  // tes. control shader
    data_str::VectorManaged<const char*> points_tes_shaders_;  // tes. eval. shader

    bool render_aabboxes_;  // Render AABBox objects in wireframe
    bool render_light_volumes_;  // Render light volumes in wireframe
    bool render_light_sources_;  // Render light object representations

    // Camera class parameters used when rendering.  They must be set!
    const math::Float4x4* view_;  // Not owned here
    const math::Float4x4* proj_;  // Not owned here
    const math::Float2* screen_size_;
    const Frustum* frustum_;  // Not owned here

    // Temporary structures
    math::Float4x4 world_;
    math::Float4x4 vw_mat_;
    math::Float4x4 pvw_mat_;
    math::Float4x4 normal_mat_;

    void renderSpotLightObject(const LightSpot* light, 
      const bool motion_blur) const;
    void renderPointLightObject(const LightPoint* light, 
      const bool motion_blur) const;
    void renderAABBox(const objects::AABBox* box, const Geometry* cube);

    GeometryType getGeometryType(const GeometryTypeIndex index) const;
    GeometryTypeIndex getGeometryTypeIndex(const GeometryType type) const;

    // Non-copyable, non-assignable.
    GeometryRenderPass(GeometryRenderPass&);
    GeometryRenderPass& operator=(const GeometryRenderPass&);
  };
};  // namespace renderer
};  // namespace jtil


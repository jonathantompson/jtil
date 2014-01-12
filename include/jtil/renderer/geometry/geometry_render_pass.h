//
//  geometry_render_pass.h
//
//  Created by Jonathan Tompson on 1/1/13.
//
//  Class to hold the state to draw all the geometry instances in the
//  geometry manager.  It means we don't need to have the same scene graph
//  render code in lots of places.  Note: This is not a lightweight class, you
//  can have as many as you want however you should try and reuse instances
//  when possible.
//
//  TODO: This is the worst part of the renderer.  The high level concept is OK
//  but the implementation is horrible.  Think of a way to avoid the hard
//  coded incides that link the vector position with the geometry type
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
    void unsetShaders();  // Reset all shaders to NULL

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
    // TODO: Make this a hash map and reference it by strings.
    typedef enum {
      INDEX_VERT_POINTS = 0,
      INDEX_VERT_LINES = 1,
      INDEX_VERT_TRIANGLES = 2,
      INDEX_VERT_QUADS = 3,
      NUM_VERTEX_PRIMATIVES_SUPORTED = 4,
    } VertexPrimativeIndex;

    // TODO: Make this a hash map and reference it by strings.
    typedef enum {
      INDEX_COLR = 0,
      INDEX_NORM_COLR = 1,
      INDEX_NORM_COLR_BONED = 2,
      INDEX_NORM_CONST_COLR = 3,
      INDEX_NORM_CONST_COLR_BONED = 4,
      INDEX_NORM_TEXT = 5,
      INDEX_NORM_TEXT_BONED = 6,
      INDEX_NORM_TEXT_DISP = 7,
      NUM_GEOMETRY_TYPES_SUPORTED = 8,
    } GeometryTypeIndex;

    Renderer* renderer_;  // Not owned here
    ShaderUniformCBFunc shader_uniform_cb_;

    // Buffer of shaders to use for each of the geometry primatives and types
    // If the name of the shader is NULL, the geometry type (for that 
    // primative) will not be rendered
    data_str::VectorManaged<const char*> v_shaders_[NUM_VERTEX_PRIMATIVES_SUPORTED];
    data_str::VectorManaged<const char*> f_shaders_[NUM_VERTEX_PRIMATIVES_SUPORTED];
    data_str::VectorManaged<const char*> g_shaders_[NUM_VERTEX_PRIMATIVES_SUPORTED];
    // tes. control and eval shaders
    data_str::VectorManaged<const char*> tcs_shaders_[NUM_VERTEX_PRIMATIVES_SUPORTED];
    data_str::VectorManaged<const char*> tes_shaders_[NUM_VERTEX_PRIMATIVES_SUPORTED];

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
    VertexPrimative getVertexPrimative(const VertexPrimativeIndex index) const;
    VertexPrimativeIndex getVertexPrimativeIndex(const VertexPrimative type) const;

    const bool isShaderSet (const VertexPrimativeIndex p, 
      const GeometryTypeIndex g) const;

    // Non-copyable, non-assignable.
    GeometryRenderPass(GeometryRenderPass&);
    GeometryRenderPass& operator=(const GeometryRenderPass&);
  };
};  // namespace renderer
};  // namespace jtil


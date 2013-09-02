//
//  g_buffer.h
//
//  Created by Jonathan Tompson on 6/8/12.
//

#pragma once

#include "jtil/renderer/gl_include.h"
#include "jtil/data_str/vector.h"
#include "jtil/data_str/pair.h"

#define GBUFFER_SCENE_INTERNAL_FORMAT GL_RGBA16F
#define GBUFFER_SCENE_FORMAT GL_RGBA
#define GBUFFER_SCENE_TYPE GL_FLOAT

namespace jtil {
namespace renderer {

  class Renderer;
  class Shader;
  class ShaderProgram;
  class Texture;
  class TextureGBuffer;
  class TextureRenderable;
  class Geometry;
  class LightSpot;
  class LightPoint;
  class GeometryRenderPass;
  struct RendererGBufferHandles;

  class GBuffer {
  public:
    GBuffer(Renderer* renderer);
    ~GBuffer();

    void renderGBuffer();
    void visualizeGBuffer() const;
    void visualizeGBufferDepth() const;
    void visualizeGBufferNormal() const;
    void visualizeGBufferAlbedo() const;
    void visualizeGBufferViewPos() const;
    void visualizeGBufferVel() const;
    void visualizeGBufferLightingStencil() const;
    void visualizeGBufferLightingAccumDiff() const;
    void visualizeGBufferLightingAccumSpec() const;

    // void visualizeScreenNormalsToScene() const;
    static void visualizeNormalsUniformCB();

    inline TextureGBuffer* g_buffer_texture() const { 
      return g_buffer_texture_; }
    inline TextureRenderable* final_scene() const { return final_scene_; }

  private:
    Renderer* renderer_;

    GeometryRenderPass* render_pass_;

    // The render target and associated data structures
    TextureGBuffer* g_buffer_texture_;
    TextureRenderable* final_scene_;

    Geometry* single_pt_;

    void clearGBuffer(Texture* tex = NULL);

    void renderSpotLightObject(const LightSpot* light) const;
    void renderPointLightObject(const LightPoint* light) const;

    // Non-copyable, non-assignable.
    GBuffer(GBuffer&);
    GBuffer& operator=(const GBuffer&);
  };
};  // namespace renderer
};  // namespace jtil

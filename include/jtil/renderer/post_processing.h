//
//  post_processing.h
//
//  Created by Jonathan Tompson on 6/16/12.
//

#pragma once

#include "jtil/renderer/gl_include.h"
#include "jtil/data_str/vector.h"

#define FULLSCREEN_QUAD_V_SHADER "./shaders/post_processing/fullscreen_quad.vert"
#define DOF_NUM_TAPS 12

namespace jtil {
namespace renderer {

  typedef enum {
    AA_OFF,
    FXAA_LQ_FULL,
    FXAA_LQ_EDGE_AWARE,
    FXAA_HQ_FULL,
    FXAA_HQ_EDGE_AWARE,
  } FXAAType;

  class Renderer;
  class Shader;
  class ShaderProgram;
  class TextureRenderable;
  class TextureRenderableArray;
  class TextureGBuffer;
  class Texture;
  class Geometry;

  class PostProcessing {
  public:
    PostProcessing(Renderer* renderer);
    ~PostProcessing();

    void renderPostProcessing(const float dt);

    void renderFullscreenQuad(const TextureRenderable* tex, 
      const uint32_t texture_index = 0) const;
    void renderFullscreenQuad(Texture* tex);
    void renderFullscreenQuad(const GLuint h_texture) const;
    void renderFullscreenNull() const;  // To the currrent render target

    void copyTex(const TextureRenderable* dst, const TextureRenderable* src) 
      const;

    // Rectangular blur
    void rectBlur(const TextureRenderable* texture, 
      const TextureRenderable* temp_texture, const uint32_t blur_radius, 
      const uint32_t texture_index = 0) const;
    void rectBlur(const TextureRenderableArray* texture, 
      const TextureRenderableArray* temp_texture, const uint32_t blur_radius, 
      const uint32_t array_index = 0, const uint32_t texture_index = 0) const;

    inline TextureRenderable* luma(const uint32_t i) const { return luma_[i]; }
    inline Geometry* quad() const { return quad_; }

  private:
    Renderer* renderer_;

    // Quad vertex buffer (and it's shader program) used by a few classes
    Geometry* quad_;

    // Temporary render textures
    TextureRenderable* rgba_ubyte_texture_;
    TextureRenderable* rgba_16f_texture_;

    // Single float luminence texture --> Array of them, to do overall luminence
    TextureRenderable** luma_;
    uint32_t nluma_textures_;

    float dof_disk_offs[DOF_NUM_TAPS * 2];  // 12 taps of vec3
    bool dof_disk_offs_synced_;

    void renderDOF(TextureRenderable* dst, TextureRenderable* src);
    void renderMotionBlur(TextureRenderable* dst, TextureRenderable* src, 
      const float dt);
    void renderFXAA(TextureRenderable* dst, TextureRenderable* src,
      int aa_type);
    void renderSMAA(TextureRenderable* dst, TextureRenderable* src,
      int aa_type);
    void syncDOFDiskOffsets();
    void useFullscreenQuadSP(const GLint format) const;
    void calculateLuma(TextureRenderable** dst_l, TextureRenderable* src_rgb) 
      const;

    // Non-copyable, non-assignable.
    PostProcessing(PostProcessing&);
    PostProcessing& operator=(const PostProcessing&);
  };
};  // namespace renderer
};  // namespace jtil

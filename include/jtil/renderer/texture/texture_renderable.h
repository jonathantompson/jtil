//
//  texture_renderable.h
//
//  Created by Jonathan Tompson on 6/8/12.
//
//  Note: the framebuffer may have many textures (multiple render targets),
//  num_textures controls the number of renderable textures.

#pragma once

#include <mutex>
#include <string>
#include "jtil/math/math_types.h"
#include "jtil/renderer/gl_include.h"
#include "jtil/renderer/texture/texture_base.h"
#include "jtil/renderer/texture/texture.h"  // For TextureFilterMode

namespace jtil {
namespace renderer {
  class TextureRenderable : public TextureBase {
  public:
    // Load texture from file:
    TextureRenderable(const GLint internal_format, 
      const int w, const int h, const GLint format, const GLint type, 
      const uint32_t num_textures, const bool create_depth_texture,
      const TextureFilterMode filter = TEXTURE_NEAREST);
    virtual ~TextureRenderable();

    virtual inline TextureType type() { return TEXTURE_RENDERABLE_TYPE; }
    virtual inline int w() const { return (int)screen_size_[0]; }
    virtual inline int h() const { return (int)screen_size_[1]; }

    // attachSharedDepthTexture - To share accross FBOs, supply an external
    // depth texture.  Ownership is not transferred and the callee must release
    // the openGL resources at shutdown.
    void attachSharedDepthTexture(const GLint depth_texture);

    void begin() const ;  // begin rendering
    void end() const ;  // end rendering

    // bind() - typical usage for single texture render target: 
    //          bind(0, GL_TEXTURE0, "f_texture_sampler")
    void bind(const uint32_t texture_index, const GLenum target_id, 
      const char* h_texture_sampler) const ;

    GLuint texture(const uint32_t i) const ;
    inline const math::Float2& screen_size() const { return screen_size_; }
    inline GLint format() const { return format_; }
    inline TextureFilterMode filter() const { return filter_; }

    // true -> formats match
    bool compareFormat(const TextureRenderable* texture) const;  

  private:
    GLuint* textures_;
    GLuint depth_texture_;
    bool shared_depth_texture_;
    GLuint fbo_;
    GLint internal_format_; 
    math::Float2 screen_size_;
    GLint format_; 
    GLint type_; 
    uint32_t num_textures_;
    TextureFilterMode filter_;
    GLenum* draw_buffers_;

    // Non-copyable, non-assignable.
    TextureRenderable(TextureRenderable&);
    TextureRenderable& operator=(const TextureRenderable&);
  };
};  // namespace renderer
};  // namespace jtil


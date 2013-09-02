//
//  texture_renderable_array.h
//
//  Created by Jonathan Tompson on 12/30/12.
//
//  A class for implementing a texture array.  Used on OpenGL 3.0 hardware to
//  replace a texture atlas.  NOTE: REQUIRES SUPPORT FOR EXT_texture_array.
//  Use "GLState::queryGLExtension("EXT_texture_array")" to query support.
//

#pragma once

#include <mutex>
#include <string>
#include "jtil/math/math_types.h"
#include "jtil/renderer/gl_include.h"
#include "jtil/renderer/texture/texture_base.h"

namespace jtil {
namespace renderer {
  class TextureRenderableArray : public TextureBase {
  public:
    // Load texture from file:
    TextureRenderableArray(const GLint internal_format, 
      const int w, const int h, const GLint format, const GLint type, 
      const uint32_t num_texture_arrays, const uint32_t array_size, 
      const bool create_depth_texture);
    virtual ~TextureRenderableArray();

    virtual inline TextureType type() { return TEXTURE_RENDERABLE_ARRAY_TYPE; }
    virtual inline int w() const { return (int)screen_size_[0]; }
    virtual inline int h() const { return (int)screen_size_[1]; }

    // attachSharedDepthTexture - To share accross FBOs, supply an external
    // depth texture.  Ownership is not transferred and the callee must release
    // the openGL resources at shutdown.
    void attachSharedDepthTexture(const GLint depth_texture);

    void begin(const uint32_t array_index) const ;  // begin rendering
    void end() const ;  // end rendering

    // bind() - typical usage for single texture render target: 
    //          bind(texture_ind, GL_TEXTURE0, "f_texture_sampler")
    void bind(const uint32_t texture_index, const GLenum target_id, 
      const char* h_texture_sampler) const ;

    GLuint texture(const uint32_t i) const ;
    inline const math::Float2& screen_size() const { return screen_size_; }
    inline GLint format() const { return format_; }

    bool compareFormat(const TextureRenderableArray* texture) const;

  private:
    GLuint* textures_;
    GLuint depth_texture_;
    bool shared_depth_texture_;
    GLuint fbo_;
    GLint internal_format_; 
    math::Float2 screen_size_;
    GLint format_; 
    GLint type_; 
    uint32_t array_size_;
    uint32_t num_textures_;
    GLenum* draw_buffers_;

    // Non-copyable, non-assignable.
    TextureRenderableArray(TextureRenderableArray&);
    TextureRenderableArray& operator=(const TextureRenderableArray&);
  };
};  // namespace renderer
};  // namespace jtil

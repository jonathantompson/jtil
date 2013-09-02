#include <string>
#include <sstream>
#include "jtil/renderer/texture/texture_renderable_array.h"
#include "jtil/exceptions/wruntime_error.h"
#include "jtil/string_util/string_util.h"
#include "jtil/renderer/renderer.h"
#include "jtil/renderer/gl_state.h"
#include "jtil/renderer/shader/shader_program.h"

using std::wstring;
using std::wruntime_error;

namespace jtil {
namespace renderer {

  // Load a texture from disk
  TextureRenderableArray::TextureRenderableArray(const GLint internal_format, 
    const int w, const int h, const GLint format, const GLint type, 
    const uint32_t num_texture_arrays, const uint32_t array_size, 
    const bool create_depth_texture) {
    internal_format_ = internal_format; 
    screen_size_.set((float)w, (float)h);
    format_ = format;
    type_ = type;
    num_textures_ = num_texture_arrays;
    array_size_ = array_size;
    shared_depth_texture_ = false;

#if defined(_DEBUG) || defined(DEBUG)
    if (num_textures_ == 0) {
      throw wruntime_error(L"TextureRenderableArray::TextureRenderableArray()"
        L" - ERROR: num_textures should not be zero");
    }
#endif

    // Create the FBO
    GLState::glsGenFramebuffers(1, &fbo_); 
    GLState::glsBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_);

    // Create the openGL texture IDs
    textures_ = new GLuint[num_textures_];
    GLState::glsGenTextures(num_textures_, textures_);

    // Create the openGL textures
    for (uint32_t i = 0; i < num_textures_; i ++) {
      GLState::glsBindTexture(GL_TEXTURE_2D_ARRAY, textures_[i]);
      GLState::glsTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internal_format_ , w, h, 
        array_size_, 0, format_, type_, NULL);
      // No need to bind it, we bind them individually later when we call bind
      GLState::glsFramebufferTexture(GL_DRAW_FRAMEBUFFER, 
        GL_COLOR_ATTACHMENT0 + i, textures_[i], 0);

      // No filtering for the render textures
      GLState::glsTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, 
        GL_NEAREST);
      GLState::glsTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, 
        GL_NEAREST); 
      GLState::glsTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, 
        GL_CLAMP_TO_EDGE);
      GLState::glsTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, 
        GL_CLAMP_TO_EDGE);
      GLfloat border[] = {0.0f, 0.0f, 0.0f, 0.0f};
      GLState::glsTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, 
        border);
    }

    if (!create_depth_texture) {
      depth_texture_ = 0;
    } else {
      GLState::glsGenTextures(1, &depth_texture_);
      GLState::glsBindTexture(GL_TEXTURE_2D_ARRAY, depth_texture_);
      GLState::glsTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, 
        w, h, array_size_, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
      GLState::glsTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER,
        GL_NEAREST);
      GLState::glsTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, 
        GL_NEAREST); 
      GLState::glsTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, 
        GL_CLAMP_TO_EDGE);
      GLState::glsTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, 
        GL_CLAMP_TO_EDGE);
      GLState::glsFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
        depth_texture_, 0);
    }

    draw_buffers_ = new GLenum[num_textures_];
    for (uint32_t i = 0; i < num_textures_; i ++) {
      draw_buffers_[i] = GL_COLOR_ATTACHMENT0 + i;
    }
    GLState::glsDrawBuffers(num_textures_, draw_buffers_);
    GLState::glsCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

    // restore default FBO so no one accidently makes changes to ours
    GLState::glsBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  }

  TextureRenderableArray::~TextureRenderableArray() {
    if (fbo_ != 0) {
        GLState::glsDeleteFramebuffers(1, &fbo_);
    }
    for (uint32_t i = 0; i < num_textures_; i ++) {
      GLState::glsDeleteTextures(1, &textures_[i]);
    }
    delete[] textures_;
    if (depth_texture_ != 0 && !shared_depth_texture_) {
      GLState::glsDeleteTextures(1, &depth_texture_);
    }
    if (draw_buffers_) {
      delete[] draw_buffers_;
      draw_buffers_ = NULL;
    }
  }

  GLuint TextureRenderableArray::texture(const uint32_t i) const {
#if defined(_DEBUG) || defined(DEBUG)
    if (i > num_textures_) {
      throw wruntime_error(L"TextureRenderableArray::texture() - i out of"
        L"bounds");
    }
#endif
    return textures_[i];
  }

  void TextureRenderableArray::begin(const uint32_t array_index) const {
    if (array_index >= array_size_) {
      throw wruntime_error("TextureRenderableArray::begin() - ERROR: "
        "array_index is out of bounds!");
    }
    GLState::glsBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_);
    for (uint32_t i = 0; i < num_textures_; i++) {
      GLState::glsFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, 
        GL_COLOR_ATTACHMENT0 + i, textures_[i], 0, array_index);
    }
    if (depth_texture_) {
      GLState::glsFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, 
        GL_DEPTH_ATTACHMENT, depth_texture_, 0, array_index);
    }
    GLState::glsDrawBuffers(num_textures_, draw_buffers_);
    GLState::glsViewport(0, 0, (GLsizei)screen_size_[0], 
      (GLsizei)screen_size_[1]);
  }

  void TextureRenderableArray::end() const {
  }

  // bind() - typical usage for single texture render target: 
  //          bind(0, GL_TEXTURE0, tex_sampler_id)
  void TextureRenderableArray::bind(const uint32_t texture_index,
    const GLenum target_id, const char* h_texture_sampler) const {
    if (!GLState::glsQueryIfTextureIsBound(GL_TEXTURE_2D_ARRAY, target_id, 
      textures_[texture_index])) {
      GLState::glsActiveTexture(target_id);
      GLState::glsBindTexture(GL_TEXTURE_2D_ARRAY, textures_[texture_index]);
    }
    int uniform_val = target_id - GL_TEXTURE0;
    BIND_UNIFORM(h_texture_sampler, &uniform_val);
  }

  void TextureRenderableArray::attachSharedDepthTexture(
    const GLint depth_texture) {
    shared_depth_texture_ = true;
    depth_texture_ = depth_texture;

    GLState::glsBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_);
    GLState::glsBindTexture(GL_TEXTURE_2D_ARRAY, depth_texture_);
    GLState::glsFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
      depth_texture_, 0);

    GLState::glsCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
  }

  bool TextureRenderableArray::compareFormat(
    const TextureRenderableArray* texture) const {
    return (this->format_ == texture->format_) &&
           (this->internal_format_ == texture->internal_format_) &&
           (math::Float2::equal(this->screen_size_, texture->screen_size_)) &&
           (this->type_ == texture->type_) &&
           (this->array_size_ == texture->array_size_);
  }

}  // namespace renderer
}  // namespace jtil
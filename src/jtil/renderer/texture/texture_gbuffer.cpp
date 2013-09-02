#include <string>
#include <sstream>
#include <iostream>
#include "jtil/renderer/texture/texture_gbuffer.h"
#include "jtil/renderer/gl_include.h"
#include "jtil/renderer/shader/shader_program.h"
#include "jtil/exceptions/wruntime_error.h"
#include "jtil/renderer/renderer.h"
#include "jtil/windowing/glfw.h"
#include "jtil/settings/settings_manager.h"
#include "jtil/string_util/string_util.h"
#include "jtil/renderer/gl_state.h"

using std::wstring;
using std::wruntime_error;

namespace jtil {

namespace renderer {

  // Load a texture from disk
  TextureGBuffer::TextureGBuffer(const int w, 
    const int h) {
    screen_size_.set((float)w, (float)h);

    // Create the FBO
    GLState::glsGenFramebuffers(1, &fbo_); 
    GLState::glsBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_);

    // Create the openGL texture IDs
    textures_ = new GLuint[GBUFFER_NUM_CHANNELS];
    GLState::glsGenTextures(GBUFFER_NUM_CHANNELS, textures_);

    for (uint32_t i = 0; i < GBUFFER_NUM_CHANNELS; i++) {
      GLState::glsBindTexture(GL_TEXTURE_2D, textures_[i]);
      GLState::glsTexImage2D(GL_TEXTURE_2D, 0, GBUFFER_INTERNAL_FORMAT, w, h,
        0, GBUFFER_FORMAT, GBUFFER_TYPE, NULL);
      GLState::glsTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
        GL_NEAREST);
      GLState::glsTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
        GL_NEAREST);
      GLState::glsFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, 
        GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, textures_[i], 0);
    }

    // final --> Accumulation texture
    GLState::glsGenTextures(1, &lighting_accum_texture_);
    GLState::glsBindTexture(GL_TEXTURE_2D, lighting_accum_texture_);
    GLState::glsTexImage2D(GL_TEXTURE_2D, 0, GBUFFER_LIGHTING_INTERNAL_FORMAT, 
      w, h, 0, GBUFFER_LIGHTING_FORMAT, GBUFFER_LIGHTING_TYPE, NULL);
    GLState::glsTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
      GL_NEAREST);
    GLState::glsTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
      GL_NEAREST);
    GLState::glsFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, (GL_COLOR_ATTACHMENT0
      + GBUFFER_NUM_CHANNELS), GL_TEXTURE_2D, lighting_accum_texture_, 0);

    // depth
    bool light_pass_stencil_opt;
    GET_SETTING("light_pass_stencil_opt", bool, light_pass_stencil_opt);
    GLState::glsGenTextures(1, &depth_texture_);
    GLState::glsBindTexture(GL_TEXTURE_2D, depth_texture_);
    if (light_pass_stencil_opt) {
      GLState::glsTexImage2D(GL_TEXTURE_2D, 0, GBUFFER_DEPTH_FORMAT_STENCIL, 
        w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
      GLenum err = GLState::glsGetError();
      if (err != GL_NO_ERROR) {
        std::cout << "WARNING: depth + stencil format not supported.";
        std::cout << "  Turning off stencil optimizations." << std::endl;
        light_pass_stencil_opt = false;
        SET_SETTING("light_pass_stencil_opt", bool, light_pass_stencil_opt);
      } else {
        GLState::glsFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, 
          GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth_texture_, 0);
      }
    }
    if (!light_pass_stencil_opt) {
      GLState::glsTexImage2D(GL_TEXTURE_2D, 0, GBUFFER_DEPTH_FORMAT, w, h, 
        0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
      GLState::glsFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, 
        GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture_, 0);
    }

    GLState::glsCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    
    // restore default FBO so no one accidently makes changes to ours
    GLState::glsBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  }

  TextureGBuffer::~TextureGBuffer() {
    if (fbo_ != 0) {
      GLState::glsDeleteFramebuffers(1, &fbo_);
    }
    if (textures_) {
      for (uint32_t i = 0; i < GBUFFER_NUM_CHANNELS; i ++) {
        if (textures_[i] != 0) {
          GLState::glsDeleteTextures(1, &textures_[i]);
        }
      }
    }
    delete[] textures_;
    if (depth_texture_ != 0) {
      GLState::glsDeleteTextures(1, &depth_texture_);
    }
    if (lighting_accum_texture_ != 0) {
      GLState::glsDeleteTextures(1, &lighting_accum_texture_);
    }
  }

  void TextureGBuffer::beginGeometryPass() const {
    GLState::glsBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_);
    GLState::glsViewport(0, 0, (int)screen_size_[0], (int)screen_size_[1]);
    GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
      GL_COLOR_ATTACHMENT2 };
    GLState::glsDrawBuffers(GBUFFER_NUM_CHANNELS, DrawBuffers);
    // Disable writes to the depth buffer
    GLState::glsDepthMask(GL_TRUE);
  }

  void TextureGBuffer::endGeometryPass() const {
    // Disable writes to the depth buffer
    GLState::glsDepthMask(GL_FALSE);
  }

  void TextureGBuffer::beginLightStencilPass() const {
    GLState::glsBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_);
    GLState::glsViewport(0, 0, (int)screen_size_[0], (int)screen_size_[1]);
    GLenum DrawBuffers[] = {GL_NONE};
    GLState::glsDrawBuffers(1, DrawBuffers);
  }

  void TextureGBuffer::endLightStencilPass() const {

  }

  void TextureGBuffer::beginLightAccumPass() const {
    GLState::glsBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_);
    GLenum DrawBuffers[] = {GL_COLOR_ATTACHMENT0 + GBUFFER_NUM_CHANNELS};
    GLState::glsDrawBuffers(1, DrawBuffers);
    GLState::glsViewport(0, 0, (int)screen_size_[0], (int)screen_size_[1]);

    // Just bind the textures
    bindDepthNormalViewTex(GL_TEXTURE0, "f_depth_normal_view_stencil");
    bindAlbedoSpecIntensityTex(GL_TEXTURE1, "f_albedo_spec_intensity");
    bindSpecPowerVel(GL_TEXTURE2, "f_spec_power_vel");
  }

  void TextureGBuffer::endLightAccumPass() const {
  }

  void TextureGBuffer::beginAmbientOcclusionPass() const {
    bindDepthNormalViewTex(GL_TEXTURE0, "f_depth_normal_view_stencil");
  }

  void TextureGBuffer::endAmbientOcclusionPass() const {
  }

  void TextureGBuffer::beginLightFinalPass() const {
    bindAlbedoSpecIntensityTex(GL_TEXTURE0, "f_albedo_spec_intensity");
    bindLightAccumulationTex(GL_TEXTURE1, "f_accumulation_tex");
  }

  void TextureGBuffer::endLightFinalPass() const {
  }

  void TextureGBuffer::clearLightAccumTexture() const {
    GLState::glsBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_);
    GLenum DrawBuffers[] = {GL_COLOR_ATTACHMENT0 + GBUFFER_NUM_CHANNELS};
    GLState::glsDrawBuffers(1, DrawBuffers);
    GLState::glsClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    GLState::glsClear(GL_COLOR_BUFFER_BIT);
  }

  void TextureGBuffer::bindInternal(const uint32_t texture_index, 
    const GLenum target_id, const char* h_texture_sampler) const {
    if (!GLState::glsQueryIfTextureIsBound(GL_TEXTURE_2D, target_id, 
      textures_[texture_index])) {
      GLState::glsActiveTexture(target_id);
      if (texture_index == GBUFFER_LIGHT_ACCUMULATION_INDEX) {
        GLState::glsBindTexture(GL_TEXTURE_2D, lighting_accum_texture_);
      } else {
        GLState::glsBindTexture(GL_TEXTURE_2D, textures_[texture_index]);
      }
    }
    int uniform_val = target_id - GL_TEXTURE0;
    BIND_UNIFORM(h_texture_sampler, &uniform_val);
  }

  void TextureGBuffer::bindDepthNormalViewTex(const GLenum target_id, 
    const char* h_texture_sampler) const {
    bindInternal(FRAGMENT_OUT0_LOC, target_id, h_texture_sampler);
  }

  void TextureGBuffer::bindAlbedoSpecIntensityTex(const GLenum target_id, 
    const char* h_texture_sampler) const {
    bindInternal(FRAGMENT_OUT1_LOC, target_id, h_texture_sampler);
  }

  void TextureGBuffer::bindSpecPowerVel(const GLenum target_id, 
    const char* h_texture_sampler) const {
    bindInternal(FRAGMENT_OUT2_LOC, target_id, h_texture_sampler);
  }

  void TextureGBuffer::bindLightAccumulationTex(const GLenum target_id, 
    const char* h_texture_sampler) const {
    bindInternal(GBUFFER_LIGHT_ACCUMULATION_INDEX, target_id, 
      h_texture_sampler);
  }

}  // namespace renderer
}  // namespace jtil

//
//  texture_gbuffer.h
//
//  Created by Jonathan Tompson on 6/8/12.
//
//  There are a few odd difference between the gbuffer texture and a regular
//  renderable texture.  For instance, the light accumulation buffer needs
//  to exist in this FBO as well in order to use the depth buffer.
//  
//  The following is the GBuffer structure in the GLSL code:
//  struct GBufferOutput {
//    vec4 depth_normal_view_stencil;  // View space depth + normal and lighting stencil
//    vec4 albedo_spec_intensity;  // albedo color and specular intensity
//    vec4 spec_power_vel;  // shadow map split, spec power, velocity
//  };
//  
//  Also attached to this frame buffer is the light accumulation texture.

#pragma once

#include <mutex>
#include <string>
#include "jtil/renderer/gl_include.h"
#include "jtil/renderer/texture/texture_base.h"
#include "jtil/math/math_types.h"

#define GBUFFER_NUM_CHANNELS 3
#define GBUFFER_INTERNAL_FORMAT GL_RGBA16F
#define GBUFFER_FORMAT GL_RGBA
#define GBUFFER_TYPE GL_FLOAT
#define GBUFFER_DEPTH_FORMAT_STENCIL GL_DEPTH24_STENCIL8
#define GBUFFER_DEPTH_FORMAT GL_DEPTH_COMPONENT24
#define GBUFFER_LIGHTING_INTERNAL_FORMAT GL_RGBA16F
#define GBUFFER_LIGHTING_FORMAT GL_RGBA
#define GBUFFER_LIGHTING_TYPE GL_FLOAT
#define GBUFFER_LIGHT_ACCUMULATION_INDEX 3

namespace jtil {
namespace renderer {

  typedef enum {
    DepthNormalViewInd = 0,
    AlbedoSpecIntensityInd = 1,
    SpecPowerVel = 2,
  } TextureGBufferIndices;

  class ShaderProgram;

  class TextureGBuffer : public TextureBase {
  public:
    // Load texture from file:
    TextureGBuffer(const int width, 
      const int height);
    virtual ~TextureGBuffer();

    virtual inline TextureType type() { return TEXTURE_GBUFFER_TYPE; }
    virtual inline int w() const { return (int)screen_size_[0]; }
    virtual inline int h() const { return (int)screen_size_[1]; }

    void bindDepthNormalViewTex(const GLenum target_id, 
      const char* h_texture_sampler) const;
    void bindAlbedoSpecIntensityTex(const GLenum target_id, 
      const char* h_texture_sampler) const;
    void bindSpecPowerVel(const GLenum target_id, 
      const char* h_texture_sampler) const;
    void bindLightAccumulationTex(const GLenum target_id, 
      const char* h_texture_sampler) const;

    void beginGeometryPass() const;
    void beginLightStencilPass() const;
    void beginLightAccumPass() const;
    void beginLightFinalPass() const;
    void beginAmbientOcclusionPass() const;

    void endGeometryPass() const;
    void endLightStencilPass() const;
    void endLightAccumPass() const;
    void endLightFinalPass() const;
    void endAmbientOcclusionPass() const;

    void clearLightAccumTexture() const;

    inline const math::Float2& screen_size() const { return screen_size_; }
    inline GLint depth_texture() const { return depth_texture_; }

    inline GLuint lighting_accum_texture() const { 
      return lighting_accum_texture_; }

  private:
    GLuint* textures_;
    GLuint depth_texture_;
    GLuint lighting_accum_texture_;
    GLuint fbo_;
    math::Float2 screen_size_;

    void bindInternal(const uint32_t texture_index, const GLenum target_id, 
      const char* h_texture_sampler) const;

    // Non-copyable, non-assignable.
    TextureGBuffer(const TextureGBuffer&);
    TextureGBuffer& operator=(const TextureGBuffer&);
  };
};  // namespace renderer
};  // namespace jtil

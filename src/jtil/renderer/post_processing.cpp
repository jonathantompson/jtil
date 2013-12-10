#include <sstream>
#include "jtil/renderer/post_processing.h"
#include "jtil/renderer/g_buffer.h"
#include "jtil/renderer/lighting.h"
#include "jtil/renderer/renderer.h"
#include "jtil/renderer/camera/camera.h"
#include "jtil/renderer/geometry/geometry.h"
#include "jtil/renderer/geometry/geometry_manager.h"
#include "jtil/renderer/shader/shader.h"
#include "jtil/renderer/shader/shader_program.h"
#include "jtil/renderer/texture/texture_renderable.h"
#include "jtil/renderer/texture/texture_renderable_array.h"
#include "jtil/renderer/texture/texture_gbuffer.h"
#include "jtil/renderer/texture/texture.h"
#include "jtil/renderer/texture/texture_utils.h"
#include "jtil/settings/settings_manager.h"
#include "jtil/renderer/gl_state.h"

#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }

#define FULLSCREEN_QUAD_F_SHADER "./shaders/post_processing/fullscreen_quad.frag"
#define FULLSCREEN_NULL_F_SHADER "./shaders/post_processing/fullscreen_null.frag"
#define FULLSCREEN_ONES_F_SHADER "./shaders/post_processing/fullscreen_ones.frag"
#define RECT_BLUR_SHADER "./shaders/post_processing/post_processing_rect_blur.frag"
#define RECT_BLUR_SHADER_ARRAY "./shaders/post_processing/post_processing_rect_blur_array.frag"
#define DOF_V_SHADER "./shaders/post_processing/post_processing_dof.vert"
#define DOF_F_SHADER "./shaders/post_processing/post_processing_dof.frag"
#define FXAA_LQ_F_SHADER "./shaders/post_processing/post_processing_fxaa_lq.frag"
#define FXAA_LQ_EDGE_AWARE_F_SHADER "./shaders/post_processing/post_processing_fxaa_lq_edge_aware.frag"
#define FXAA_HQ_F_SHADER "./shaders/post_processing/post_processing_fxaa_hq.frag"
#define FXAA_HQ_EDGE_AWARE_F_SHADER "./shaders/post_processing/post_processing_fxaa_hq_edge_aware.frag"
#define MOTION_BLUR_F_SHADER "./shaders/post_processing/post_processing_motion_blur.frag"
#define LUMA_F_SHADER "./shaders/post_processing/post_processing_luma.frag"

#define FULLSCREEN_RGBA_QUAD_F_SHADER FULLSCREEN_QUAD_F_SHADER
#define FULLSCREEN_RGB_QUAD_F_SHADER "./shaders/post_processing/fullscreen_rgb_quad.frag"
#define FULLSCREEN_RG_QUAD_F_SHADER "./shaders/post_processing/fullscreen_rg_quad.frag"
#define FULLSCREEN_R_QUAD_F_SHADER "./shaders/post_processing/fullscreen_r_quad.frag"

using std::wstring;
using std::wruntime_error;

namespace jtil {

using math::Float4x4;

namespace renderer {

  PostProcessing::PostProcessing(Renderer* renderer) {
    renderer_ = renderer;
    quad_ = NULL;

    // Fullscreen quad vertices
    quad_ = renderer_->geometry_manager()->makeQuadGeometry("PostProcessingQuad");

    // Temporary textures
    rgba_ubyte_texture_ = new TextureRenderable(GL_RGBA, renderer_->width(), 
      renderer_->height(), GL_RGBA, GL_UNSIGNED_BYTE, 1, false);

    rgba_16f_texture_ = new TextureRenderable(GL_RGBA16F, renderer_->width(), 
      renderer_->height(), GL_RGBA, GL_FLOAT, 1, false, TEXTURE_LINEAR);

    // Single texture for now
    nluma_textures_ = 1;
    luma_ = new TextureRenderable*[nluma_textures_];
    luma_[0] = new TextureRenderable(GL_R16F, renderer_->width(), 
      renderer_->height(), GL_RED, GL_FLOAT, 1, false, TEXTURE_LINEAR);

    // Scale blur tap offsets based on render target size
    dof_disk_offs_synced_ = false;
  }

  void PostProcessing::syncDOFDiskOffsets() {
    if (!dof_disk_offs_synced_) {
      float dx = 0.5f / (float)renderer_->width(); 
      float dy = 0.5f / (float)renderer_->height();
      dof_disk_offs[0] = -0.326212f * dx; 
      dof_disk_offs[1] = -0.40581f * dy;
      dof_disk_offs[2] = -0.840144f * dx; 
      dof_disk_offs[3] = -0.07358f * dy;
      dof_disk_offs[4] = -0.840144f * dx; 
      dof_disk_offs[5] = 0.457137f * dy;
      dof_disk_offs[6] = -0.203345f * dx; 
      dof_disk_offs[7] = 0.620716f * dy;
      dof_disk_offs[8] = 0.96234f * dx;  
      dof_disk_offs[9] = -0.194983f * dy;
      dof_disk_offs[10] = 0.473434f * dx; 
      dof_disk_offs[11] = -0.480026f * dy;
      dof_disk_offs[12] = 0.519456f * dx; 
      dof_disk_offs[13] = 0.767022f * dy;
      dof_disk_offs[14] = 0.185461f * dx; 
      dof_disk_offs[15] = -0.893124f * dy;
      dof_disk_offs[16] = 0.507431f * dx; 
      dof_disk_offs[17] = 0.064425f * dy;
      dof_disk_offs[18] = 0.89642f * dx;  
      dof_disk_offs[19] = 0.412458f * dy;
      dof_disk_offs[20] = -0.32194f * dx;  
      dof_disk_offs[21] = -0.932615f * dy;
      dof_disk_offs[22] = -0.791559f * dx; 
      dof_disk_offs[23] = -0.59771f * dy;
      dof_disk_offs_synced_ = true;
      BIND_UNIFORM("f_disk_offsets", dof_disk_offs);
    }
  }

  PostProcessing::~PostProcessing() {
    SAFE_DELETE(rgba_ubyte_texture_);
    SAFE_DELETE(rgba_16f_texture_);
    for (uint32_t i = 0; i < nluma_textures_; i++) {
      SAFE_DELETE(luma_[i]);
    }
    SAFE_DELETE(luma_);
    SAFE_DELETE(quad_);
  }

  void PostProcessing::renderPostProcessing(const float dt) {
    // This is not very useful:
    //bool visualize_normals;
    //GET_SETTING("visualize_normals", bool, visualize_normals);
    //if (visualize_normals) {
    //  renderer_->g_buffer()->visualizeScreenNormalsToScene();
    //}

    TextureRenderable* src = renderer_->g_buffer()->final_scene();
    TextureRenderable* dst = rgba_16f_texture_;
    bool dof_on;
    GET_SETTING("dof_on", bool, dof_on);
    if (dof_on) {
      renderDOF(dst, src);
      // Now swap the textures
      TextureRenderable* tmp = src;
      src = dst;
      dst = tmp;
    }

    // Order is important, we could do motion blur first...
    bool motion_blur_on;
    GET_SETTING("motion_blur_on", bool, motion_blur_on);
    if (motion_blur_on) {
      renderMotionBlur(dst, src, dt);
      // Now swap the textures
      TextureRenderable* tmp = src;
      src = dst;
      dst = tmp;
    }

    int aa_type_enum;
    GET_SETTING("aa_type_enum", int, aa_type_enum);

    // BOTH HDR or FXAA need luma
    if (aa_type_enum != AA_OFF || true /* || hdr_on */) {
      calculateLuma(luma_, src);
    }

    if (aa_type_enum != AA_OFF) {
      if (aa_type_enum >= FXAA_LQ_FULL && aa_type_enum <= FXAA_HQ_EDGE_AWARE) {
        renderFXAA(dst, src, aa_type_enum);
        // Now swap the textures
        TextureRenderable* tmp = src;
        src = dst;
        dst = tmp;
      }
    }

    if (src == rgba_16f_texture_) {
      copyTex(dst, src);
    }
  }

  void PostProcessing::copyTex(const TextureRenderable* dst, 
    const TextureRenderable* src) const {
    dst->begin();

    GLState::glsViewport(0, 0, dst->w(), dst->h());  
    ERROR_CHECK;

    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      FULLSCREEN_QUAD_F_SHADER);

    src->bind(0, GL_TEXTURE0, "f_texture_sampler");

    GLState::glsDisable(GL_DEPTH_TEST);
    GLState::glsDisable(GL_CULL_FACE);
    GLState::glsDisable(GL_BLEND);

    quad_->draw();

    dst->end();
  }

  void PostProcessing::useFullscreenQuadSP(const GLint format) const {
    uint32_t num_chan = NumElementsOfGLFormat(format);
    switch (num_chan) {
      case 1:
      ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
        FULLSCREEN_R_QUAD_F_SHADER);
      break;
      case 2:
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
        FULLSCREEN_RG_QUAD_F_SHADER);
      break;
      case 3:
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
        FULLSCREEN_RGB_QUAD_F_SHADER);
      break;
      case 4:
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
        FULLSCREEN_RGBA_QUAD_F_SHADER);
      break;
    }
  }

  void PostProcessing::renderFullscreenQuad(const TextureRenderable* tex, 
    const uint32_t texture_index) const {
    // Render to the default framebuffer (back buffer)
    GLState::glsBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLState::glsViewport(0, 0, renderer_->viewport_width(), 
      renderer_->viewport_height());  
    useFullscreenQuadSP(tex->format());
    tex->bind(texture_index, GL_TEXTURE0, "f_texture_sampler");
    GLState::setupQuadRendering();
    quad_->draw();
  }  

  void PostProcessing::renderFullscreenNull() const {
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      FULLSCREEN_NULL_F_SHADER);
    GLState::setupQuadRendering();
    quad_->draw();
  }  

  void PostProcessing::renderFullscreenQuad(Texture* tex) {
    // Render to the default framebuffer (back buffer)
    GLState::glsBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLState::glsViewport(0, 0, renderer_->viewport_width(), 
      renderer_->viewport_height());  
    useFullscreenQuadSP(tex->format());
    tex->bind(GL_TEXTURE0, "f_texture_sampler");
    GLState::setupQuadRendering();
    quad_->draw();
  }

  void PostProcessing::renderFullscreenQuad(const GLuint h_texture) const {
    // Render to the default framebuffer (back buffer)
    GLState::glsBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLState::glsViewport(0, 0, renderer_->viewport_width(), 
      renderer_->viewport_height());  
    // Assume RGBA
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      FULLSCREEN_QUAD_F_SHADER);

    if (!GLState::glsQueryIfTextureIsBound(GL_TEXTURE_2D, GL_TEXTURE0, 
      h_texture)) {
      GLState::glsActiveTexture(GL_TEXTURE0);
      GLState::glsBindTexture(GL_TEXTURE_2D, h_texture);
    }
    BIND_UNIFORM("f_texture_sampler", 0);
    ERROR_CHECK;
    GLState::setupQuadRendering();
    quad_->draw();
  }

  void PostProcessing::rectBlur(const TextureRenderable* texture,
    const TextureRenderable* temp_texture, const uint32_t blur_radius, 
    const uint32_t texture_index) const {
    // Check that the temporary texture format matches the input texture
    if (!texture->compareFormat(temp_texture)) {
      throw wruntime_error(L"PostProcessing::rectBlur() - "
        L"ERROR: temp texture format does not match input texture");
    }

    // horizontal blur from texture into temp_texture
    temp_texture->begin();
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      RECT_BLUR_SHADER);
    texture->bind(texture_index, GL_TEXTURE0, "f_texture_sampler");
    GLint radius = (GLint)blur_radius;
    BIND_UNIFORM("f_radius", &radius);

    math::Float2 texel_size(1.0f / (float)texture->w(), 0.0f);
    BIND_UNIFORM("f_texel_size", texel_size.m);

    GLState::setupQuadRendering();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ERROR_CHECK;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    ERROR_CHECK;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ERROR_CHECK;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    ERROR_CHECK;

    quad_->draw();
    temp_texture->end();

    // vertical blur from temp_texture into texture
    texture->begin();
    temp_texture->bind(texture_index, GL_TEXTURE0, "f_texture_sampler");
    texel_size.set(0, 1.0f / (float)texture->h());
    BIND_UNIFORM("f_texel_size", texel_size.m);

    quad_->draw();
    texture->end();
  }

  void PostProcessing::rectBlur(const TextureRenderableArray* texture,
    const TextureRenderableArray* temp_texture, const uint32_t blur_radius, 
    const uint32_t array_index, const uint32_t texture_index) const {
    // Check that the temporary texture format matches the input texture
    if (!texture->compareFormat(temp_texture)) {
      throw wruntime_error(L"PostProcessing::rectBlur() - "
        L"ERROR: temp texture format does not match input texture");
    }

    // horizontal blur from texture into temp_texture
    temp_texture->begin(array_index);
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      RECT_BLUR_SHADER_ARRAY);
    texture->bind(texture_index, GL_TEXTURE0, "f_texture_sampler");
    GLint radius = (GLint)blur_radius;
    BIND_UNIFORM("f_radius", &radius);

    math::Float2 texel_size(1.0f / (float)texture->w(), 0.0f);
    BIND_UNIFORM("f_texel_size", texel_size.m);
    float a_index = static_cast<float>(array_index);
    BIND_UNIFORM("f_array_index", &a_index);

    GLState::setupQuadRendering();

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ERROR_CHECK;
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    ERROR_CHECK;
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ERROR_CHECK;
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    ERROR_CHECK;

    quad_->draw();
    temp_texture->end();

    // vertical blur from temp_texture into texture
    texture->begin(array_index);
    temp_texture->bind(0, GL_TEXTURE0, "f_texture_sampler");
    texel_size.set(0.0, 1.0f / (float)texture->h());
    BIND_UNIFORM("f_texel_size", texel_size.m);

    quad_->draw();
    texture->end();
  }

  void PostProcessing::renderDOF(TextureRenderable* dst, 
    TextureRenderable* src) {
    ShaderProgram::useShaderProgram(DOF_V_SHADER, DOF_F_SHADER);
    GLState::setupQuadRendering();

    dst->begin();

    math::Float4 dof_bounds;
    GET_SETTING("dof_bounds", math::Float4, dof_bounds);
    BIND_UNIFORM("v_dof_bounds", &dof_bounds);
    const math::Float2& near_far = renderer_->camera()->near_far();
    BIND_UNIFORM("v_camera_far", &renderer_->camera()->near_far()[1]);
    float mid = 0.5f * (near_far[0] + near_far[1]);
    BIND_UNIFORM("v_camera_mid", &mid);

    renderer_->g_buffer()->g_buffer_texture()->bindDepthNormalViewTex(
      GL_TEXTURE0, "v_depth_normal_view");

    renderer_->g_buffer()->g_buffer_texture()->bindDepthNormalViewTex(
      GL_TEXTURE1, "f_depth_normal_view_stencil");

    src->bind(0, GL_TEXTURE2, "f_final_scene");

    syncDOFDiskOffsets();

    quad_->draw();

    dst->end();
  }

  void PostProcessing::renderMotionBlur(TextureRenderable* dst, 
    TextureRenderable* src, const float dt) {
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      MOTION_BLUR_F_SHADER);
    GLState::setupQuadRendering();

    dst->begin();

    int max_samples;
    GET_SETTING("motion_blur_max_samples", int, max_samples);
    BIND_UNIFORM("f_max_samples", &max_samples);

    renderer_->g_buffer()->g_buffer_texture()->bindDepthNormalViewTex(
        GL_TEXTURE0, "f_depth_normal_view_stencil");
    renderer_->g_buffer()->g_buffer_texture()->bindSpecPowerVel(
        GL_TEXTURE1, "f_spec_power_vel");
    src->bind(0, GL_TEXTURE2, "f_src_color");

    math::Float2 texel_size(1.0f / (float)src->w(), 1.0f / (float)src->h());
    BIND_UNIFORM("f_texel_size", texel_size.m);

    float target_fps, depth_cutoff;
    GET_SETTING("motion_blur_target_fps", float, target_fps);
    GET_SETTING("motion_blur_depth_cutoff", float, depth_cutoff);
    float time_scale = (1.0f / (dt * target_fps));  // cur_fps / target_fps
    BIND_UNIFORM("f_time_scale", &time_scale);
    BIND_UNIFORM("f_depth_cutoff", &depth_cutoff);

    quad_->draw();

    dst->end();
  }

  void PostProcessing::renderFXAA(TextureRenderable* dst, 
    TextureRenderable* src, int aa_type) {
    switch (aa_type) {
    case FXAA_LQ_FULL:
      ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
        FXAA_LQ_F_SHADER);
      break;
    case FXAA_LQ_EDGE_AWARE:
      ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
        FXAA_LQ_EDGE_AWARE_F_SHADER);
      break;
    case FXAA_HQ_FULL:
      ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
        FXAA_HQ_F_SHADER);
      break;
    case FXAA_HQ_EDGE_AWARE:
      ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
        FXAA_HQ_EDGE_AWARE_F_SHADER);
      break;
    default:
      throw std::wruntime_error("PostProcessing::renderFXAA() - ERROR: type"
        " not recognized!");
    }
    GLState::setupQuadRendering();

    dst->begin();

#if defined(DEBUG) || defined(_DEBUG)
    if (src->filter() != TextureFilterMode::TEXTURE_LINEAR) {
      throw wruntime_error("PostProcessing::renderFXAA() - ERROR: src texture"
        " needs bilinear filtering enabled!");
    }
#endif

    src->bind(0, GL_TEXTURE2, "f_src_rgb");
    luma_[0]->bind(0, GL_TEXTURE3, "f_src_luma");
    math::Float2 texel_size(1.0f / (float)src->w(), 1.0f / (float)src->h());
    BIND_UNIFORM("f_texel_size", texel_size.m);

    if (aa_type == FXAA_LQ_EDGE_AWARE || aa_type == FXAA_HQ_EDGE_AWARE) {
      renderer_->g_buffer()->g_buffer_texture()->bindDepthNormalViewTex(
        GL_TEXTURE1, "f_depth_normal_view_stencil");
      float depth_threshold, angle_threshold;
      GET_SETTING("fxaa_angle_threshold", float, angle_threshold);
      GET_SETTING("fxaa_depth_threshold", float, depth_threshold);

      // A.B = |A||B|cos(theta)
      float dot_thresh = cosf(angle_threshold * (float)PI_OVER_180);

      BIND_UNIFORM("f_normal_dot_threshold", &dot_thresh);
      BIND_UNIFORM("f_depth_threshold", &depth_threshold);
    }

    if (aa_type == FXAA_HQ_FULL || aa_type == FXAA_HQ_EDGE_AWARE) {
      float hq_subpix_quality;
      GET_SETTING("fxaa_hq_subpix_quality", float, hq_subpix_quality);
      BIND_UNIFORM("f_hq_subpix_quality", &hq_subpix_quality);
      
    }

    quad_->draw();

    dst->end();
  }

  void PostProcessing::renderSMAA(TextureRenderable* dst, 
    TextureRenderable* src, int aa_type) {
    static_cast<void>(dst);
    static_cast<void>(src);
    static_cast<void>(aa_type);
    throw std::wruntime_error("SMAA Not yet finished");
  }

  void PostProcessing::calculateLuma(TextureRenderable** dst_l, 
    TextureRenderable* src_rgb) const {
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      LUMA_F_SHADER);
    GLState::setupQuadRendering();

    dst_l[0]->begin();
    src_rgb->bind(0, GL_TEXTURE0, "f_src_rgb");
    quad_->draw();
    dst_l[0]->end();

    // Perform downsample + averaging here...
  }

}  // namespace renderer
}  // namespace jtil

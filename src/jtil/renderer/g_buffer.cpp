#include <iostream>  // TEMP CODE
#include <sstream>
#include "jtil/renderer/g_buffer.h"
#include "jtil/renderer/renderer.h"
#include "jtil/renderer/post_processing.h"
#include "jtil/renderer/shader/shader.h"
#include "jtil/renderer/shader/shader_program.h"
#include "jtil/renderer/camera/camera.h"
#include "jtil/renderer/geometry/geometry.h"
#include "jtil/renderer/geometry/geometry_manager.h"
#include "jtil/renderer/geometry/geometry_instance.h"
#include "jtil/renderer/geometry/geometry_render_pass.h"
#include "jtil/renderer/texture/texture_gbuffer.h"
#include "jtil/renderer/texture/texture_renderable.h"
#include "jtil/settings/settings_manager.h"
#include "jtil/renderer/gl_state.h"

using std::string;
using std::wstring;
using std::wruntime_error;

namespace jtil {

using math::Float4x4;
using math::Float4;
using math::Float3;
using math::Float2;
using data_str::Pair;

#define COLR_POINTS_V_SHADER "./shaders/g_buffer/g_buffer_colr_points.vert"
#define COLR_POINTS_VEL_V_SHADER "./shaders/g_buffer/g_buffer_colr_points_vel.vert"
#define COLR_POINTS_F_SHADER "./shaders/g_buffer/g_buffer_colr_points.frag"
#define COLR_POINTS_VEL_F_SHADER "./shaders/g_buffer/g_buffer_colr_points_vel.frag"

#define COLR_LINES_V_SHADER "./shaders/g_buffer/g_buffer_colr_lines.vert"
#define COLR_LINES_VEL_V_SHADER "./shaders/g_buffer/g_buffer_colr_lines_vel.vert"
#define COLR_LINES_F_SHADER "./shaders/g_buffer/g_buffer_colr_lines.frag"
#define COLR_LINES_VEL_F_SHADER "./shaders/g_buffer/g_buffer_colr_lines_vel.frag"

#define COLR_MESH_V_SHADER "./shaders/g_buffer/g_buffer_colr_mesh.vert"
#define COLR_MESH_VEL_V_SHADER "./shaders/g_buffer/g_buffer_colr_mesh_vel.vert"
#define COLR_MESH_F_SHADER "./shaders/g_buffer/g_buffer_colr_mesh.frag"
#define COLR_MESH_VEL_F_SHADER "./shaders/g_buffer/g_buffer_colr_mesh_vel.frag"

#define COLR_BONED_MESH_V_SHADER "./shaders/g_buffer/g_buffer_colr_boned_mesh.vert"
#define COLR_BONED_MESH_VEL_V_SHADER "./shaders/g_buffer/g_buffer_colr_boned_mesh_vel.vert"
#define COLR_BONED_MESH_VEL_HQ_V_SHADER "./shaders/g_buffer/g_buffer_colr_boned_mesh_vel_hq.vert"

#define CONST_COLR_MESH_V_SHADER "./shaders/g_buffer/g_buffer_const_colr_mesh.vert"
#define CONST_COLR_MESH_VEL_V_SHADER "./shaders/g_buffer/g_buffer_const_colr_mesh_vel.vert"
#define CONST_COLR_MESH_F_SHADER "./shaders/g_buffer/g_buffer_const_colr_mesh.frag"
#define CONST_COLR_MESH_VEL_F_SHADER "./shaders/g_buffer/g_buffer_const_colr_mesh_vel.frag"

#define CONST_COLR_BONED_MESH_V_SHADER "./shaders/g_buffer/g_buffer_const_colr_boned_mesh.vert"
#define CONST_COLR_BONED_MESH_VEL_V_SHADER "./shaders/g_buffer/g_buffer_const_colr_boned_mesh_vel.vert"
#define CONST_COLR_BONED_MESH_VEL_HQ_V_SHADER "./shaders/g_buffer/g_buffer_const_colr_boned_mesh_vel_hq.vert"

#define TEXT_MESH_V_SHADER "./shaders/g_buffer/g_buffer_text_mesh.vert"
#define TEXT_MESH_VEL_V_SHADER "./shaders/g_buffer/g_buffer_text_mesh_vel.vert"
#define TEXT_MESH_F_SHADER "./shaders/g_buffer/g_buffer_text_mesh.frag"
#define TEXT_MESH_VEL_F_SHADER "./shaders/g_buffer/g_buffer_text_mesh_vel.frag"

#define TEXT_BONED_MESH_V_SHADER "./shaders/g_buffer/g_buffer_text_boned_mesh.vert"
#define TEXT_BONED_MESH_VEL_V_SHADER "./shaders/g_buffer/g_buffer_text_boned_mesh_vel.vert"
#define TEXT_BONED_MESH_VEL_HQ_V_SHADER "./shaders/g_buffer/g_buffer_text_boned_mesh_vel_hq.vert"

#define TEXT_DISP_MESH_V_SHADER "./shaders/g_buffer/g_buffer_text_disp_mesh.vert"
#define TEXT_DISP_MESH_TC_SHADER "./shaders/g_buffer/g_buffer_text_disp_mesh.tessc.geom"
#define TEXT_DISP_MESH_TE_SHADER "./shaders/g_buffer/g_buffer_text_disp_mesh.tesse.geom"
#define TEXT_DISP_MESH_VEL_V_SHADER "./shaders/g_buffer/g_buffer_text_disp_mesh_vel.vert"
#define TEXT_DISP_MESH_VEL_TC_SHADER "./shaders/g_buffer/g_buffer_text_disp_mesh_vel.tessc.geom"
#define TEXT_DISP_MESH_VEL_TE_SHADER "./shaders/g_buffer/g_buffer_text_disp_mesh_vel.tesse.geom"

#define CLEAR_F_SHADER "./shaders/g_buffer/g_buffer_clear.frag"
#define CLEAR_TEX_F_SHADER "./shaders/g_buffer/g_buffer_clear_tex.frag"

#define VISUALIZE_DEPTH_F_SHADER "./shaders/g_buffer/g_buffer_visualize_depth.frag"
#define VISUALIZE_NORMAL_F_SHADER "./shaders/g_buffer/g_buffer_visualize_normal.frag"
#define VISUALIZE_LIGHT_ACCUM_DIFF_F_SHADER "./shaders/g_buffer/g_buffer_visualize_light_accum_diffuse.frag"
#define VISUALIZE_LIGHT_ACCUM_SPEC_F_SHADER "./shaders/g_buffer/g_buffer_visualize_light_accum_specular.frag"
#define VISUALIZE_ALBEDO_F_SHADER "./shaders/g_buffer/g_buffer_visualize_albedo.frag"
#define VISUALIZE_VIEW_POS_F_SHADER "./shaders/g_buffer/g_buffer_visualize_pos.frag"
#define VISUALIZE_VEL_F_SHADER "./shaders/g_buffer/g_buffer_visualize_vel.frag"
#define VISUALIZE_LIGHTING_STENCIL_F_SHADER "./shaders/g_buffer/g_buffer_visualize_lighting_stencil.frag"

#define EMPTY_V_SHADER "./shaders/g_buffer/g_buffer_empty.vert"
#define SCREEN_NORM_G_SHADER "./shaders/g_buffer/g_buffer_norm_vis.geom"
#define NORM_F_SHADER "./shaders/g_buffer/g_buffer_norm_vis.frag"
#define NORM_G_SHADER "./shaders/g_buffer/g_buffer_norm_vis.geom"

#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }

namespace renderer {

  GBuffer::GBuffer(Renderer* renderer) {
    renderer_ = renderer;

    // The render target
    g_buffer_texture_ = new TextureGBuffer(
      Renderer::g_renderer()->width(), Renderer::g_renderer()->height());

    // The final scene buffer --> It shares a depth buffer with the g_buffer
    // texture.  This is so that we can render wireframe and light effects
    // using the populated depth buffer.
    final_scene_ = new TextureRenderable(
      GBUFFER_SCENE_INTERNAL_FORMAT, Renderer::g_renderer()->width(), 
      Renderer::g_renderer()->height(), GBUFFER_SCENE_FORMAT, 
      GBUFFER_SCENE_TYPE, 1, false, TEXTURE_LINEAR);
    final_scene_->attachSharedDepthTexture(g_buffer_texture_->depth_texture());

    render_pass_ = new GeometryRenderPass(renderer);
    render_pass_->view() = &renderer->camera()->view();
    render_pass_->proj() = &renderer->camera()->proj();
    render_pass_->frustum() = renderer->camera()->frustum();

    single_pt_ = new Geometry("gbuffer_single_point");
    single_pt_->addVertexAttribute(VERTATTR_POS);
    single_pt_->addPos(Float3(0,0,0));
    single_pt_->primative_type() = VERT_POINTS;
    single_pt_->sync();

    bool tess_on;
    GET_SETTING("tess_on", bool, tess_on);
    if (tess_on && !GLState::queryGLExtension("ARB_tessellation_shader")) {
      std::cout << "Warning: Tessellation is not supported.  Tessellation will";
      std::cout << "will be disabled..." << std::endl;
      SET_SETTING("tess_on", bool, false);
    }

  }

  GBuffer::~GBuffer() {
    SAFE_DELETE(g_buffer_texture_);
    SAFE_DELETE(final_scene_);
    SAFE_DELETE(render_pass_);
    SAFE_DELETE(single_pt_);
  }

  void GBuffer::renderGBuffer() {
    g_buffer_texture_->beginGeometryPass();

    GLState::glsDepthMask(GL_TRUE);
    GLState::glsClear(GL_DEPTH_BUFFER_BIT);

    clearGBuffer(Renderer::g_renderer()->background_tex());

    GLState::glsEnable(GL_DEPTH_TEST);
    GLState::glsDepthFunc(GL_LESS);
    GLState::glsEnable(GL_CULL_FACE);
    GLState::glsCullFace(GL_BACK);
    GLState::glsDisable(GL_BLEND);

    bool render_light_volumes=false, render_light_sources=false, 
      render_aabboxes, motion_blur_on, tess_on;
    GET_SETTING("render_light_volumes", bool, render_light_volumes);
    GET_SETTING("render_light_sources", bool, render_light_sources);
    GET_SETTING("render_aabboxes", bool, render_aabboxes);
    GET_SETTING("motion_blur_on", bool, motion_blur_on);
    GET_SETTING("tess_on", bool, tess_on);

    // Note: Textured Mesh AND Texture Displaced Mesh share a fragment
    // shader!

    if (!motion_blur_on) {
      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_COLR, 
        COLR_MESH_V_SHADER, COLR_MESH_F_SHADER);
      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_COLR_BONED, 
        COLR_BONED_MESH_V_SHADER, COLR_MESH_F_SHADER);
      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_CONST_COLR, 
        CONST_COLR_MESH_V_SHADER, CONST_COLR_MESH_F_SHADER);
      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_CONST_COLR_BONED, 
        CONST_COLR_BONED_MESH_V_SHADER, CONST_COLR_MESH_F_SHADER);
      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT, 
        TEXT_MESH_V_SHADER, TEXT_MESH_F_SHADER);
      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT_BONED, 
        TEXT_BONED_MESH_V_SHADER, TEXT_MESH_F_SHADER);
      if (tess_on) {
        render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT_DISP, 
          TEXT_DISP_MESH_V_SHADER, TEXT_MESH_F_SHADER, NULL, 
          TEXT_DISP_MESH_TC_SHADER, TEXT_DISP_MESH_TE_SHADER);
      } else {
        render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT_DISP, 
          TEXT_MESH_V_SHADER, TEXT_MESH_F_SHADER);
      }
      render_pass_->setShader(VERT_POINTS, GEOMETRY_COLR, 
        COLR_POINTS_V_SHADER, COLR_POINTS_F_SHADER);
      render_pass_->setShader(VERT_LINES, GEOMETRY_COLR, 
        COLR_LINES_V_SHADER, COLR_LINES_F_SHADER);
    } else {
      bool motion_blur_hq_boned;
      GET_SETTING("motion_blur_hq_boned", bool, motion_blur_hq_boned);

      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_COLR, 
        COLR_MESH_VEL_V_SHADER, COLR_MESH_VEL_F_SHADER);
      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_CONST_COLR, 
        CONST_COLR_MESH_VEL_V_SHADER, CONST_COLR_MESH_VEL_F_SHADER);
      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT, 
        TEXT_MESH_VEL_V_SHADER, TEXT_MESH_VEL_F_SHADER);
      if (motion_blur_hq_boned) {
        render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_CONST_COLR_BONED, 
          CONST_COLR_BONED_MESH_VEL_HQ_V_SHADER, CONST_COLR_MESH_VEL_F_SHADER);
        render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_COLR_BONED, 
          COLR_BONED_MESH_VEL_HQ_V_SHADER, COLR_MESH_VEL_F_SHADER);
        render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT_BONED, 
          TEXT_BONED_MESH_VEL_HQ_V_SHADER, TEXT_MESH_VEL_F_SHADER);
      } else {
        render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_CONST_COLR_BONED, 
          CONST_COLR_BONED_MESH_VEL_V_SHADER, CONST_COLR_MESH_VEL_F_SHADER);
        render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_COLR_BONED, 
          COLR_BONED_MESH_VEL_V_SHADER, COLR_MESH_VEL_F_SHADER);
        render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT_BONED, 
          TEXT_BONED_MESH_VEL_V_SHADER, TEXT_MESH_VEL_F_SHADER);
      }
      if (tess_on) {
        render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT_DISP, 
          TEXT_DISP_MESH_VEL_V_SHADER, TEXT_MESH_VEL_F_SHADER, NULL, 
          TEXT_DISP_MESH_VEL_TC_SHADER, TEXT_DISP_MESH_VEL_TE_SHADER);
      } else {
        render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT_DISP, 
          TEXT_MESH_VEL_V_SHADER, TEXT_MESH_VEL_F_SHADER);
      }
      render_pass_->setShader(VERT_POINTS, GEOMETRY_COLR, 
        COLR_POINTS_VEL_V_SHADER, COLR_POINTS_VEL_F_SHADER);
      render_pass_->setShader(VERT_LINES, GEOMETRY_COLR, 
        COLR_LINES_VEL_V_SHADER, COLR_LINES_VEL_F_SHADER);
    }

    render_pass_->render_aabboxes() = render_aabboxes;
    render_pass_->render_light_sources() = render_light_sources;
    render_pass_->render_light_volumes() = render_light_volumes;
    render_pass_->screen_size() = &g_buffer_texture_->screen_size();

    render_pass_->render();

    bool visualize_normals;
    GET_SETTING("visualize_normals", bool, visualize_normals);
    if (visualize_normals) {
      render_pass_->unsetShaders();
      // Normal visualization pass:  It is slow but it's just for debugging
      render_pass_->shader_uniform_cb() = visualizeNormalsUniformCB;
      render_pass_->render_aabboxes() = false;
      render_pass_->render_light_sources() = false;
      render_pass_->render_light_volumes() = false;
      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_COLR, 
        COLR_MESH_V_SHADER, NORM_F_SHADER, NORM_G_SHADER);
      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_COLR_BONED, 
        COLR_BONED_MESH_V_SHADER, NORM_F_SHADER, NORM_G_SHADER);
      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_CONST_COLR, 
        CONST_COLR_MESH_V_SHADER, NORM_F_SHADER, NORM_G_SHADER);
      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_CONST_COLR_BONED, 
        CONST_COLR_BONED_MESH_V_SHADER, NORM_F_SHADER, NORM_G_SHADER);
      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT, 
        TEXT_MESH_V_SHADER, NORM_F_SHADER, NORM_G_SHADER);
      render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT_BONED, 
        TEXT_BONED_MESH_V_SHADER, NORM_F_SHADER, NORM_G_SHADER);
      // TODO: Fix this!!! This geometry type throws invalid operation
      //render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT_DISP, 
      //  TEXT_DISP_MESH_V_SHADER, NORM_F_SHADER, NORM_G_SHADER, 
      //  TEXT_DISP_MESH_TC_SHADER, TEXT_DISP_MESH_TE_SHADER);
      // TODO: Fix this!!! This geometry type throws invalid operation
      //render_pass_->setShader(VERT_POINTS, GEOMETRY_COLR, 
      //  COLR_POINTS_V_SHADER, NORM_F_SHADER, NORM_G_SHADER);
      render_pass_->render();
      render_pass_->shader_uniform_cb() = NULL;
      render_pass_->unsetShaders();
    }

    g_buffer_texture_->endGeometryPass();

    // Set the depth mask to false to prevent anyone else from writing to it.
    GLState::glsDepthMask(GL_FALSE);
  } 

  void GBuffer::visualizeNormalsUniformCB() {
    if (QUERY_UNIFORM("normal_length")) {
      float visualize_normals_length;
      GET_SETTING("visualize_normals_length", float, visualize_normals_length);
      BIND_UNIFORM("normal_length", &visualize_normals_length);
    }
  }

  void GBuffer::clearGBuffer(Texture* tex) {
    if (tex == NULL) {
      ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
        CLEAR_F_SHADER);
      Float3 clear_color;
      GET_SETTING("clear_color", Float3, clear_color);
      BIND_UNIFORM("f_clear_color", clear_color.m);
    } else {
      ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
        CLEAR_TEX_F_SHADER);
      tex->bind(GL_TEXTURE0, "f_texture_sampler");
      int32_t stretch = renderer_->stretch_background_tex() ? 1 : 0;
      BIND_UNIFORM("f_strech_tex", &stretch);
      GLfloat tex_aspect = (float)tex->w() / (float)tex->h();
      BIND_UNIFORM("f_tex_aspect", &tex_aspect);
      GLfloat screen_aspect = (float)g_buffer_texture_->w() / 
        (float)g_buffer_texture_->h();
      BIND_UNIFORM("f_screen_aspect", &screen_aspect);
      Float3 clear_color;
      GET_SETTING("clear_color", Float3, clear_color);
      BIND_UNIFORM("f_clear_color", clear_color.m);
    }
    GLState::setupQuadRendering();
    renderer_->post_processing()->quad()->draw();
  }

  void GBuffer::visualizeGBuffer() const {
    uint32_t w = Renderer::g_renderer()->width();
    uint32_t h = Renderer::g_renderer()->height();
    GLState::glsViewport(0, 0, w/2, h/2);
    visualizeGBufferViewPos();
    GLState::glsViewport(w/2, 0, w/2, h/2);
    visualizeGBufferNormal();
    GLState::glsViewport(0, h/2, w/2, h/2);
    visualizeGBufferAlbedo();
    GLState::glsViewport(w/2, h/2, w/2, h/2);
    visualizeGBufferDepth();
    GLState::glsViewport(0, 0, w, h);  
  }

  void GBuffer::visualizeGBufferNormal() const {
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      VISUALIZE_NORMAL_F_SHADER);
    GLState::setupQuadRendering();
    g_buffer_texture_->bindDepthNormalViewTex(GL_TEXTURE0, 
      "f_texture_sampler");
    renderer_->post_processing()->quad()->draw();
  }

  void GBuffer::visualizeGBufferLightingAccumDiff() const {
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      VISUALIZE_LIGHT_ACCUM_DIFF_F_SHADER);
    GLState::setupQuadRendering();
    g_buffer_texture_->bindLightAccumulationTex(GL_TEXTURE0, 
      "f_texture_sampler");
    renderer_->post_processing()->quad()->draw();
  }

  void GBuffer::visualizeGBufferLightingAccumSpec() const {
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      VISUALIZE_LIGHT_ACCUM_SPEC_F_SHADER);
    GLState::setupQuadRendering();
    g_buffer_texture_->bindLightAccumulationTex(GL_TEXTURE0, 
      "f_texture_sampler");
    renderer_->post_processing()->quad()->draw();
  }

  void GBuffer::visualizeGBufferAlbedo() const {
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      VISUALIZE_ALBEDO_F_SHADER);
    GLState::setupQuadRendering();
    g_buffer_texture_->bindAlbedoSpecIntensityTex(GL_TEXTURE0, 
      "f_texture_sampler");
    renderer_->post_processing()->quad()->draw();
  }
  
  void GBuffer::visualizeGBufferDepth() const {
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      VISUALIZE_DEPTH_F_SHADER);
    GLState::setupQuadRendering();
    g_buffer_texture_->bindDepthNormalViewTex(GL_TEXTURE0, 
      "f_texture_sampler");
    BIND_UNIFORM("f_camera_near_far", renderer_->camera()->near_far().m);
    renderer_->post_processing()->quad()->draw();
  }

  void GBuffer::visualizeGBufferViewPos() const {
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      VISUALIZE_VIEW_POS_F_SHADER);
    GLState::setupQuadRendering();
    g_buffer_texture_->bindDepthNormalViewTex(GL_TEXTURE0, 
      "f_texture_sampler");
    BIND_UNIFORM("f_inv_focal_length", 
      renderer_->camera()->inv_focal_length().m);
    renderer_->post_processing()->quad()->draw();
  }

  void GBuffer::visualizeGBufferVel() const {
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      VISUALIZE_VEL_F_SHADER);
    GLState::setupQuadRendering();
    g_buffer_texture_->bindSpecPowerVel(GL_TEXTURE0, 
      "f_texture_sampler");
    renderer_->post_processing()->quad()->draw();
  }

  void GBuffer::visualizeGBufferLightingStencil() const {
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      VISUALIZE_LIGHTING_STENCIL_F_SHADER);
    GLState::setupQuadRendering();
    g_buffer_texture_->bindDepthNormalViewTex(GL_TEXTURE0, 
      "f_texture_sampler");
    renderer_->post_processing()->quad()->draw();
  }
}  // namespace renderer
}  // namespace jtil

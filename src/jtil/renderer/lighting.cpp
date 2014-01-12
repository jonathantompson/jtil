#include <sstream>
#include <iostream>
#include "jtil/renderer/lighting.h"
#include "jtil/renderer/renderer.h"
#include "jtil/renderer/post_processing.h"
#include "jtil/renderer/g_buffer.h"
#include "jtil/renderer/shader/shader.h"
#include "jtil/renderer/shader/shader_program.h"
#include "jtil/renderer/texture/texture_renderable.h"
#include "jtil/renderer/texture/texture_renderable_array.h"
#include "jtil/renderer/texture/texture_gbuffer.h"
#include "jtil/renderer/texture/texture.h"
#include "jtil/renderer/geometry/geometry.h"
#include "jtil/renderer/geometry/geometry_instance.h"
#include "jtil/renderer/geometry/geometry_manager.h"
#include "jtil/renderer/geometry/geometry_render_pass.h"
#include "jtil/renderer/colors/colors.h"
#include "jtil/renderer/lights/light.h"
#include "jtil/renderer/lights/light_dir.h"
#include "jtil/renderer/lights/light_point.h"
#include "jtil/renderer/lights/light_spot.h"
#include "jtil/renderer/lights/light_spot_cvsm.h"
#include "jtil/renderer/camera/camera.h"
#include "jtil/renderer/objects/aabbox.h"
#include "jtil/renderer/camera/frustum.h"
#include "jtil/settings/settings_manager.h"
#include "jtil/renderer/gl_state.h"

// Light stencil and accumulation shaders
#define LIGHT_VOLUME_V_SHADER "./shaders/lighting/lighting_light_volume.vert"
#define LIGHT_VOLUME_STENCIL_F_SHADER "./shaders/lighting/lighting_light_volume_stencil.frag"
#define LIGHT_VOLUME_SPOT_ACCUM_F_SHADER "./shaders/lighting/lighting_light_volume_spot_accum.frag"
#define LIGHT_VOLUME_SPOT_VSM_ACCUM_F_SHADER "./shaders/lighting/lighting_light_volume_spot_vsm_accum.frag"
#define LIGHT_VOLUME_SPOT_CVSM_ACCUM_F_SHADER "./shaders/lighting/lighting_light_volume_spot_cvsm_accum.frag"
#define LIGHT_VOLUME_SPOT_CVSM_CNT1_ACCUM_F_SHADER "./shaders/lighting/lighting_light_volume_spot_cvsm_cnt1_accum.frag"
#define LIGHT_VOLUME_POINT_ACCUM_F_SHADER "./shaders/lighting/lighting_light_volume_point_accum.frag"
#define LIGHT_DIR_ACCUM_F_SHADER "./shaders/lighting/lighting_light_dir_accum.frag"
#define LIGHT_FINAL_F_SHADER "./shaders/lighting/lighting_final.frag"
#define LIGHTING_NOISE_TEXTURE "resource_files/vector_noise_128x128.png"

// Ambient occlusion shaders and textures
#define LIGHT_AMBIENT_OCCLUSION_F_SHADER "./shaders/lighting/lighting_ambient_occlusion.frag"
#define LIGHT_AMBIENT_OCCLUSION_BLUR_F_SHADER "./shaders/lighting/lighting_ambient_occlusion_blur.frag"

// Shadow map Depth and visualization shaders:
#define VSM_COLR_MESH_V_SHADER "./shaders/lighting/lighting_vsm_colr_mesh.vert"
#define VSM_COLR_BONED_MESH_V_SHADER "./shaders/lighting/lighting_vsm_colr_boned_mesh.vert"
#define VSM_CONST_COLR_MESH_V_SHADER "./shaders/lighting/lighting_vsm_const_colr_mesh.vert"
#define VSM_CONST_COLR_BONED_MESH_V_SHADER "./shaders/lighting/lighting_vsm_const_colr_boned_mesh.vert"
#define VSM_TEXT_MESH_V_SHADER "./shaders/lighting/lighting_vsm_text_mesh.vert"
#define VSM_TEXT_BONED_MESH_V_SHADER "./shaders/lighting/lighting_vsm_text_boned_mesh.vert"
#define VSM_TEXT_DISP_MESH_V_SHADER "./shaders/g_buffer/g_buffer_text_disp_mesh.vert"
#define VSM_TEXT_DISP_MESH_TC_SHADER "./shaders/g_buffer/g_buffer_text_disp_mesh.tessc.geom"
#define VSM_TEXT_DISP_MESH_TE_SHADER "./shaders/g_buffer/g_buffer_text_disp_mesh.tesse.geom"
#define VSM_MESH_F_SHADER "./shaders/lighting/lighting_vsm_mesh.frag"

#define DRAW_VSM_F_SHADER "./shaders/lighting/lighting_draw_vsm.frag"
#define DRAW_CVSM_F_SHADER "./shaders/lighting/lighting_draw_cvsm.frag"
#define CALC_CSM_SPLIT_F_SHADER "./shaders/lighting/lighting_calc_csm_split.frag"
#define VISUALIZE_CSM_SPLIT_F_SHADER "./shaders/lighting/lighting_visualize_csm_split.frag"

// Ambient occlusion buffer
#define LIGHTING_AMBIENT_INTERNAL_FORMAT GL_R16F
#define LIGHTING_AMBIENT_FORMAT GL_RED
#define LIGHTING_AMBIENT_TYPE GL_FLOAT

// shadowmap
//#define LIGHTING_VSM_INTERNAL_FORMAT GL_RG16F
#define LIGHTING_VSM_INTERNAL_FORMAT GL_RG32F
#define LIGHTING_VSM_FORMAT GL_RG
#define LIGHTING_VSM_TYPE GL_FLOAT

// shadowmap split
#define LIGHTING_VSM_SPLITS_INTERNAL_FORMAT GL_R8I
#define LIGHTING_VSM_SPLITS_FORMAT GL_RED_INTEGER
#define LIGHTING_VSM_SPLITS_TYPE GL_INT

#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }

using std::wstring;
using std::wruntime_error;
using std::cout;
using std::endl;

namespace jtil {

using math::Float4x4;
using math::Float4;
using math::Float3;
using math::Float2;
using renderer::Geometry;

namespace renderer {

  const LightSpotCVSM* Lighting::cur_light_spot_vsm_;
  const Lighting* Lighting::cur_lighting_;

  Lighting::Lighting(Renderer* renderer) {
    renderer_ = renderer;

    // Load in the light geometry
    light_geom_point_ = renderer_->geometry_manager()->makeSphereGeometry(
      std::string("LightSphereGeometry"),
      LIGHT_POINT_MODEL_STACKS, LIGHT_POINT_MODEL_SLICES, 
      LIGHT_POINT_MODEL_INSIDE_RADIUS);
    light_geom_spot_ = renderer_->geometry_manager()->makeConeGeometry(
      std::string("LightConeGeometry"),
      LIGHT_SPOT_MODEL_SLICES, LIGHT_SPOT_MODEL_HEIGHT, 
      LIGHT_SPOT_MODEL_INSIDE_RADIUS);

    ambient_ = new TextureRenderable(LIGHTING_AMBIENT_INTERNAL_FORMAT, 
      renderer_->width(), renderer_->height(), LIGHTING_AMBIENT_FORMAT, 
      LIGHTING_AMBIENT_TYPE, 1, false);
    ambient_blur_temp_ = new TextureRenderable(
      LIGHTING_AMBIENT_INTERNAL_FORMAT, renderer_->width(), 
      renderer_->height(), LIGHTING_AMBIENT_FORMAT,
      LIGHTING_AMBIENT_TYPE, 1, false);

    int res_e;
    GET_SETTING("vsm_resolution_enum", int, res_e);
    uint32_t res = LightSpotCVSM::VSMResEnum2Res(static_cast<VSM_RES>(res_e));

    // Check whether or not Texture arrays are supported...
    bool texture_array_disable;
    GET_SETTING("force_texture_array_disable", bool, texture_array_disable);
    if (texture_array_disable || 
      !GLState::queryGLExtension("GL_EXT_texture_array")) {
      cout << "Cascaded shadow map functionality will be disabled." << endl;
      shadow_map_ = new TextureRenderable(LIGHTING_VSM_INTERNAL_FORMAT, 
        res, res, LIGHTING_VSM_FORMAT, LIGHTING_VSM_TYPE, 1, true);
      shadow_map_blur_temp_ = new TextureRenderable(
        LIGHTING_VSM_INTERNAL_FORMAT, res, res, LIGHTING_VSM_FORMAT, 
        LIGHTING_VSM_TYPE, 1, true);
    } else {
      shadow_map_ = new TextureRenderableArray(LIGHTING_VSM_INTERNAL_FORMAT, 
        res, res, LIGHTING_VSM_FORMAT, LIGHTING_VSM_TYPE, 1, 
        LIGHTING_CVSM_MAX_COUNT, true);
      shadow_map_blur_temp_ = new TextureRenderableArray(
        LIGHTING_VSM_INTERNAL_FORMAT, res, res, LIGHTING_VSM_FORMAT, 
        LIGHTING_VSM_TYPE, 1, LIGHTING_CVSM_MAX_COUNT, true);
    }

    vector_noise_tex_ = renderer_->geometry_manager()->loadTexture(
      LIGHTING_NOISE_TEXTURE, TEXTURE_REPEAT, TEXTURE_NEAREST, false);
    ambient_cleared_ = false;

    vsm_render_pass_ = new GeometryRenderPass(renderer);
    vsm_render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_COLR, 
      VSM_COLR_MESH_V_SHADER, VSM_MESH_F_SHADER);
    vsm_render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_COLR_BONED, 
      VSM_COLR_BONED_MESH_V_SHADER, VSM_MESH_F_SHADER);
    vsm_render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_CONST_COLR, 
      VSM_CONST_COLR_MESH_V_SHADER, VSM_MESH_F_SHADER);
    vsm_render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_CONST_COLR_BONED, 
      VSM_CONST_COLR_BONED_MESH_V_SHADER, VSM_MESH_F_SHADER);
    vsm_render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT, 
      VSM_TEXT_MESH_V_SHADER, VSM_MESH_F_SHADER);
    vsm_render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT_BONED, 
      VSM_TEXT_BONED_MESH_V_SHADER, VSM_MESH_F_SHADER);

    vsm_render_pass_->render_light_sources() = false;
    vsm_render_pass_->render_light_volumes() = false;

    vsm_splits_ = new TextureRenderable(LIGHTING_VSM_SPLITS_INTERNAL_FORMAT, 
      renderer_->width(), renderer_->height(), LIGHTING_VSM_SPLITS_FORMAT, 
      LIGHTING_VSM_SPLITS_TYPE, 1, false);

    vsm_frustum_ = new Frustum();
  }

  Lighting::~Lighting() {
    SAFE_DELETE(light_geom_point_);
    SAFE_DELETE(light_geom_spot_);
    SAFE_DELETE(ambient_);
    SAFE_DELETE(ambient_blur_temp_);
    SAFE_DELETE(shadow_map_);
    SAFE_DELETE(shadow_map_blur_temp_);
    SAFE_DELETE(vsm_render_pass_);
    SAFE_DELETE(vsm_splits_);
    SAFE_DELETE(vsm_frustum_);
  }

  void Lighting::addLight(Light* light) { 
    lights_.pushBack(light);
  }

  void Lighting::updateLights() {
    // Simply iterate through and update each light object
    for (uint32_t i = 0; i < lights_.size(); i++) {
      if (lights_[i]->on()) {
        lights_[i]->update();
      }
    }
  }

  void Lighting::updateLightsPVW() {
    for (uint32_t i = 0; i < lights_.size(); i++) {
      if (lights_[i]->on()) {
        lights_[i]->updatePVW();
      }
    }
  }

  void Lighting::renderLighting() {
    updateLightsPVW();
    renderLightAccumulationPass();
    renderAmbientOcclusionPass();
    renderLightFinalPass();
    visualizeCVSMSplit();
  }  

  void Lighting::renderLightAccumulationPass() {
    TextureGBuffer* g_buffer = renderer_->g_buffer()->g_buffer_texture();
    g_buffer->clearLightAccumTexture();
    
    const Frustum* frustum = renderer_->camera()->frustum();

    bool light_pass_stencil_opt = false;
    GET_SETTING("light_pass_stencil_opt", bool, light_pass_stencil_opt);

    // Render light contributions into the accumulation buffer: point lights
    bool point_lights;
    GET_SETTING("point_lights", bool, point_lights);
    if (point_lights) {
      for (uint32_t i = 0; i < lights_.size(); i++) {
        if (lights_[i]->on() && lights_[i]->type() == LIGHT_POINT) {
          if (lights_[i]->aabbox()->frustumCullTest(frustum)) {
            LightPoint* light = reinterpret_cast<LightPoint*>(lights_[i]);
            if (light_pass_stencil_opt) {
              lightPointStencilPass(light);
            }
            lightPointAccumPass(light, light_pass_stencil_opt);
          }
        }
      }
    }

    bool spot_lights, vsm_on;
    GET_SETTING("spot_lights", bool, spot_lights);
    GET_SETTING("vsm_on", bool, vsm_on);
    if (spot_lights) {
      // Render light contributions into the accumulation buffer: spot lights
      for (uint32_t i = 0; i < lights_.size(); i++) {
        if (lights_[i]->on() && (lights_[i]->type() == LIGHT_SPOT || 
          (!vsm_on && lights_[i]->type() == LIGHT_SPOT_VSM))) {
          if (lights_[i]->aabbox()->frustumCullTest(frustum)) {
            LightSpot* light = reinterpret_cast<LightSpot*>(lights_[i]);
            if (light_pass_stencil_opt) {
              lightSpotStencilPass(light);
            }
            lightSpotAccumPass(light, light_pass_stencil_opt);
          }
        }
      }
      // Render shadow casters in a seperate pass
      if (vsm_on) {
        bool tess_on;
        GET_SETTING("tess_on", bool, tess_on);
        if (tess_on) {
          vsm_render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT_DISP, 
            VSM_TEXT_DISP_MESH_V_SHADER, VSM_MESH_F_SHADER, NULL,
            VSM_TEXT_DISP_MESH_TC_SHADER, VSM_TEXT_DISP_MESH_TE_SHADER);
        } else {
          vsm_render_pass_->setShader(VERT_TRIANGLES, GEOMETRY_NORM_TEXT_DISP, 
            VSM_TEXT_MESH_V_SHADER, VSM_MESH_F_SHADER);
        }
        bool cascaded_sm_disabled = shadow_map_->type() == 
          TextureType::TEXTURE_RENDERABLE_TYPE;

        // Render all the VSM first (with CSM count of one)
        for (uint32_t i = 0; i < lights_.size(); i++) {
          if (lights_[i]->on() && (lights_[i]->type() == LIGHT_SPOT_VSM && 
            (((LightSpotCVSM*)lights_[i])->cvsm_count() == 1 || 
            cascaded_sm_disabled))) {
            if (lights_[i]->aabbox()->frustumCullTest(frustum)) {
              // Render the shadow map depth + variance buffer
              LightSpotCVSM* light = (LightSpotCVSM*)(lights_[i]);
              light->updateVSMMatrices();
              renderSpotVSM(light);
              blurShadowMap(1);
              if (light_pass_stencil_opt) {
                lightSpotStencilPass((LightSpot*)light);
              }
              LightSpotVSMAccumPass(light, light_pass_stencil_opt);
            }
          }
        }
        
        // The rendering will be faster if we batch shadow casters with the
        // same cascaded shadow map count.
        if (!cascaded_sm_disabled) {
          for (uint32_t cnt = 2; cnt <= LIGHTING_CVSM_MAX_COUNT; cnt++) {
            bool vsm_of_cnt_exists = false;
            for (uint32_t i = 0; i < lights_.size(); i++) {
              if (lights_[i]->type() == LIGHT_SPOT_VSM && 
                ((LightSpotCVSM*)lights_[i])->cvsm_count() == cnt) {
                  vsm_of_cnt_exists = true;
              }
            }

            if (vsm_of_cnt_exists) {
              calcCVSMSplitDepths(cnt);
              renderCVSMSplitTexture(cnt);
              for (uint32_t i = 0; i < lights_.size(); i++) {
                if (lights_[i]->on() && (lights_[i]->type() == LIGHT_SPOT_VSM
                  && ((LightSpotCVSM*)lights_[i])->cvsm_count() == cnt)) {
                  if (lights_[i]->aabbox()->frustumCullTest(frustum)) {
                    // Render the shadow map depth + variance buffer
                    LightSpotCVSM* light = (LightSpotCVSM*)(lights_[i]);
                    light->updateVSMMatrices();
                    renderSpotCVSM(light);
                    blurShadowMap(light->cvsm_count());
                    if (light_pass_stencil_opt) {
                      lightSpotStencilPass((LightSpot*)light);
                    }
                    LightSpotCVSMAccumPass(light, light_pass_stencil_opt);
                  }
                }
              }  
            }  //  if (vsm_of_cnt_exists)
          } // if (!cascaded_sm_disabled)
        }
      }
    }

    // Directional lights fill the entire scene, so disable stencil tests.
    GLState::glsDisable(GL_STENCIL_TEST);
    GLState::glsCullFace(GL_BACK);

    bool directional_lights;
    GET_SETTING("directional_lights", bool, directional_lights);
    if (directional_lights) {
      // Render light contributions into the accumulation buffer: dir lights
      for (uint32_t i = 0; i < lights_.size(); i++) {
        if (lights_[i]->on() && lights_[i]->type() == LIGHT_DIR) {
          LightDir* light = reinterpret_cast<LightDir*>(lights_[i]);
          lightDirAccumPass(light);
        }
      }
    }
    GLState::glsDisable(GL_BLEND);
  }

  // Point lights
  void Lighting::lightPointStencilPass(const LightPoint* light) const {
    LightGeometryStencilPass(light_geom_point_, light->pvw_mat());
  }

  void Lighting::lightPointAccumPass(const LightPoint* light, 
    const bool stencil_optim) const {
    ShaderProgram::useShaderProgram(LIGHT_VOLUME_V_SHADER,
      LIGHT_VOLUME_POINT_ACCUM_F_SHADER);

    light->setHandles();  // Set the light parameters (color, intensity, etc)

    bool inside = false;
    if (!stencil_optim) {
      inside = light->cameraInside(renderer_->camera());
    }

    lightGeomAccumPass(light_geom_point_, light->pvw_mat(), stencil_optim,
      inside);
  }

  // Spot lights
  void Lighting::lightSpotStencilPass(const LightSpot* light) const {
    LightGeometryStencilPass(light_geom_spot_, light->pvw_mat());
  }

  void Lighting::lightSpotAccumPass(const LightSpot* light,
    const bool stencil_optim) const {
    ShaderProgram::useShaderProgram(LIGHT_VOLUME_V_SHADER,
      LIGHT_VOLUME_SPOT_ACCUM_F_SHADER);

    light->setHandles();  // Set the light parameters (color, intensity, etc)

    bool inside = false;
    if (!stencil_optim) {
      inside = light->cameraInside(renderer_->camera());
    }

    lightGeomAccumPass(light_geom_spot_, light->pvw_mat(), stencil_optim,
      inside);
  }

  // Spot lights with variance shadow mapping (no cascade)
  void Lighting::LightSpotVSMAccumPass(const LightSpotCVSM* light,
    const bool stencil_optim) const {
    TextureType type = shadow_map_->type();
    if (type != TEXTURE_RENDERABLE_ARRAY_TYPE && 
      type != TEXTURE_RENDERABLE_TYPE) {
      throw std::wruntime_error("Lighting::LightSpotVSMAccumPass() - INTERNAL "
        "ERROR: Incorrect shadow map type");
    }
    if (type == TEXTURE_RENDERABLE_TYPE) {
      ShaderProgram::useShaderProgram(LIGHT_VOLUME_V_SHADER,
        LIGHT_VOLUME_SPOT_VSM_ACCUM_F_SHADER);
    } else {
      ShaderProgram::useShaderProgram(LIGHT_VOLUME_V_SHADER,
        LIGHT_VOLUME_SPOT_CVSM_CNT1_ACCUM_F_SHADER);
    }

    if (type == TEXTURE_RENDERABLE_TYPE) {
      ((TextureRenderable*)shadow_map_)->bind(0, GL_TEXTURE3, "f_vsm");
    } else {
      ((TextureRenderableArray*)shadow_map_)->bind(0, GL_TEXTURE3, 
        "f_vsm_array");
    }

    // Lots of shadow map uniforms:
    float vsm_min_variance, vsm_lbr_amount;
    GET_SETTING("vsm_min_variance", float, vsm_min_variance);
    GET_SETTING("vsm_lbr_amount", float, vsm_lbr_amount);

    BIND_UNIFORM("f_vsm_split_pv_camviewinv", vsm_split_pv_camviewinv_float_);
    BIND_UNIFORM("f_vsm_min_variance", &vsm_min_variance);
    BIND_UNIFORM("f_lbr_amount", &vsm_lbr_amount);
    BIND_UNIFORM("f_light_near_far", light->near_far_vsm().m);

    light->setHandles();  // Set the light parameters (color, intensity, etc)

    bool inside = false;
    if (!stencil_optim) {
      inside = light->cameraInside(renderer_->camera());
    }

    lightGeomAccumPass(light_geom_spot_, light->pvw_mat(), stencil_optim,
      inside);
  }

  // Spot lights with cascaded variance shadow mapping
  void Lighting::LightSpotCVSMAccumPass(const LightSpotCVSM* light,
    const bool stencil_optim) const {
    TextureType type = shadow_map_->type();
    if (type != TEXTURE_RENDERABLE_ARRAY_TYPE) {
      throw std::wruntime_error("Lighting::LightSpotCVSMAccumPass() - INTERNAL"
        " ERROR: Incorrect shadow map type");
    }
    ShaderProgram::useShaderProgram(LIGHT_VOLUME_V_SHADER,
      LIGHT_VOLUME_SPOT_CVSM_ACCUM_F_SHADER);

    ((TextureRenderableArray*)shadow_map_)->bind(0, GL_TEXTURE3, 
      "f_vsm_array");

    vsm_splits_->bind(0, GL_TEXTURE4, "f_vsm_splits");

    // Lots of shadow map uniforms:
    float vsm_min_variance, vsm_blend_zone, vsm_lbr_amount;
    GET_SETTING("vsm_min_variance", float, vsm_min_variance);
    GET_SETTING("vsm_lbr_amount", float, vsm_lbr_amount);
    GET_SETTING("vsm_blend_zone", float, vsm_blend_zone);

    BIND_UNIFORM("f_vsm_split_pv_camviewinv", vsm_split_pv_camviewinv_float_);
    BIND_UNIFORM("f_vsm_split_depths", vsm_split_depths_);
    BIND_UNIFORM("f_vsm_count", &light->cvsm_count());
    BIND_UNIFORM("f_vsm_min_variance", &vsm_min_variance);
    BIND_UNIFORM("vsm_lbr_amount", &vsm_lbr_amount);
    BIND_UNIFORM("f_light_near_far", light->near_far_vsm().m);
    BIND_UNIFORM("f_vsm_blend_zone", &vsm_blend_zone);

    light->setHandles();  // Set the light parameters (color, intensity, etc)

    bool inside = false;
    if (!stencil_optim) {
      inside = light->cameraInside(renderer_->camera());
    }

    lightGeomAccumPass(light_geom_spot_, light->pvw_mat(), stencil_optim,
      inside);
  }

  // Directional light
  void Lighting::lightDirAccumPass(const LightDir* light) const {
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER,
      LIGHT_DIR_ACCUM_F_SHADER);

    light->setHandles();  // Set the light parameters (color, intensity, etc)

    renderer_->g_buffer()->g_buffer_texture()->beginLightAccumPass();

    GLState::glsDisable(GL_DEPTH_TEST);
    GLState::glsDisable(GL_CULL_FACE);
    GLState::glsEnable(GL_BLEND);
    GLState::glsBlendEquation(GL_FUNC_ADD);
    GLState::glsBlendFunc(GL_ONE, GL_ONE);

    BIND_UNIFORM("f_screen_size", renderer_->camera()->screen_size().m);
    BIND_UNIFORM("f_inv_focal_length", 
      renderer_->camera()->inv_focal_length().m);
    BIND_UNIFORM("f_camera_far", &renderer_->camera()->near_far()[1]);

    renderer_->post_processing()->quad()->draw();

    renderer_->g_buffer()->g_buffer_texture()->endLightAccumPass();
  }

  // Generic light (shared code)
  void Lighting::LightGeometryStencilPass(const Geometry* geom, 
    const Float4x4& pvw_mat) const {
    GLState::glsPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    GLState::glsDepthMask(GL_FALSE);

    ShaderProgram::useShaderProgram(LIGHT_VOLUME_V_SHADER,
      LIGHT_VOLUME_STENCIL_F_SHADER);
    renderer_->g_buffer()->g_buffer_texture()->beginLightStencilPass();

    GLState::glsEnable(GL_DEPTH_TEST);
    GLState::glsDisable(GL_CULL_FACE);
    GLState::glsEnable(GL_STENCIL_TEST);
    
    // GLState::glsStencilMask(0xff);
    GLState::glsClear(GL_STENCIL_BUFFER_BIT);

    // We need the stencil test to be enabled but we want it
		// to succeed always. Only the depth test matters.
    GLState::glsStencilFunc(GL_ALWAYS, 0, 0);

    GLState::glsStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, 
      GL_KEEP);
    GLState::glsStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, 
      GL_KEEP);

    BIND_UNIFORM("pvw_mat", pvw_mat.m);

    // Render the light object:
    geom->draw();

    renderer_->g_buffer()->g_buffer_texture()->endLightStencilPass();
  }

  void Lighting::lightGeomAccumPass(const Geometry* geom, 
    const Float4x4& pvw_mat, const bool stencil_optim, 
    const bool camera_inside) const {
    // Assumes that light handles are set and the correct shader has been
    // selected
      
    // Bind the texture samplers to the correct g buffer textures
    renderer_->g_buffer()->g_buffer_texture()->beginLightAccumPass();
    
    if (stencil_optim) {
      GLState::glsEnable(GL_STENCIL_TEST);
      // GLState::glsStencilMask(0x00);
      GLState::glsStencilFunc(GL_EQUAL, 1, 0xFF);  // Stencil does not equal zero
      GLState::glsDisable(GL_DEPTH_TEST);
      GLState::glsEnable(GL_CULL_FACE);
      GLState::glsCullFace(GL_FRONT);
    } else {
      // Not using a stencil pass: Just render the light volume using back
      // faces.  Will overdraw, but only within the light extent.
      if (camera_inside) {
        GLState::glsDisable(GL_DEPTH_TEST);
        GLState::glsEnable(GL_CULL_FACE);
        GLState::glsCullFace(GL_FRONT);
      } else {
        GLState::glsEnable(GL_DEPTH_TEST);
        GLState::glsEnable(GL_CULL_FACE);
        GLState::glsCullFace(GL_BACK);
      }
    }

    GLState::glsEnable(GL_BLEND);
    GLState::glsBlendEquation(GL_FUNC_ADD);
    GLState::glsBlendFunc(GL_ONE, GL_ONE);
    GLState::glsPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    BIND_UNIFORM("pvw_mat", pvw_mat.m);
    BIND_UNIFORM("f_screen_size", renderer_->camera()->screen_size().m);
    BIND_UNIFORM("f_inv_focal_length", 
      renderer_->camera()->inv_focal_length().m);
    BIND_UNIFORM("f_camera_far", &renderer_->camera()->near_far()[1]);

    // Render the light object:
    geom->draw();

    renderer_->g_buffer()->g_buffer_texture()->endLightAccumPass();
    
  }

  void Lighting::renderLightFinalPass() {
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER,
      LIGHT_FINAL_F_SHADER);
    GLState::setupQuadRendering();

    renderer_->g_buffer()->final_scene()->begin();
    // beginLightFinalPass sets textures 0 through 2
    renderer_->g_buffer()->g_buffer_texture()->beginLightFinalPass();

    float global_ambient;
    GET_SETTING("global_ambient", float, global_ambient);
    BIND_UNIFORM("f_global_ambient", &global_ambient);

    ambient_->bind(0, GL_TEXTURE3, "f_ambient_occ");

    renderer_->post_processing()->quad()->draw();

    renderer_->g_buffer()->g_buffer_texture()->endLightFinalPass();
    renderer_->g_buffer()->final_scene()->end();
  }

  void Lighting::renderAmbientOcclusionPass() {
    bool ssao_on;
    GET_SETTING("ssao_on", bool, ssao_on);

    if (!ambient_cleared_ || ssao_on) {
      // Clear the ambient occlusion buffer
      ambient_->begin();
      GLenum DrawBuffers[] = {GL_COLOR_ATTACHMENT0};
      GLState::glsDrawBuffers(1, DrawBuffers);

      GLState::glsClearColor(1, 1, 1, 1);
      GLState::glsClear(GL_COLOR_BUFFER_BIT);

      ambient_cleared_ = true;

      if (!ssao_on) {
        ambient_->end();
      }
    }

    // Now calculate the ambient occlusion (if we want)
    if (ssao_on) {
      ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER,
        LIGHT_AMBIENT_OCCLUSION_F_SHADER);

      GLState::setupQuadRendering();
      renderer_->g_buffer()->g_buffer_texture()->beginAmbientOcclusionPass();

      BIND_UNIFORM("f_screen_size", renderer_->camera()->screen_size().m);
      BIND_UNIFORM("f_inv_focal_length", 
        renderer_->camera()->inv_focal_length().m);

      vector_noise_tex_->bind(GL_TEXTURE1, "f_vector_noise");
      Float2 vector_noise_size(1.0f / (float)(vector_noise_tex_->w()), 
        1.0f / (float)(vector_noise_tex_->h()));
      BIND_UNIFORM("f_one_over_vector_noise_size", vector_noise_size.m);

      float ssao_scale;
      GET_SETTING("ssao_scale", float, ssao_scale);
      BIND_UNIFORM("f_ssao_scale", &ssao_scale);

      float ssao_bias;
      GET_SETTING("ssao_bias", float, ssao_bias);
      BIND_UNIFORM("f_ssao_bias", &ssao_bias);

      float ssao_intensity;
      GET_SETTING("ssao_intensity", float, ssao_intensity);
      BIND_UNIFORM("f_ssao_intensity", &ssao_intensity);

      float ssao_sample_radius;
      GET_SETTING("ssao_sample_radius", float, ssao_sample_radius);
      BIND_UNIFORM("f_ssao_sample_radius", &ssao_sample_radius);

      renderer_->post_processing()->quad()->draw();

      ambient_cleared_ = false;

      ambient_->end();
      renderer_->g_buffer()->g_buffer_texture()->endAmbientOcclusionPass();

      bool ssao_blur_on;
      GET_SETTING("ssao_blur_on", bool, ssao_blur_on);
      if (ssao_blur_on) {
        blurAmbientOcclusion();
      }
    }
  }

  void Lighting::renderSpotVSM(const LightSpotCVSM* light) {

    cur_light_spot_vsm_ = light;
    cur_lighting_ = this;

    // Compute the maximum LOD from the softness setting for this shadow map.  
    // Note that this effectively makes LOD a logarithmic slider of filter 
    // width, which is convenient and intuitive.
	  float dim = (float)std::max<uint32_t>(shadow_map_->w(), shadow_map_->h());
    vsm_max_lod_ = light->softness() * logf(dim) / log(2.0f);
	  vsm_min_filter_width_ = powf(2.0f, vsm_max_lod_);

    math::Float4x4::multSIMD(vsm_split_pv_camviewinv_[0], light->pv_mat_vsm(),
      renderer_->camera()->view_inv());
    memcpy(vsm_split_pv_camviewinv_float_, vsm_split_pv_camviewinv_[0].m,
      sizeof(vsm_split_pv_camviewinv_float_[0])*16);
    vsm_split_depths_[0] = renderer_->camera()->near_far()[0];
    vsm_split_depths_[1] = renderer_->camera()->near_far()[1];
    vsm_split_scales_[0].set(1, 1);
    
    GLState::glsEnable(GL_DEPTH_TEST);
    GLState::glsDepthMask(GL_TRUE);
    // cull front - gets rid of self shadowing
    GLState::glsCullFace(GL_FRONT);
    GLState::glsEnable(GL_CULL_FACE);
    GLState::glsDisable(GL_STENCIL_TEST);
    GLState::glsDisable(GL_BLEND);
    
    vsm_render_pass_->shader_uniform_cb() = &this->renderSpotVSMUniformCB;

    if (shadow_map_->type() == TextureType::TEXTURE_RENDERABLE_TYPE) {
      ((TextureRenderable*)shadow_map_)->begin();
    } else {
      ((TextureRenderableArray*)shadow_map_)->begin(0);
    }

    GLState::glsClearColor(0, 0, 0, 0);
    GLState::glsClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vsm_render_pass_->view() = &light->v_mat_vsm();
    vsm_render_pass_->proj() = &light->p_mat_vsm();
    vsm_frustum_->Set(light->pv_mat_vsm().m);
    vsm_render_pass_->frustum() = vsm_frustum_;

    if (shadow_map_->type() == TextureType::TEXTURE_RENDERABLE_TYPE) {
      vsm_render_pass_->screen_size() = 
        &((TextureRenderable*)shadow_map_)->screen_size();
    } else {
      vsm_render_pass_->screen_size() = 
        &((TextureRenderableArray*)shadow_map_)->screen_size();
    }

    vsm_render_pass_->render();

    if (shadow_map_->type() == TextureType::TEXTURE_RENDERABLE_TYPE) {
      ((TextureRenderable*)shadow_map_)->end();
    } else {
      ((TextureRenderableArray*)shadow_map_)->end();
    }

    GLState::glsDepthMask(GL_FALSE);
  }

  void Lighting::renderSpotCVSM(const LightSpotCVSM* light) {
    cur_light_spot_vsm_ = light;
    cur_lighting_ = this;

    // Compute the maximum LOD from the softness setting for this shadow map.  
    // Note that this effectively makes LOD a logarithmic slider of filter 
    // width, which is convenient and intuitive.
	  float dim = (float)std::max<uint32_t>(shadow_map_->w(), shadow_map_->h());
    vsm_max_lod_ = light->softness() * logf(dim) / log(2.0f);
	  vsm_min_filter_width_ = powf(2.0f, vsm_max_lod_);

    calcCVSMSplitMats(light);

    GLState::glsEnable(GL_DEPTH_TEST);
    GLState::glsDepthMask(GL_TRUE);
    // cull front - gets rid of self shadowing
    GLState::glsCullFace(GL_FRONT);
    GLState::glsEnable(GL_CULL_FACE);
    GLState::glsDisable(GL_STENCIL_TEST);
    GLState::glsDisable(GL_BLEND);

    GLState::glsClearColor(0, 0, 0, 0);
    GLState::glsClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (shadow_map_->type() == TextureType::TEXTURE_RENDERABLE_TYPE) {
      throw std::wruntime_error("Lighting::renderSpotCVSM() - ERROR: "
        "Cascaded shadowmap support only for TextureRenderableArray!");
    }

    vsm_render_pass_->shader_uniform_cb() = &this->renderSpotCVSMUniformCB;

    for (uint32_t i = 0; i < light->cvsm_count(); i++) {
      ((TextureRenderableArray*)shadow_map_)->begin(i);

      vsm_render_pass_->view() = &light->v_mat_vsm();
      vsm_render_pass_->proj() = &vsm_split_proj_[i];
      vsm_frustum_->Set(vsm_split_pv_[i].m);
      vsm_render_pass_->frustum() = vsm_frustum_;
      vsm_render_pass_->screen_size() = 
        &((TextureRenderableArray*)shadow_map_)->screen_size();

      vsm_render_pass_->render();
      ((TextureRenderableArray*)shadow_map_)->end();
    }

    GLState::glsDepthMask(GL_FALSE);
  }

  void Lighting::renderSpotCVSMUniformCB() {
    float vsm_depth_epsilon;
    GET_SETTING("vsm_depth_epsilon", float, vsm_depth_epsilon);

    BIND_UNIFORM("f_vsm_depth_epsilon", &vsm_depth_epsilon);
    BIND_UNIFORM("f_light_near_far", 
      cur_light_spot_vsm_->near_far_vsm().m);
  }

  void Lighting::renderSpotVSMUniformCB() {
    float vsm_depth_epsilon;
    GET_SETTING("vsm_depth_epsilon", float, vsm_depth_epsilon);

    BIND_UNIFORM("f_vsm_depth_epsilon", &vsm_depth_epsilon);
    BIND_UNIFORM("f_light_near_far", 
      cur_light_spot_vsm_->near_far_vsm().m);
  }

  void Lighting::calcCVSMSplitDepths(const uint32_t csm_count) {
    Float2& near_far = renderer_->camera()->near_far();
    float lambda;
    GET_SETTING("vsm_cascade_split_log_factor", float, lambda);

    // Implements the "practical" split scheme from the PSSM paper
    float split_range = near_far[1] - near_far[0];
    float split_ratio = near_far[1] / near_far[0];

    if (csm_count > LIGHTING_CVSM_MAX_COUNT || csm_count < 1) {
      throw wruntime_error("csm_count > LIGHTING_CVSM_MAX_COUNT!");
    }

    uint32_t i;
    for (i = 0; i < static_cast<uint32_t>(csm_count); ++i) {
      float p = (float)i / (float)csm_count;
      float log_split = near_far[0] * pow(split_ratio, p);
      float uniform_split = near_far[0] + split_range * p;
      // Lerp between the two schemes
      float split = lambda * (log_split - uniform_split) + uniform_split;
      vsm_split_depths_[i] = split;
    }

    // Just for simplicty later, push the camera far plane as the last "split"
    vsm_split_depths_[i] = near_far[1];
    i++;

    // Fill in the rest with a large number
    for (; i < LIGHTING_CVSM_MAX_COUNT + 1; i++) {
      vsm_split_depths_[i] = -(std::numeric_limits<float>::max)();
    }
  }

  // Some of the code here is taken from chapter 8 sample code of GPU Gems 3
  void Lighting::calcCVSMSplitMats(const LightSpotCVSM* light) {
    // Compute the minimum filter width in normalized device coordinates 
    // ([-1, 1]).  NOTE: We compute *half* of this width here for modifying a 
    // region later.
    float sm_size = static_cast<float>(shadow_map_->w());
    Float2 half_min_filter_width_ndc;
    half_min_filter_width_ndc[0] = vsm_min_filter_width_ / sm_size;
    half_min_filter_width_ndc[1] = vsm_min_filter_width_ / sm_size;

    // Extract some useful camera-related information
    const Float4x4& camera_view_inv = renderer_->camera()->view_inv();
    const Float4x4& camera_proj = renderer_->camera()->proj();

    float x_scale_inv = 1.0f / camera_proj(0, 0);
    float y_scale_inv = 1.0f / camera_proj(1, 1);

    // Construct a matrix that transforms from camera view space into projected
    // light space:
    Float4x4 view_2_proj_light_space;
    Float4x4::multSIMD(view_2_proj_light_space, 
      light->pv_mat_vsm(), camera_view_inv);

    float vsm_blend_zone;
    GET_SETTING("vsm_blend_zone", float, vsm_blend_zone);
    float vsm_overlap;
    GET_SETTING("vsm_overlap", float, vsm_overlap);

    Float4 corners[8];
    Float4 corners_proj[8];
    Float4x4 zoom;
    Float4x4 temp1;
    for (uint32_t i = 0; i < light->cvsm_count(); ++i) {
      // Compute corners for this frustum
      // Need to overlap min into blend zone for all splits OTHER than the 1st
      float pnear;
      if (i > 0) {
        // last_split_distance = distance of the split BEFORE this one
        float last_split_distance = vsm_split_depths_[i] - 
          vsm_split_depths_[i-1];
        pnear = vsm_split_depths_[i] - (last_split_distance * vsm_blend_zone);
      } else {
        pnear = vsm_split_depths_[i];
      }
      float pfar  = vsm_split_depths_[i+1];

      float splitDistance = pfar - pnear;
      pnear = pnear - splitDistance * vsm_overlap;
      pfar = pfar + splitDistance * vsm_overlap;

      // near corners (in view space)
      float nx = x_scale_inv * pnear;
      float ny = y_scale_inv * pnear;
      corners[0].set(-nx, ny, pnear, 1.0f);
      corners[1].set(nx, ny, pnear, 1.0f);
      corners[2].set(-nx, -ny, pnear, 1.0f);
      corners[3].set(nx, -ny, pnear, 1.0f);
      // Far corners (in view space)
      float fx = x_scale_inv * pfar;
      float fy = y_scale_inv * pfar;
      corners[4].set(-fx, fy, pfar, 1.0f);
      corners[5].set(fx, fy, pfar, 1.0f);
      corners[6].set(-fx, -fy, pfar, 1.0f);
      corners[7].set(fx, -fy, pfar, 1.0f);

      // Transform corners into projected light space and then transform into
      // NDC (note this step is slightly different for DirectX)
      for (uint32_t j = 0; j < 8; j++) {
        Float4::mult(corners_proj[j], view_2_proj_light_space, corners[j]);
        Float4::scale(corners_proj[j], 1.0f / corners_proj[j][3]);
      }

      // TODO: Adjust near/Far and corresponding depth scaling
      Float2 pmin(1.0f, 1.0f);
      Float2 pmax(-1.0f, -1.0f);
      for (uint32_t c = 0; c < 8; ++c) {
        // Homogenious divide x and y
        const Float4& p = corners_proj[c];

        // OpenGL NDC w value is from -1 --> 1 by convention
        // DirectX NDC w value is from 0 --> 1
        if (p[2] < -1.0f) {
          // In front of near clipping plane! Be conservative...
          pmin.set(-1, -1);
          pmax.set(1, 1);
          break;
        } else {
          Float2 v(p[0], p[1]);
          // Update boundaries
          (Float2::min)(pmin, pmin, v);
          (Float2::max)(pmax, pmax, v);
        }
      }

      // Degenerate slice?
      Float2 dim;
      Float2::sub(dim, pmax, pmin);
      if (pmax[0] <= -1.0f || pmax[1] <= -1.0f || pmin[0] >= 1.0f || 
        pmin[1] >= 1.0f || dim[0] <= EPSILON || dim[1] <= EPSILON) {
        // TODO: Something better... (skip this slice)
        pmin.set(-1, -1);
        pmax.set(1,  1);
      }

      // TODO: Clamp extreme magnifications, since they will cause gigantic 
      // blurs. Not an issue if we were using PSSAVSM though (i.e. summed-area 
      // tables)

      // Expand region by minimum filter width in each dimension to make sure 
      // thatwe can blur properly and get adjacent geometry.
      Float2::sub(pmin, pmin, half_min_filter_width_ndc);
      Float2::add(pmax, pmax, half_min_filter_width_ndc);

      // Clamp to valid range
      pmin[0] = std::min<float>(1.0f, std::max<float>(-1.0f, pmin[0]));
      pmin[1] = std::min<float>(1.0f, std::max<float>(-1.0f, pmin[1]));
      pmax[0] = std::min<float>(1.0f, std::max<float>(-1.0f, pmax[0]));
      pmax[1] = std::min<float>(1.0f, std::max<float>(-1.0f, pmax[1]));

      // Compute scale and offset
      Float2 scale;
      scale[0] = 2.0f / (pmax[0] - pmin[0]);
      scale[1] = 2.0f / (pmax[1] - pmin[1]);
      Float2 offset;
      offset[0] = -0.5f * (pmax[0] + pmin[0]) * scale[0];
      offset[1] = -0.5f * (pmax[1] + pmin[1]) * scale[1];

      // Store scale factors for later use when blurring
      vsm_split_scales_[i].set(scale[0], scale[1]); 

      // Adjust projection matrix to "zoom in" on the target region  
      zoom.set(scale[0], 0.0f, 0.0f, 0.0f,  // TO DO: Check this
               0.0f, scale[1], 0.0f, 0.0f,
               0.0f, 0.0f, 1.0f, 0.0f,
               offset[0], offset[1], 0.0f, 1.0f);

      // Compute new composite matrices and store
      // TO DO: Check order!
      math::Float4x4::multSIMD(vsm_split_proj_[i], zoom, light->p_mat_vsm());  

      // Update update to include inverse camera as well
      Float4x4::multSIMD(vsm_split_pv_[i], light->p_mat_vsm(), light->v_mat_vsm());
      Float4x4::multSIMD(vsm_split_pv_camviewinv_[i], vsm_split_pv_[i], 
        camera_view_inv);

      // We need a flat float array, so transfer it into a float buffer
      memcpy(&vsm_split_pv_camviewinv_float_[i * 16], 
        vsm_split_pv_camviewinv_[i].m, 
        sizeof(vsm_split_pv_camviewinv_[i].m[0]) * 16);
    }  // for (uint32_t  i = 0; i < vsm_count; ++i) 
  }

  void Lighting::renderCVSMSplitTexture(const uint32_t vsm_count) const {
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      CALC_CSM_SPLIT_F_SHADER);
    GLState::setupQuadRendering();

    vsm_splits_->begin();
    renderer_->g_buffer()->g_buffer_texture()->beginAmbientOcclusionPass();

    BIND_UNIFORM("f_vsm_split_depths", vsm_split_depths_);
    BIND_UNIFORM("f_vsm_count", &vsm_count);

    renderer_->post_processing()->quad()->draw();

    renderer_->g_buffer()->g_buffer_texture()->endAmbientOcclusionPass();
    vsm_splits_->end();
  }

  void Lighting::drawVSM() const {
    TextureType type = shadow_map_->type();
    if (type != TEXTURE_RENDERABLE_ARRAY_TYPE && 
      type != TEXTURE_RENDERABLE_TYPE) {
      throw std::wruntime_error("Lighting::drawVSM() - INTERNAL ERROR: "
        "Incorrect shadow map type");
    }
    if (type == TEXTURE_RENDERABLE_TYPE){
      ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
        DRAW_VSM_F_SHADER);
    } else {
      ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
        DRAW_CVSM_F_SHADER);
    }
    GLState::setupQuadRendering();
    GLState::glsClearColor(0, 0, 0, 1);
    GLState::glsClear(GL_COLOR_BUFFER_BIT);

    if (type == TextureType::TEXTURE_RENDERABLE_ARRAY_TYPE) {
      ((TextureRenderableArray*)shadow_map_)->bind(0, GL_TEXTURE0, "f_vsm_array");
      uint32_t w = renderer_->width();
      uint32_t h = renderer_->height();
      uint32_t tilex = (uint32_t)floorf(sqrtf(LIGHTING_CVSM_MAX_COUNT));
      uint32_t tiley = LIGHTING_CVSM_MAX_COUNT / tilex;
      uint32_t pix_x = w / tilex;
      uint32_t pix_y = h / tilex;

      int i_sm = 0;
      for (uint32_t x = 0; x < tilex && i_sm < LIGHTING_CVSM_MAX_COUNT; x++) {
        for (uint32_t y = 0; y < tiley && i_sm < LIGHTING_CVSM_MAX_COUNT; y++) {
          BIND_UNIFORM("sm_index", &i_sm);
          GLState::glsViewport(x * pix_x, y * pix_y, pix_x, pix_y);
          renderer_->post_processing()->quad()->draw();
          i_sm++;
        }
      }
    
      // Put the viewport back to normal for UI rendering:
      GLState::glsViewport(0, 0, w, h);
    } else {
      ((TextureRenderable*)shadow_map_)->bind(0, GL_TEXTURE0, "f_vsm");
      renderer_->post_processing()->quad()->draw();
    }
  }

  void Lighting::visualizeCVSMSplit() const {
    bool vsm_visualize_split, vsm_on;
    GET_SETTING("vsm_visualize_split", bool, vsm_visualize_split);
    GET_SETTING("vsm_on", bool, vsm_on);

    if (vsm_on && vsm_visualize_split) {
      ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER,
        VISUALIZE_CSM_SPLIT_F_SHADER);
      GLState::setupQuadRendering();

      GLState::glsEnable(GL_BLEND);
      GLState::glsBlendEquation(GL_FUNC_ADD);
      GLState::glsBlendFunc(GL_SRC_ALPHA, 
        GL_ONE_MINUS_SRC_ALPHA);

      renderer_->g_buffer()->final_scene()->begin();

      const float f_vsm_split_alpha = 0.5f;
      BIND_UNIFORM("f_vsm_split_alpha", &f_vsm_split_alpha);

      vsm_splits_->bind(0, GL_TEXTURE0, "f_vsm_splits");

      renderer_->post_processing()->quad()->draw();

      renderer_->g_buffer()->final_scene()->end();
    }
  }

  void Lighting::blurShadowMap(const uint32_t csm_count) const {
    bool vsm_soft_shadows;
    GET_SETTING("vsm_soft_shadows", bool, vsm_soft_shadows);
    if (vsm_soft_shadows) {
      if (shadow_map_->type() == TEXTURE_RENDERABLE_ARRAY_TYPE &&
        shadow_map_blur_temp_->type() == TEXTURE_RENDERABLE_ARRAY_TYPE) {
        for (uint32_t i = 0; i < csm_count; i++) {
          Float2 split_min_filter_width_(vsm_split_scales_[i]);
          Float2::scale(split_min_filter_width_, vsm_min_filter_width_);
		      int HorizFilterSamples = (int)floor(split_min_filter_width_[0] + 0.5f);

          renderer_->post_processing()->rectBlur((TextureRenderableArray*)shadow_map_, 
            (TextureRenderableArray*)shadow_map_blur_temp_, HorizFilterSamples, i, 0);
        }
      } else if (shadow_map_->type() == TEXTURE_RENDERABLE_TYPE &&
        shadow_map_blur_temp_->type() == TEXTURE_RENDERABLE_TYPE) {
          Float2 split_min_filter_width_(vsm_split_scales_[0]);
          Float2::scale(split_min_filter_width_, vsm_min_filter_width_);
		      int HorizFilterSamples = (int)floor(split_min_filter_width_[0] + 0.5f);
          renderer_->post_processing()->rectBlur((TextureRenderable*)shadow_map_, 
            (TextureRenderable*)shadow_map_blur_temp_, HorizFilterSamples, 0);
      } else {
        throw std::wruntime_error("Lighting::blurShadowMap() - ERROR: "
          "Incorrect shadowmap texture type!");
      }
    }
  }

  void Lighting::blurAmbientOcclusion() const {
    // horizontal blur
    ShaderProgram::useShaderProgram(FULLSCREEN_QUAD_V_SHADER, 
      LIGHT_AMBIENT_OCCLUSION_BLUR_F_SHADER);

    GLState::setupQuadRendering();

    renderer_->g_buffer()->g_buffer_texture()->beginAmbientOcclusionPass();
    ambient_blur_temp_->begin();

    ambient_->bind(0, GL_TEXTURE4, "f_ssao_buffer");

    math::Float2 texel_size(1.0f / (float)ambient_->screen_size()[0], 0.0f);
    BIND_UNIFORM("f_texel_size", texel_size.m);
   
    renderer_->post_processing()->quad()->draw();
    ambient_blur_temp_->end();

    // vertical blur
    ambient_->begin();
    ambient_blur_temp_->bind(0, GL_TEXTURE4, "f_ssao_buffer");
    texel_size.set(0, 1.0f / (float)ambient_blur_temp_->screen_size()[1]);
    BIND_UNIFORM("f_texel_size", texel_size.m);

    renderer_->post_processing()->quad()->draw();
    ambient_->end();
    renderer_->g_buffer()->g_buffer_texture()->endAmbientOcclusionPass();
  }
}  // namespace renderer
}  // namespace jtil

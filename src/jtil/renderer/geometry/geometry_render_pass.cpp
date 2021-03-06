#include <string>
#include <iostream>
#include "jtil/renderer/geometry/geometry_render_pass.h"
#include "jtil/renderer/geometry/geometry_manager.h"
#include "jtil/renderer/geometry/geometry_instance.h"
#include "jtil/renderer/geometry/geometry.h"
#include "jtil/renderer/renderer.h"
#include "jtil/renderer/shader/shader_program.h"
#include "jtil/settings/settings_manager.h"
#include "jtil/renderer/lights/light_spot.h"
#include "jtil/renderer/lights/light_point.h"
#include "jtil/renderer/lighting.h"
#include "jtil/renderer/camera/frustum.h"
#include "jtil/renderer/objects/aabbox.h"
#include "jtil/renderer/gl_state.h"

using std::string;
using std::wruntime_error;

namespace jtil {

using math::Float4x4;
using math::Float3;
using renderer::objects::AABBox;

#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }

namespace renderer {

  GeometryRenderPass::GeometryRenderPass(Renderer* renderer) {
    frustum_ = NULL;
    renderer_ = renderer;

    // Initialize space for all the shaders and set them to NULL
    for (uint32_t i = 0; i < NUM_VERTEX_PRIMATIVES_SUPORTED; i++) {
      v_shaders_[i].capacity(NUM_GEOMETRY_TYPES_SUPORTED);
      v_shaders_[i].resize(NUM_GEOMETRY_TYPES_SUPORTED);
      memset(v_shaders_[i].at(0), NULL, sizeof(v_shaders_[i].at(0)) * 
        NUM_GEOMETRY_TYPES_SUPORTED);
      f_shaders_[i].capacity(NUM_GEOMETRY_TYPES_SUPORTED);
      f_shaders_[i].resize(NUM_GEOMETRY_TYPES_SUPORTED);
      memset(f_shaders_[i].at(0), NULL, sizeof(f_shaders_[i].at(0)) * 
        NUM_GEOMETRY_TYPES_SUPORTED);
      g_shaders_[i].capacity(NUM_GEOMETRY_TYPES_SUPORTED);
      g_shaders_[i].resize(NUM_GEOMETRY_TYPES_SUPORTED);
      memset(g_shaders_[i].at(0), NULL, sizeof(g_shaders_[i].at(0)) * 
        NUM_GEOMETRY_TYPES_SUPORTED);
      tcs_shaders_[i].capacity(NUM_GEOMETRY_TYPES_SUPORTED);
      tcs_shaders_[i].resize(NUM_GEOMETRY_TYPES_SUPORTED);
      memset(tcs_shaders_[i].at(0), NULL, sizeof(tcs_shaders_[i].at(0)) * 
        NUM_GEOMETRY_TYPES_SUPORTED);
      tes_shaders_[i].capacity(NUM_GEOMETRY_TYPES_SUPORTED);
      tes_shaders_[i].resize(NUM_GEOMETRY_TYPES_SUPORTED);
      memset(tes_shaders_[i].at(0), NULL, sizeof(tes_shaders_[i].at(0)) * 
        NUM_GEOMETRY_TYPES_SUPORTED);
    }

    render_aabboxes_ = false;
    render_light_volumes_ = true;
    render_light_sources_ = true;

    shader_uniform_cb_ = NULL;

    view_ = NULL;
    proj_ = NULL;
    frustum_ = NULL;
    screen_size_ = NULL;
  }

  GeometryRenderPass::~GeometryRenderPass() {
    // Explicitly clear out the strings
    for (uint32_t i = 0; i < NUM_VERTEX_PRIMATIVES_SUPORTED; i++) {
      v_shaders_[i].clear();
      f_shaders_[i].clear();
      g_shaders_[i].clear();
      tcs_shaders_[i].clear();
      tes_shaders_[i].clear();
    }
  }

  // TODO: Get rid of this function!  Maybe use a hash map
  GeometryRenderPass::GeometryTypeIndex 
    GeometryRenderPass::getGeometryTypeIndex(const GeometryType type) const {
    switch (type) {
    case GEOMETRY_COLR:
      return INDEX_COLR;
    case GEOMETRY_NORM_COLR:
      return INDEX_NORM_COLR;
    case GEOMETRY_NORM_COLR_BONED:
      return INDEX_NORM_COLR_BONED;
    case GEOMETRY_NORM_CONST_COLR:
      return INDEX_NORM_CONST_COLR;
    case GEOMETRY_NORM_CONST_COLR_BONED:
      return INDEX_NORM_CONST_COLR_BONED;
    case GEOMETRY_NORM_TEXT:
      return INDEX_NORM_TEXT;
    case GEOMETRY_NORM_TEXT_BONED:
      return INDEX_NORM_TEXT_BONED;
    case GEOMETRY_NORM_TEXT_DISP:
      return INDEX_NORM_TEXT_DISP;
    default:
      throw std::wruntime_error("GeometryRenderPass::getGeometryTypeIndex()"
        " - ERROR: index not recognized or not supported.");
    }
  }

  // TODO: Get rid of this function!  Maybe use a hash map
  GeometryType GeometryRenderPass::getGeometryType(
    const GeometryRenderPass::GeometryTypeIndex index) const {
    switch (index) {
    case INDEX_NORM_COLR:
      return GEOMETRY_NORM_COLR;
    case INDEX_COLR:
      return GEOMETRY_COLR;
    case INDEX_NORM_COLR_BONED:
      return GEOMETRY_NORM_COLR_BONED;
    case INDEX_NORM_CONST_COLR:
      return GEOMETRY_NORM_CONST_COLR;
    case INDEX_NORM_CONST_COLR_BONED:
      return GEOMETRY_NORM_CONST_COLR_BONED;
    case INDEX_NORM_TEXT:
      return GEOMETRY_NORM_TEXT;
    case INDEX_NORM_TEXT_BONED:
      return GEOMETRY_NORM_TEXT_BONED;
    case INDEX_NORM_TEXT_DISP:
      return GEOMETRY_NORM_TEXT_DISP;
    default:
      throw std::wruntime_error("GeometryRenderPass::getGeometryType() - "
        "ERROR: index not recognized or not supported.");
    }
  }

  // TODO: Get rid of this function!  Maybe use a hash map
  GeometryRenderPass::VertexPrimativeIndex 
    GeometryRenderPass::getVertexPrimativeIndex(const VertexPrimative type) const {
    switch (type) {
    case VERT_POINTS:
      return INDEX_VERT_POINTS;
    case VERT_LINES:
      return INDEX_VERT_LINES;
    case VERT_TRIANGLES:
      return INDEX_VERT_TRIANGLES;
    case VERT_QUADS:
      return INDEX_VERT_QUADS;
    default:
      throw std::wruntime_error("GeometryRenderPass::getVertexPrimativeIndex()"
        " - ERROR: index not recognized or not supported.");
    }
  }

  // TODO: Get rid of this function!  Maybe use a hash map
  VertexPrimative GeometryRenderPass::getVertexPrimative(
    const GeometryRenderPass::VertexPrimativeIndex index) const {
    switch (index) {
    case INDEX_VERT_POINTS:
      return VERT_POINTS;
    case INDEX_VERT_LINES:
      return VERT_LINES;
    case INDEX_VERT_TRIANGLES:
      return VERT_TRIANGLES;
    case INDEX_VERT_QUADS:
      return VERT_QUADS;
    default:
      throw std::wruntime_error("GeometryRenderPass::getVertexPrimative() - "
        "ERROR: index not recognized or not supported.");
    }
  }

  char* copyStrSafe(const char* str) {
    char* ret_val = NULL;
    if (str) {
      ret_val = new char[strlen(str)+1];
      strncpy(ret_val, str, strlen(str)+1);
    }
    return ret_val;
  }

  void GeometryRenderPass::setShader(const VertexPrimative primative, 
    const GeometryType geom_type, const char* vshader, const char* fshader, 
    const char* gshader, const char* tcsshader, const char* tesshader) {
    // Make a copy of the input char*
    char* vshader_cpy = copyStrSafe(vshader);
    char* fshader_cpy = copyStrSafe(fshader);
    char* gshader_cpy = copyStrSafe(gshader);
    char* tcsshader_cpy = copyStrSafe(tcsshader);
    char* tesshader_cpy = copyStrSafe(tesshader);
    GeometryTypeIndex index = getGeometryTypeIndex(geom_type);

#if defined(DEBUG) || defined(_DEBUG)
    if (primative >= NUM_VERTEX_PRIMATIVES_SUPORTED) {
      throw std::wruntime_error("primative >= NUM_VERTEX_PRIMATIVES_SUPORTED");
    }
    if (primative >= NUM_VERTEX_PRIMATIVES_SUPORTED) {
      throw std::wruntime_error("geom_type >= NUM_GEOMETRY_TYPES_SUPORTED");
    }
#endif

    v_shaders_[primative].deleteAt(index);
    f_shaders_[primative].deleteAt(index);
    g_shaders_[primative].deleteAt(index);
    tcs_shaders_[primative].deleteAt(index);
    tes_shaders_[primative].deleteAt(index);

    v_shaders_[primative].set(index, vshader_cpy);
    f_shaders_[primative].set(index, fshader_cpy);
    g_shaders_[primative].set(index, gshader_cpy);
    tcs_shaders_[primative].set(index, tcsshader_cpy);
    tes_shaders_[primative].set(index, tesshader_cpy);
  }

  void GeometryRenderPass::unsetShaders() {
    for (uint32_t p = 0; p < NUM_VERTEX_PRIMATIVES_SUPORTED; p++) {
      for (uint32_t g = 0; g < NUM_GEOMETRY_TYPES_SUPORTED; g++) {
        v_shaders_[p].deleteAt(g);
        f_shaders_[p].deleteAt(g);
        g_shaders_[p].deleteAt(g);
        tcs_shaders_[p].deleteAt(g);
        tes_shaders_[p].deleteAt(g);
      }
    }
  }

  void GeometryRenderPass::render() {
#if defined(DEBUG) || defined(_DEBUG)
    if (view_ == NULL || proj_ == NULL || frustum_ == NULL || 
      screen_size_ == NULL) {
      throw wruntime_error("GeometryRenderPass::render() - ERROR: view_ or"
        " proj_ or frustum_ or screen_size_ not set for this render pass!");
    }
#endif

    bool wireframe=false, motion_blur_hq_boned=false;
    GET_SETTING("render_wireframe", bool, wireframe);
    GET_SETTING("motion_blur_hq_boned", bool, motion_blur_hq_boned);
    if (wireframe) {
      GLState::glsPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
      GLState::glsPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // ********************************************
    // Render all the types, but batch render them (so all geometry of
    // a primative and type are rendered together)...  This avoids context
    // switching the shader.
    for (uint32_t p = 0; p < NUM_VERTEX_PRIMATIVES_SUPORTED; p++) {
      for (uint32_t g = 0; g < NUM_GEOMETRY_TYPES_SUPORTED; g++) {
        GeometryType cur_type = getGeometryType((GeometryTypeIndex)g);
        VertexPrimative cur_prim = getVertexPrimative((VertexPrimativeIndex)p);

        if (!isShaderSet((VertexPrimativeIndex)p, (GeometryTypeIndex)g)) {
          // No shader for this geometry primative and type has been set
          continue;
        }

        ShaderProgram::useShaderProgram(v_shaders_[p][g], f_shaders_[p][g], 
          g_shaders_[p][g], tcs_shaders_[p][g], tes_shaders_[p][g]);

        if (shader_uniform_cb_) {
          shader_uniform_cb_();
        }

        if (QUERY_UNIFORM("tc_tess_factor")) {
          int tess_factor;
          GET_SETTING("tess_factor", int, tess_factor);
          float ftess_factor = (float)tess_factor;
          BIND_UNIFORM("tc_tess_factor", &ftess_factor);
        }

        renderer_->geometry_manager()->renderStackReset();
        while (!renderer_->geometry_manager()->renderStackEmpty()) {
          GeometryInstance* cur_geom = renderer_->geometry_manager()->renderStackPop();
          if (cur_geom->render() && 
              cur_geom->type() == cur_type && 
              cur_prim == cur_geom->geom()->primative_type()) {
            // HACK! AABBOX for boned meshes is broken, so pretend it always 
            // passes frustum culling.
            bool boned_mesh = false;
            if (cur_geom->geom() && cur_geom->geom()->bone_names().size() > 0) {
              boned_mesh = true;
            }
            if (boned_mesh || (cur_geom->aabbox() && 
              cur_geom->aabbox()->frustumCullTest(frustum_))) {

              // Calculate model view matrix and bind it to the shader
              if (QUERY_UNIFORM("vw_mat")) {
                Float4x4::multSIMD(vw_mat_, *view_, cur_geom->mat_hierarchy());
                BIND_UNIFORM("vw_mat", vw_mat_.m);
              }

              // Calculate model view normal matrix and bind it to the shader
              // Normal matrix is the (M_modelview^-1)^T:
              // --> http://www.songho.ca/opengl/gl_transform.html
              if (QUERY_UNIFORM("normal_mat")) {
                Float4x4::affineInverse(normal_mat_, vw_mat_);
                normal_mat_.transpose();
                BIND_UNIFORM("normal_mat", normal_mat_.m);
              }

              if (QUERY_UNIFORM("p_mat")) {
                // Plane projection matrix used in tessellation shader
                BIND_UNIFORM("p_mat", proj_->m);
              }

              if (QUERY_UNIFORM("pvw_mat")) {
                // Calculate model view projection matrix and bind it
                Float4x4::multSIMD(pvw_mat_, *proj_, vw_mat_ );
                BIND_UNIFORM("pvw_mat", pvw_mat_.m);
              }

              if (QUERY_UNIFORM("pvw_mat_prev_frame")) {
                if (wireframe) {
                  // Escentially no motion blur when rendering wireframe!
                  BIND_UNIFORM("pvw_mat_prev_frame", pvw_mat_.m);
                } else {
                  BIND_UNIFORM("pvw_mat_prev_frame", 
                    cur_geom->pvm_prev_frame().m);
                }
                cur_geom->pvm_prev_frame().set(pvw_mat_);
              }

              if (QUERY_UNIFORM("v_point_size")) {
                GLState::glsEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
                BIND_UNIFORM("v_point_size", &cur_geom->point_line_size());
              } else {
                GLState::glsDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
              }

              if (cur_geom->geom()->primative_type() == VERT_LINES) {
                GLState::glsLineWidth(cur_geom->point_line_size());
              }

              if (QUERY_UNIFORM("f_lighting_stencil")) {
                const float stencil_value = 
                  cur_geom->apply_lighting() ? 1.0f : 0.0f;
                BIND_UNIFORM("f_lighting_stencil", &stencil_value);
              }

              if (QUERY_UNIFORM("f_vel_mul")) {
                float motion_blur_intensity;
                GET_SETTING("motion_blur_intensity", float, 
                  motion_blur_intensity);
                BIND_UNIFORM("f_vel_mul", &motion_blur_intensity);
              }

              cur_geom->mtrl().setUniforms();
              cur_geom->bindRGBTexture(GL_TEXTURE0);
              cur_geom->bindBumpTexture(GL_TEXTURE1);
              cur_geom->bindDispTexture(GL_TEXTURE2);
              cur_geom->bindBoneMatrices(motion_blur_hq_boned);

              // Draw the current geometry
              cur_geom->draw();
            }
          }
        }  // while
      }  // for (uint32_t g = 0; g < NUM_GEOMETRY_TYPES_SUPORTED; g++)
    }  // for (uint32_t p = 0; p < NUM_VERTEX_PRIMATIVES_SUPORTED; p++)

    // ********************************************
    // Render all the light objects --> these are flagged as non-lightable in
    // the gbuffer (just like the skybox)
    // TODO: PUT ALL THESE IN THE SCENE GRAPH!
    bool spot_lights = false;
    GET_SETTING("spot_lights", bool, spot_lights);
    bool point_lights = false;
    GET_SETTING("point_lights", bool, point_lights);

    const data_str::VectorManaged<Light*>& lights = 
      renderer_->lighting()->lights();

    if (render_light_volumes_) {
      if (spot_lights || point_lights) {
        const uint32_t g = INDEX_NORM_CONST_COLR;
        const uint32_t p = INDEX_VERT_TRIANGLES;
        if (!isShaderSet((VertexPrimativeIndex)p, (GeometryTypeIndex)g)) {
          // No shader for this geometry primative and type has been set
          throw std::wruntime_error("Trying to render light volumes without "
            "a corresponding shader set.");
        }
        ShaderProgram::useShaderProgram(v_shaders_[p][g], f_shaders_[p][g], 
          g_shaders_[p][g], tcs_shaders_[p][g], tes_shaders_[p][g]);

        if (shader_uniform_cb_) {
          shader_uniform_cb_();
        }

        GLState::glsEnable(GL_DEPTH_TEST);
        GLState::glsDisable(GL_CULL_FACE);
        GLState::glsPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        if (QUERY_UNIFORM("f_lighting_stencil")) {
          const float stencil_value = 0.0f;
          BIND_UNIFORM("f_lighting_stencil", &stencil_value);
        }

        // Render lights as wireframe objects
        // Don't bother frustum culling since this is just a debug mode.
        for (uint32_t i = 0; i < lights.size(); i++) {
          if (lights[i]->on()) {
            switch (lights[i]->type()) {
            case LightType::LIGHT_POINT:
              if (point_lights) {
                renderPointLightObject(reinterpret_cast<LightPoint*>(lights[i]),
                  false);
              }
              break;
            case LightType::LIGHT_SPOT_VSM:
            case LightType::LIGHT_SPOT:
              if (spot_lights) {
                renderSpotLightObject(reinterpret_cast<LightSpot*>(lights[i]),
                  false);
              }
              break;
            default:
              break;
            }
          }
        }
      }
    }

    if (render_light_sources_) {
      if (spot_lights || point_lights) {
        const uint32_t g = INDEX_NORM_CONST_COLR;
        const uint32_t p = INDEX_VERT_TRIANGLES;
        if (!isShaderSet((VertexPrimativeIndex)p, (GeometryTypeIndex)g)) {
          // No shader for this geometry primative and type has been set
          throw std::wruntime_error("Trying to render light sources without "
            "a corresponding shader set.");
        }
        ShaderProgram::useShaderProgram(v_shaders_[p][g], f_shaders_[p][g], 
          g_shaders_[p][g], tcs_shaders_[p][g], tes_shaders_[p][g]);

        if (shader_uniform_cb_) {
          shader_uniform_cb_();
        }

        GLState::glsEnable(GL_DEPTH_TEST);
        GLState::glsDisable(GL_CULL_FACE);
        if (wireframe) {
          GLState::glsPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
          GLState::glsPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        if (QUERY_UNIFORM("f_lighting_stencil")) {
          const float stencil_value = 0.0f;
          BIND_UNIFORM("f_lighting_stencil", &stencil_value);
        }

        // Render lights as scaled down solid objects
        LightPoint* light_point;
        LightSpot* light_spot;
        for (uint32_t i = 0; i < lights.size(); i++) {
          if (lights[i]->on()) {
            switch (lights[i]->type()) {
            case LightType::LIGHT_POINT:
              if (point_lights) {
                light_point = reinterpret_cast<LightPoint*>(lights[i]);
                float scale = LIGHT_OBJECT_SIZE / light_point->outside_rad();
                light_point->scalePVWMat(scale);
                renderPointLightObject(light_point, true);
                light_point->scalePVWMat(1.0f / scale);
              }
              break;
            case LightType::LIGHT_SPOT_VSM:
            case LightType::LIGHT_SPOT:
              if (spot_lights) {
                light_spot = reinterpret_cast<LightSpot*>(lights[i]);
                float scale = 1.5f * LIGHT_OBJECT_SIZE / 
                  light_spot->near_far()[1];
                light_spot->scalePVWMat(scale);
                renderSpotLightObject(light_spot, true);
                light_spot->scalePVWMat(1.0f / scale);
              }
              break;
            default:
              break;
            }
          }
        }
      }
    }

    // ********************************************
    // Render all the AABBoxes
    // TODO: PUT ALL THESE IN THE SCENE GRAPH!
    if (render_aabboxes_) {
      const uint32_t g = INDEX_NORM_CONST_COLR;
      const uint32_t p = INDEX_VERT_TRIANGLES;
      if (!isShaderSet((VertexPrimativeIndex)p, (GeometryTypeIndex)g)) {
        // No shader for this geometry primative and type has been set
        throw std::wruntime_error("Trying to render AABBOXes without "
          "a corresponding shader set.");
      }
      ShaderProgram::useShaderProgram(v_shaders_[p][g], f_shaders_[p][g], 
        g_shaders_[p][g], tcs_shaders_[p][g], tes_shaders_[p][g]);

      if (shader_uniform_cb_) {
        shader_uniform_cb_();
      }

      GLState::glsEnable(GL_DEPTH_TEST);
      GLState::glsDisable(GL_CULL_FACE);
      GLState::glsPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

      if (QUERY_UNIFORM("f_lighting_stencil")) {
        const float stencil_value = 0.0f;
        BIND_UNIFORM("f_lighting_stencil", &stencil_value);
      }

      Geometry* cube_geometry = 
        renderer_->geometry_manager()->findGeometryByName(AABBOX_CUBE_NAME);

      // Render AABBoxes as wireframe objects.  Don't bother frustum culling
      // since they're just debug objects.
      renderer_->geometry_manager()->GeometryManager::renderStackReset();
      while (!renderer_->geometry_manager()->GeometryManager::renderStackEmpty()) {
        GeometryInstance* cur_geom = 
          renderer_->geometry_manager()->GeometryManager::renderStackPop();
        if (cur_geom->render() && 
          cur_geom->type() != GeometryType::GEOMETRY_BASE) {
          if (cur_geom->aabbox()) {
            renderAABBox(cur_geom->aabbox(), cube_geometry);
          }
        }
      }  // while
      for (uint32_t i = 0; i < lights.size(); i++) {
        if (lights[i]->on()) {
          switch (lights[i]->type()) {
          case LightType::LIGHT_POINT:
            if (point_lights) {
              renderAABBox(lights[i]->aabbox(), cube_geometry);
            }
            break;
          case LightType::LIGHT_SPOT_VSM:
          case LightType::LIGHT_SPOT:
            if (spot_lights) {
              renderAABBox(lights[i]->aabbox(), cube_geometry);
            }
            break;
          default:
            break;
          }
        }
      }
    }
  }

  void GeometryRenderPass::renderAABBox(const AABBox* box, 
    const Geometry* cube) {
    Float4x4::scaleMat(world_, box->half_lengths());
    world_[0] += 0.01f;
    world_[5] += 0.01f;
    world_[10] += 0.01f;
    world_.leftMultTranslation(box->center());

    Float4x4::multSIMD(vw_mat_, *view_, world_);
    BIND_UNIFORM("vw_mat", vw_mat_.m);

    if (QUERY_UNIFORM("normal_mat")) {
      Float4x4::affineInverse(normal_mat_, vw_mat_);
      normal_mat_.transpose();
      BIND_UNIFORM("normal_mat", normal_mat_.m);
    }

    // Calculate model view projection matrix and bind it to the shader
    Float4x4::multSIMD(pvw_mat_, *proj_, vw_mat_ );
    BIND_UNIFORM("pvw_mat", pvw_mat_.m);
    
    // No motion blur on AABBox --> But this is OK since it's just a debug mode
    if (QUERY_UNIFORM("pvw_mat_prev_frame")) {
      BIND_UNIFORM("pvw_mat_prev_frame", pvw_mat_.m);
    }

    const math::Float3 aabbox_color(1.0f, 1.0f, 1.0f);
    const float spec_power = 0;
    const float spec_intensity = 0;
    if (QUERY_UNIFORM("f_const_albedo")) {
      BIND_UNIFORM("f_const_albedo", aabbox_color.m);
    }
    if (QUERY_UNIFORM("f_spec_power")) {
      BIND_UNIFORM("f_spec_power", &spec_power);
    }
    if (QUERY_UNIFORM("f_spec_intensity")) {
      BIND_UNIFORM("f_spec_intensity", &spec_intensity);
    }

    cube->draw();
  }

  void GeometryRenderPass::renderSpotLightObject(const LightSpot* light, 
    const bool motion_blur) const {
    BIND_UNIFORM("pvw_mat", light->pvw_mat().m);
    BIND_UNIFORM("vw_mat", light->vw_mat().m);

    if (QUERY_UNIFORM("pvw_mat_prev_frame")) {
      if (motion_blur) {
        BIND_UNIFORM("pvw_mat_prev_frame", light->pvm_prev_frame().m);
      } else {
        BIND_UNIFORM("pvw_mat_prev_frame", light->pvw_mat().m);
      }
    }

    BIND_UNIFORM("normal_mat", light->normal_mat().m);
    BIND_UNIFORM("f_const_albedo", light->diffuse_color().m);
    const float spec_power = 0;
    const float spec_intensity = 0;
    BIND_UNIFORM("f_spec_power", &spec_power);
    BIND_UNIFORM("f_spec_intensity", &spec_intensity);

    renderer_->lighting()->light_geom_spot()->draw();
  }

  void GeometryRenderPass::renderPointLightObject(const LightPoint* light, 
    const bool motion_blur) const {
    BIND_UNIFORM("pvw_mat", light->pvw_mat().m);
    BIND_UNIFORM("vw_mat", light->vw_mat().m);

    if (QUERY_UNIFORM("pvw_mat_prev_frame")) {
      if (motion_blur) {
        BIND_UNIFORM("pvw_mat_prev_frame", light->pvm_prev_frame().m);
      } else {
        BIND_UNIFORM("pvw_mat_prev_frame", light->pvw_mat().m);
      }
    }

    BIND_UNIFORM("normal_mat", light->normal_mat().m);
    BIND_UNIFORM("f_const_albedo", light->diffuse_color().m);
    const float spec_power = 0;
    const float spec_intensity = 0;
    BIND_UNIFORM("f_spec_power", &spec_power);
    BIND_UNIFORM("f_spec_intensity", &spec_intensity);

    renderer_->lighting()->light_geom_point()->draw();
  }

  const bool GeometryRenderPass::isShaderSet(const VertexPrimativeIndex p, 
    const GeometryTypeIndex g) const {
    if (v_shaders_[p][g] == NULL && f_shaders_[p][g] == NULL && 
      g_shaders_[p][g] == NULL && tcs_shaders_[p][g] == NULL && 
      tes_shaders_[p][g] == NULL) {
      return false;
    }
    return true;
  }

}  // namespace renderer
}  // namespace jtil

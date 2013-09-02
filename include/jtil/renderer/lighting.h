//
//  lighting.h
//
//  Created by Jonathan Tompson on 6/15/12.
//

#pragma once

#include "jtil/renderer/gl_include.h"
#include "jtil/data_str/vector.h"
#include "jtil/data_str/vector_managed.h"

#define LIGHTING_CVSM_MAX_COUNT 4
#define LIGHT_OBJECT_SIZE 0.1f

namespace jtil {
namespace renderer {

  class Renderer;
  class Shader;
  class ShaderProgram;
  class TextureBase;
  class TextureRenderable;
  class TextureRenderableArray;
  class Texture;
  class Geometry;
  class GeometryRenderPass;
  class Light;
  class LightPoint;
  class LightDir;
  class LightSpot;
  class LightSpotCVSM;
  class Frustum;

  class Lighting {
  public:
    Lighting(Renderer* renderer);
    ~Lighting();

    void renderLighting();
    void updateLights();  // Call after camera view matrix is correct
    void updateLightsPVW();  // Call after camera view + proj matrix is correct
    void addLight(Light* light);
    void drawVSM() const;

    inline TextureRenderable* ambient() const { return ambient_; }

    inline const data_str::VectorManaged<Light*>& lights() const { 
      return lights_; }
    inline Geometry* light_geom_point() const { return light_geom_point_; }
    inline Geometry* light_geom_spot() const { return light_geom_spot_; }

  private:
    Renderer* renderer_;
    
    // Geometry for the various light volumes (sphere and cone) and matricies
    Geometry* light_geom_point_;
    Geometry* light_geom_spot_;
    TextureRenderable* ambient_;
    TextureRenderable* ambient_blur_temp_;
    bool ambient_cleared_;
    data_str::VectorManaged<Light*> lights_;
    Texture* vector_noise_tex_;  // NOT OWNED HERE
    TextureBase* shadow_map_;
    TextureBase* shadow_map_blur_temp_;
    float vsm_split_depths_[LIGHTING_CVSM_MAX_COUNT + 1];
    math::Float2 vsm_split_scales_[LIGHTING_CVSM_MAX_COUNT + 1];
    math::Float4x4 vsm_split_pv_camviewinv_[LIGHTING_CVSM_MAX_COUNT];
    math::Float4x4 vsm_split_pv_[LIGHTING_CVSM_MAX_COUNT];
    float vsm_split_pv_camviewinv_float_[LIGHTING_CVSM_MAX_COUNT * 16];
    math::Float4x4 vsm_split_proj_[LIGHTING_CVSM_MAX_COUNT];
    float vsm_max_lod_;
    float vsm_min_filter_width_;
    GeometryRenderPass* vsm_render_pass_;
    TextureRenderable* vsm_splits_;
    Frustum* vsm_frustum_;

    // Ptrs to static members --> Used when rendering VSM
    static const LightSpotCVSM* cur_light_spot_vsm_;
    static const Lighting* cur_lighting_;

    void renderLightAccumulationPass();
    void renderLightFinalPass();
    void renderAmbientOcclusionPass();

    void renderCVSMSplitTexture(const uint32_t vsm_count) const;
    void visualizeCVSMSplit() const;
    
    void lightPointStencilPass(const LightPoint* light) const;
    void lightPointAccumPass(const LightPoint* light, 
      const bool stencil_optim) const;
    void lightSpotStencilPass(const LightSpot* light) const;
    void lightSpotAccumPass(const LightSpot* light, 
      const bool stencil_optim) const;
    void LightSpotCVSMAccumPass(const LightSpotCVSM* light, 
      const bool stencil_optim) const;
    void LightSpotVSMAccumPass(const LightSpotCVSM* light, 
      const bool stencil_optim) const;
    void lightDirAccumPass(const LightDir* light) const;
    // Generic function for point and spot light sources
    void LightGeometryStencilPass(const Geometry* geom, 
      const math::Float4x4& pvw_mat) const;
    void lightGeomAccumPass(const Geometry* geom, 
      const math::Float4x4& pvw_mat, const bool stencil_optim,
      const bool camera_inside) const;

    void renderSpotCVSM(const LightSpotCVSM* light);
    void renderSpotVSM(const LightSpotCVSM* light);

    static void renderSpotCVSMUniformCB();
    static void renderSpotVSMUniformCB();

    void blurShadowMap(const uint32_t csm_count) const;
    void blurAmbientOcclusion() const;  // Note: Blur radius is hard coded

    void calcCVSMSplitDepths(const uint32_t csm_count);
    void calcCVSMSplitMats(const LightSpotCVSM* light);

    // Non-copyable, non-assignable.
    Lighting(Lighting&);
    Lighting& operator=(const Lighting&);
  };
};  // namespace renderer
};  // namespace jtil

#include <iostream>
#include <thread>
#include "jtil/renderer/gl_include.h"
#include "jtil/renderer/renderer.h"
#include "jtil/renderer/shader/shader.h"
#include "jtil/renderer/shader/shader_program.h"
#include "jtil/settings/settings_manager.h"
#include "jtil/ui/ui.h"
#include "jtil/renderer/camera/camera.h"
#include "jtil/exceptions/wruntime_error.h"
#include "jtil/renderer/gl_state.h"
#include "jtil/renderer/geometry/geometry.h"
#include "jtil/renderer/geometry/bone.h"
#include "jtil/renderer/geometry/geometry_instance.h"
#include "jtil/renderer/geometry/geometry_manager.h"
#include "jtil/renderer/texture/texture.h"
#include "jtil/renderer/texture/texture_gbuffer.h"
#include "jtil/renderer/g_buffer.h"
#include "jtil/renderer/lighting.h"
#include "jtil/renderer/post_processing.h"
#include "jtil/renderer/lights/light_spot.h"
#include "jtil/renderer/lights/light_spot_cvsm.h"
#include "jtil/renderer/lights/light_point.h"
#include "jtil/renderer/lights/light_dir.h"
#include "jtil/renderer/colors/colors.h"
#include "jtil/renderer/objects/aabbox.h"
#include "jtil/math/math_types.h"
#include "jtil/math/perlin_noise.h"
#include "jtil/clk/clk.h"
#include "jtil/threading/thread.h"
#include "jtil/threading/callback.h"

#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }
#define SM_SETTINGS_FILE "settings.csv"
#define FLASHLIGHT_MODEL_PATH "resource_files/flashlight/"
#define FLASHLIGHT_MODEL_JBIN "flashlight2.jbin"
#define FLASHLIGHT_MODEL_OBJ "flashlight2.obj"
#define LOAD_JBIN_FLASHLIGHT

//#if defined(DEBUG) || defined(_DEBUG)
//  #define VERIFY_OPEN_GL_STATE_PER_FRAME  // VERY EXPENSIVE
//#endif

using std::wstring;
using std::wruntime_error;

namespace jtil {

using math::Float2;
using math::Float3;
using math::Float4;
using math::FloatQuat;
using math::Float4x4;
using renderer::Geometry;
using renderer::objects::AABBox;
using windowing::Window;
using windowing::WindowSettings;
using windowing::KeyboardCBFuncPtr;
using windowing::MousePosCBFuncPtr;
using windowing::MouseButCBFuncPtr;
using windowing::MouseWheelCBFuncPtr;
using windowing::CloseWndCBFuncPtr;
using clk::Clk;
using settings::SettingsManager;
using threading::Callback;
using threading::MakeCallableOnce;
using threading::MakeThread;

namespace renderer {
  Renderer* Renderer::g_renderer_ = NULL;

  void Renderer::InitRenderer() {
    if (g_renderer_) {
      throw wruntime_error("Renderer::InitRenderer() - ERROR: Renderer has "
        "already been initialized!  Only one Renderer instance is allowed.");
    }

    // Init the SettingsManager first!
    SettingsManager::initSettings(SM_SETTINGS_FILE);
    // Initialize the Texture Library --> Done once on startup
    Texture::initTextureSystem();

    createNewRenderer();
  }

  void Renderer::createNewRenderer() {
    if (g_renderer_) {
      throw wruntime_error("Renderer::InitRenderer() - ERROR: Renderer has "
        "already been initialized!  Only one Renderer instance is allowed.");
    }
    g_renderer_ = new renderer::Renderer();
    g_renderer_->init();
  }

  void Renderer::ShutdownRenderer() {
    SAFE_DELETE(g_renderer_);
    SettingsManager::saveSettings(SM_SETTINGS_FILE);
    SettingsManager::shutdownSettingsManager();
    Texture::shutdownTextureSystem();
    Window::killWindowSystem();  // Must come last!
  }

  Renderer::Renderer() {
    camera_ = NULL;
    gm_ = NULL;
    wnd_ = NULL;
    g_buffer_ = NULL;
    lighting_ = NULL;
    post_processing_ = NULL;
    wnd_ = NULL;
    ui_ = NULL;
    clk_ = NULL;
    keyboard_cb_ = NULL;
    mouse_pos_cb_ = NULL;
    mouse_button_cb_ = NULL;
    mouse_wheel_cb_ = NULL;
    reset_screen_cb_ = NULL;
    close_cb_ = NULL;
    background_tex_ = NULL;

    frame_counter_ = 0;
    reload_renderer_ = false;
    stretch_background_tex_ = true;
  }

  Renderer::~Renderer() {
    shutdown();
  }

  void Renderer::shutdown() {
    // Called just before the render thread is shutdown
    SAFE_DELETE(clk_);
    SAFE_DELETE(ui_);
    SAFE_DELETE(camera_);
    SAFE_DELETE(g_buffer_);
    SAFE_DELETE(lighting_);
    SAFE_DELETE(post_processing_);
    SAFE_DELETE(gm_);
    ShaderProgram::releaseShaders();
    SAFE_DELETE(wnd_);  // Delete window last! (destroys OpenGL Context)
  }

  void Renderer::createWindow() {
    if (wnd_) {
      throw wruntime_error(L"App::createWindow() - ERROR: A window exists!");
    }

    // Get the window settings from the UI manager
    int screen_resolution, width, height;
    GET_SETTING("screen_resolution", int, screen_resolution);
    Window::windowResEnumToInt(screen_resolution, width, height);
    bool fullscreen = false;
    GET_SETTING("fullscreen", bool, fullscreen);
    bool double_buffering = true;
    GET_SETTING("double_buffering", bool, double_buffering);
    int num_depth_bits = 32;
    GET_SETTING("window_depth_buffer_bits", int, num_depth_bits);
    int num_stencil_bits = 8;
    GET_SETTING("window_stencil_buffer_bits", int, num_stencil_bits);
    int num_rgb_bits = 24;
    GET_SETTING("window_rgba_bits", int, num_rgb_bits);
    int gl_major_version = 3;
    GET_SETTING("open_gl_major_version", int, gl_major_version);
    int gl_minor_version = 2;
    GET_SETTING("open_gl_minor_version", int, gl_minor_version);
    bool gl_core_profile = true;
    GET_SETTING("open_gl_core_profile", bool, gl_core_profile);
    
    WindowSettings settings;
    settings.fullscreen = fullscreen;
    settings.double_buffering = double_buffering;
    settings.height = height;
    settings.width = width;
    settings.gl_major_version = gl_major_version;
    settings.gl_minor_version = gl_minor_version;
    settings.gl_core_profile = gl_core_profile;
    settings.num_depth_bits = num_depth_bits;
    settings.num_rgba_bits = num_rgb_bits;
    settings.num_stencil_bits = num_stencil_bits;
    settings.samples = 1;
    wnd_ = new Window(settings);
    
    wnd_->registerKeyboardCB(Renderer::keyboardCB);
    wnd_->registerMousePosCB(Renderer::mousePosCB);
    wnd_->registerMouseButCB(Renderer::mouseButtonCB);
    wnd_->registerMouseWheelCB(Renderer::mouseWheelCB);
    wnd_->registerCloseWndCB(windowClose);
  }

  void Renderer::reloadRenderer() {
    // To toggle to fullscreen we have to recreate the window which will
    // destroy the existing OpenGL context.  This means we need to reload
    // all the OpenGL rendering assets --> Solution, just completely
    // reload the renderer and the ui.  This is slow, but wont happen often.
    ResetScreenCBFuncPtr cb = g_renderer_->reset_screen_cb_;
    SAFE_DELETE(g_renderer_);
    createNewRenderer();
    if (cb) {
      cb();
    }
  }

  int Renderer::windowClose() {
    if (g_renderer_->close_cb_) {
      return g_renderer_->close_cb_();
    } else {
      return GL_FALSE;
    }
  }

  void Renderer::renderFrame() {    
    t0_ = t1_;
    t1_ = clk_->getTime();
    float dt = (float)(t1_-t0_);

#if defined(VERIFY_OPEN_GL_STATE_PER_FRAME)
    GLState::verifyOpenGLState();
#endif

    ui_->update(dt);

    // Update camera parameters
    updateCameraFOVScreenSize();  // SettingsManager may have changed them
    camera_->updateView();
    updateFlashlight(dt);  // Before updateLights but after updateView!

    // Update all heirachical matrices (for renderable geometry)
    updateMatrices();  
    updateBoundingVolumes();

    lighting_->updateLights();  // Lights need valid camera view at this point

    fitCameraNearFarToObjects();  // must update camera view matrix first!
    camera_->updateProjection();  // near/far is tightly fitted to world bounds  
    lighting_->updateLightsPVW();  // camera project must have been updated

    g_buffer_->renderGBuffer();  // Renders to g-buffer texture

    lighting_->renderLighting();  // Renders to light accumulation texture

    post_processing_->renderPostProcessing(dt);

    renderOutputFrameToScreen();

    ui_->renderFrame();  // Draw UI last (on top)

    // Make sure we're single or double buffering
    bool double_buffering;
    GET_SETTING("double_buffering", bool, double_buffering);
    if (wnd_->getDoubleBuffering() != double_buffering) {
      wnd_->setDoubleBuffering(double_buffering);
    }
    wnd_->swapBackBuffer();

    // Reload the renderer only when it's a good time to do so.
    if (reload_renderer_) {
      reload_renderer_ = false;
      reloadRenderer();
      return;  // Renderer has been destroyed so return immediately
    }

    frame_counter_++;
  }

  void Renderer::renderOutputFrameToScreen() {
    // render_output = enum describing what we're rendering to the screen
    int render_output;
    GET_SETTING("render_output_enum", int, render_output);

    // Render final result to screen
    GLState::glsBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLState::glsDrawBuffer(GL_BACK);
    GLState::glsViewport(0, 0, wnd_->viewport_width(), 
      wnd_->viewport_height());  

    switch (render_output) {
    case RenderOutput::FULL_SCENE:
      post_processing_->renderFullscreenQuad(g_buffer_->final_scene());
      break;
    case RenderOutput::GBUFFER:
      g_buffer_->visualizeGBuffer();
      break;
    case RenderOutput::LIGHT_ACCUMULATION_DIFFUSE:
      g_buffer_->visualizeGBufferLightingAccumDiff();
      break;
    case RenderOutput::LIGHT_ACCUMULATION_SPECULAR:
      g_buffer_->visualizeGBufferLightingAccumSpec();
      break;
    case RenderOutput::SHADOW_MAP:
      lighting_->drawVSM();
      break;
    case RenderOutput::AMBIENT_OCCLUSION:
      post_processing_->renderFullscreenQuad(lighting_->ambient());
      break;
    case RenderOutput::LUMINANCE:
      post_processing_->renderFullscreenQuad(post_processing_->luma(0));
      break;
    case RenderOutput::VELOCITY:
      g_buffer_->visualizeGBufferVel();
      break;
    case RenderOutput::ALBEDO:
      g_buffer_->visualizeGBufferAlbedo();
      break;
    case RenderOutput::NORMAL:
      g_buffer_->visualizeGBufferNormal();
      break;
    case RenderOutput::DEPTH:
      g_buffer_->visualizeGBufferDepth();
      break;
    case RenderOutput::VIEW_POS:
      g_buffer_->visualizeGBufferViewPos();
      break;
    case RenderOutput::LIGHTING_STENCIL:
      g_buffer_->visualizeGBufferLightingStencil();
      break;
    default:
      throw wruntime_error(L"Renderer::renderFrame() - ERROR: "
        L"unrecognized render_output enum value."); 
    }
  }

  void Renderer::init() {
    // Open a new window.
    Window::initWindowSystem();
    createWindow();

    FloatQuat eye_rot;
    GET_SETTING("starting_eye_rot", FloatQuat, eye_rot);
    Float3 eye_pos;
    GET_SETTING("starting_eye_pos", Float3, eye_pos);
    float znear, zfar, fov_deg;
    GET_SETTING("view_plane_near", float, znear);
    GET_SETTING("view_plane_far", float, zfar);
    GET_SETTING("fov_deg", float, fov_deg);
    camera_ = new Camera(eye_rot, eye_pos, wnd_->width(), wnd_->height(),
      fov_deg, znear, zfar);

    clk_ = new Clk();
    t1_ = clk_->getTime();

    gm_ = new GeometryManager(this);

    // WARNING: ALWAYS INITIALIZE SUB-RENDERERS FIRST!
    //          ALSO, ORDER IS IMPORTANT!
    post_processing_ = new PostProcessing(this);
    g_buffer_ = new GBuffer(this);
    lighting_ = new Lighting(this);

    // Force a resize event TO set the correct viewport and proj matricies
    resize(wnd_->width(), wnd_->height());

    flashlight_ = new LightSpotCVSM(this);
    float flashlight_softness;
    GET_SETTING("flashlight_softness", float, flashlight_softness);
    flashlight_->softness() = flashlight_softness;
    addLight(flashlight_);

#ifndef LOAD_JBIN_FLASHLIGHT
    // Load the obj version
    flashlight_model_ = gm_->loadModelFromFile(FLASHLIGHT_MODEL_PATH, 
      FLASHLIGHT_MODEL_OBJ, false, true, false);

    // Save the jbin version
    gm_->saveModelToJBinFile(FLASHLIGHT_MODEL_PATH, 
      FLASHLIGHT_MODEL_JBIN, flashlight_model_);

#else
    // Load the jbin version
    flashlight_model_ = gm_->loadModelFromJBinFile(FLASHLIGHT_MODEL_PATH, 
      FLASHLIGHT_MODEL_JBIN);
    gm_->scene_root()->addChild(flashlight_model_);
#endif

    ui_ = new ui::UI();
    ui_->init();
  }

  const double Renderer::getTime() {
    return g_renderer_->clk_->getTime();
  }

  GeometryInstance* Renderer::scene_root() { 
    return g_renderer_->gm_->scene_root(); 
  }

  void Renderer::addLight(Light* light) {
    lighting_->addLight(light);
  }

  void Renderer::setBackgroundTexture(Texture* tex) {
    background_tex_ = tex;
  }

  void Renderer::setBackgroundTextureStrech(const bool stretch) {
    stretch_background_tex_ = stretch;
  }

  const data_str::VectorManaged<Light*>& Renderer::lights() const {
    return lighting_->lights();
  }

  void Renderer::resize(const uint32_t width, const uint32_t height) {
    updateCameraFOVScreenSize();
    camera_->updateProjection();
    camera_->updateView();  // Not necessary, but good measure anyway
    GLState::glsViewport(0, 0, width, height);
    ERROR_CHECK;
  }

  void Renderer::updateMatrices() {
    Float4x4 tmp;
    gm_->renderStackReset();
    while (!gm_->renderStackEmpty()) {
      GeometryInstance* cur_geom = gm_->renderStackPop();
      // Update the render matrix based on our parents position
      if (cur_geom->parent() != NULL) {
        Float4x4::multSIMD(cur_geom->mat_hierarchy(), 
          cur_geom->parent()->mat_hierarchy(), cur_geom->mat());
      } else {
        cur_geom->mat_hierarchy().set(cur_geom->mat());
      }

      if (cur_geom->mat_hierarchy_inv()) {
        // A non-null indicates that we'll need to calculate it.
        Float4x4::inverse(*cur_geom->mat_hierarchy_inv(),
          cur_geom->mat_hierarchy());
      }

      if (cur_geom->bone() != NULL) {  
        // This node is a bone, calculate the local bone space matrix
        Float4x4* cur_bone_final_trans = cur_geom->bone_transform();
        Float4x4* bone_offset = &cur_geom->bone()->bone_offset;
        
        Float4x4::multSIMD(tmp, cur_geom->mat_hierarchy(), *bone_offset);
        Float4x4::multSIMD(*cur_bone_final_trans, 
          *cur_geom->bone_root_node()->mat_hierarchy_inv(), tmp);
      }
    }
  }

  void Renderer::updateBoundingVolumes() {
    gm_->renderStackReset();
    while (!gm_->renderStackEmpty()) {
      GeometryInstance* cur_geom = gm_->renderStackPop();
      // Only update the bounding volumes for objects that we care about (ie, 
      // don't bother updating the root class (or billboard classes)
      if (cur_geom->aabbox()) {
        cur_geom->aabbox()->update(cur_geom->mat_hierarchy());
      }
    }
  }
  
  void Renderer::fitCameraNearFarToObjects() {
    // Note, the fitting is NOT tight.   Firstly, we're using the geometry's
    // axis aligned bounding box, which is not tight (after the model
    // transform) then we're assuming the size of the box is maximum after
    // rotation into the camera space (which is also not tight).
    
    float near_min, far_max;
    GET_SETTING("view_plane_near", float, near_min);
    GET_SETTING("view_plane_far", float, far_max);

    fitViewNearFarToObjects(camera_->near_far(), camera_->view(), near_min,
      far_max, true);
  }

  void Renderer::fitViewNearFarToObjects(Float2& near_far, 
    const Float4x4& view, const float near_min, const float far_max,
    const bool inc_light_vols) const {
    near_far.set(near_min, far_max);
    return;

    // Note, the fitting is NOT tight.   Firstly, we're using the geometry's
    // axis aligned bounding box, which is not tight (after the model
    // transform) then we're assuming the size of the box is maximum after
    // rotation into the camera space (which is also not tight).
    
    // Recall: OpenGL convention is to look down the negative Z axis,
    //         therefore, more negative values are actually further away.
    near_far[0] = -std::numeric_limits<float>::infinity();  // znear
    near_far[1] = std::numeric_limits<float>::infinity();  // zfar
    
    float min_z, max_z;

    gm_->renderStackReset();
    while (!gm_->renderStackEmpty()) {
      GeometryInstance* cur_geometry = gm_->renderStackPop();
      AABBox* aabbox = cur_geometry->aabbox();
      if (aabbox) {
        aabbox->calcMinMaxZBoundInViewSpace(min_z, max_z, view);
        if (max_z < near_far[1]) {
          near_far[1] = max_z;
        }
        if (min_z > near_far[0]) {
          near_far[0] = min_z;
        }
      }
    }
    
    // Also fit to light geometry:
    if (inc_light_vols) {
      Float3 pos_source;
      Float3 dir;
      float rad;
      Float3 center_view;
      LightPoint* light_point;
      LightSpot* light_spot;
      for (uint32_t i = 0; i < lighting_->lights().size(); i++) {
        switch (lighting_->lights()[i]->type()) {
        case LIGHT_POINT:
          light_point = (LightPoint*)(lighting_->lights()[i]);
          // Calculate model view projection matrix
          Float3::affineTransformPos(center_view, view, 
            light_point->pos_world());
          rad = light_point->outside_rad();
          if ((center_view[2] - rad) < near_far[1]) {
            near_far[1] = center_view[2] - rad;
          }
          if ((center_view[2] + rad) > near_far[0]) {
            near_far[0] = center_view[2] + rad;
          }
          break;
        case LIGHT_SPOT_VSM:
        case LIGHT_SPOT:
          light_spot = (LightSpot*)(lighting_->lights()[i]);
          // Check the source point
          Float3::affineTransformPos(pos_source, view, 
            light_spot->pos_world());
          if (pos_source[2] < near_far[1]) {
            near_far[1] = pos_source[2];
          }
          if (pos_source[2] > near_far[0]) {
            near_far[0] = pos_source[2];
          }
          // Now check the cone end
          Float3::affineTransformPos(center_view, view, 
            light_spot->cone_center_world());
          rad = light_spot->cone_outside_radius();
          if ((center_view[2] - rad) < near_far[1]) {
            near_far[1] = center_view[2] - rad;
          }
          if ((center_view[2] + rad) > near_far[0]) {
            near_far[0] = center_view[2] + rad;
          }
          break;
        default:
          break;
        }
      }
    }
    
    near_far[0] += LOOSE_EPSILON;
    near_far[1] -= LOOSE_EPSILON;

    // now clamp the near and far to the user defined values
    if (near_far[0] > near_min) {
      near_far[0] = near_min;
    }
    if (near_far[1] < far_max) {
      near_far[1] = far_max;
    }
    if (near_far[1] > near_far[0]) {
      near_far[1] = near_far[0] - 0.1f;
    }
  }
  
  void Renderer::updateCameraFOVScreenSize() {
    camera_->screen_size().set(static_cast<float>(wnd_->width()), 
      static_cast<float>(wnd_->height()));
    float fov_deg;
    GET_SETTING("fov_deg", float, fov_deg);
    camera_->fov_deg() = fov_deg;
  }

  void Renderer::updateFlashlight(float dt) {
    static float time = 0;
    bool on;
    GET_SETTING("flashlight_on", bool, on);
    flashlight_->on() = on;
    flashlight_model_->setRenderHierarchy(on);  // recursive
    if (on) {
      time += dt;
      float dist=10.0f, outer_fov=0.5f, inner_fov=0.4f, 
        diffuse_intensity = 1.0f, scale = 1.0f;
      int csvm_count = 1;
      Float3 pos_view;
      Float3 target_view;
      GET_SETTING("flashlight_dist", float, dist);
      GET_SETTING("flashlight_outer_fov", float, outer_fov);
      GET_SETTING("flashlight_inner_fov", float, inner_fov);
      GET_SETTING("flashlight_csvm_count", int, csvm_count);
      GET_SETTING("flashlight_diffuse_intensity", float, diffuse_intensity);
      GET_SETTING("flashlight_pos_view", Float3, pos_view);
      GET_SETTING("flashlight_target_view", Float3, target_view);
      GET_SETTING("flashlight_scale", float, scale);
      flashlight_->near_far().set(0.1f, dist);
      flashlight_->outer_fov_deg() = outer_fov;
      flashlight_->diffuse_intensity() = diffuse_intensity;
      flashlight_->inner_fov_deg() =inner_fov;
      flashlight_->cvsm_count(csvm_count);

      // The light object itself...
      Float3::affineTransformPos(flashlight_->pos_world(), camera_->view_inv(), 
        pos_view);

      // Add some noise to the world position
      Float3 rand_vec;
      rand_vec[0] = math::PerlinNoise::Noise(0.0f, 0.5f*time) * 0.15f;
      rand_vec[1] = math::PerlinNoise::Noise(100.0f, 0.5f*time) * 0.15f;
      rand_vec[2] = 0;
      Float3::add(flashlight_->pos_world(), rand_vec, 
        flashlight_->pos_world());

      Float3 dir_view;
      Float3::sub(dir_view, target_view, pos_view);
      dir_view.normalize();
      Float3::affineTransformVec(flashlight_->dir_world(), camera_->view_inv(),
        dir_view);
      rand_vec[0] = math::PerlinNoise::Noise(0.0f, 0.5f*time) * 0.15f;
      rand_vec[1] = math::PerlinNoise::Noise(100.0f, 0.5f*time) * 0.15f;
      rand_vec[2] = 0;
      Float3::add(flashlight_->dir_world(), rand_vec, 
        flashlight_->dir_world());

      // Now the flashlight model
      Float4x4& world_mat = flashlight_model_->mat();
      Float3 up(0, 1, 0);
      if (fabsf(fabsf(Float3::dot(flashlight_->dir_world(), up)) - 1) < EPSILON) {
        // preturb the up vector 
        up[0] += (float)LOOSE_EPSILON;
        up[2] += (float)LOOSE_EPSILON;
        up.normalize();
      }
      Float3 side;
      Float3::cross(side, up, flashlight_->dir_world());
      side.normalize();
      Float3::cross(up, flashlight_->dir_world(), side);
      up.normalize();
      Float4x4::rotateMatBasis(world_mat, up, flashlight_->dir_world(), side);
      world_mat.rightMultScale(scale, scale, scale);
      world_mat.leftMultTranslation(flashlight_->pos_world());
    }
  }

  const bool Renderer::getKeyState(const int key) const {
    return wnd_->getKeyState(key);
  }

  const bool Renderer::getMousePosition(math::Double2& pos) const {
    return wnd_->getMousePosition(pos);
  }

  const bool Renderer::getMouseButtonStateRight() const {
    return wnd_->getMouseButtonStateRight();
  }

  const bool Renderer::getMouseButtonStateLeft() const {
    return wnd_->getMouseButtonStateLeft();
  }

  const bool Renderer::getMouseButtonStateMiddle() const {
    return wnd_->getMouseButtonStateMiddle();
  }

  const int Renderer::width() const {
    return wnd_->width();
  }

  const int Renderer::height() const {
    return wnd_->height();
  }

  const int Renderer::viewport_width() const {
    return wnd_->viewport_width();
  }

  const int Renderer::viewport_height() const {
    return wnd_->viewport_height();
  }

  const bool Renderer::fullscreen() const {
     return wnd_->fullscreen();
  }

  const bool Renderer::isOpen() const {
    return wnd_->isOpen();
  }

  const bool Renderer::getDoubleBuffering() const {
    return wnd_->getDoubleBuffering();
  }

  void Renderer::setDoubleBuffering(const bool double_buffer) {
    wnd_->setDoubleBuffering(double_buffer);
  }

  void Renderer::registerKeyboardCB(KeyboardCBFuncPtr callback) {
    keyboard_cb_ = callback;
  }
  
  void Renderer::registerMousePosCB(MousePosCBFuncPtr callback) {
    mouse_pos_cb_ = callback;
  }
  
  void Renderer::registerMouseButCB(MouseButCBFuncPtr callback) {
    mouse_button_cb_ = callback;
  }
  
  void Renderer::registerMouseWheelCB(MouseWheelCBFuncPtr callback) {
    mouse_wheel_cb_ = callback;
  }

  void Renderer::registerCloseWndCB(CloseWndCBFuncPtr callback) {
    close_cb_ = callback;
  }

  void Renderer::registerResetScreenCB(ResetScreenCBFuncPtr callback) {
    reset_screen_cb_ = callback;
  }

  void Renderer::requestReloadRenderer() {
    g_renderer_->reload_renderer_ = true;
  }

  void Renderer::keyboardCB(int key, int scancode, int action, int mods) {
    if (g_renderer_->ui_ != NULL) {
      g_renderer_->ui_->keyboardInputCB(key, scancode, action, mods);
    }

    // If cntr + shift is held down AND we press one of the special keys
    bool renderer_command = g_renderer_->wnd_->getKeyState(KEY_LCTRL) && 
      g_renderer_->wnd_->getKeyState(KEY_LSHIFT) && ((key == KEY_F1 || 
      key == KEY_F2 || key == 'U' || key == 'F') && (action == RELEASED));

    if (renderer_command) {
      switch (key) {
      case KEY_F1:
        {
          bool fullscreen;
          GET_SETTING("fullscreen", bool, fullscreen);
          SET_SETTING("fullscreen", bool, !fullscreen);
          Renderer::requestReloadRenderer();
        }
        break;
      case KEY_F2: 
        g_renderer_->ui_->debugger_visible(
          !g_renderer_->ui_->debugger_visible());
        break;
      case 'U':
        {
          bool render_ui_app;
          GET_SETTING("render_ui_app", bool, render_ui_app);
          SET_SETTING("render_ui_app", bool, !render_ui_app);
          g_renderer_->ui_->setSettingsVisibility(!render_ui_app);
        }
        break;
      case 'F':
        {
          bool flashlight_on;
          GET_SETTING("flashlight_on", bool, flashlight_on);
          SET_SETTING("flashlight_on", bool, !flashlight_on);
          if (g_renderer_->ui_) {
            g_renderer_->ui_->setRendererCheckboxVal("flashlight_on", 
              !flashlight_on);
          }
        }
        break;
      }
    } else {
      if (g_renderer_->keyboard_cb_) {
        g_renderer_->keyboard_cb_(key, scancode, action, mods);
      }
    }
  }
  
  void Renderer::mousePosCB(double x, double y) {
    if (g_renderer_->ui_ != NULL) {
      g_renderer_->ui_->mousePosCB(x, y);
    }
  }
  
  void Renderer::mouseButtonCB(int button, int action, int mods) {
    if (g_renderer_->ui_ != NULL) {
      g_renderer_->ui_->mouseButtonCB(button, action, mods);
    }
  }
  
  void Renderer::mouseWheelCB(double xoffset, double yoffset) {
    g_renderer_->mouse_wheel_pos_old.set(g_renderer_->mouse_wheel_pos_);
    g_renderer_->mouse_wheel_pos_[0] += xoffset;
    g_renderer_->mouse_wheel_pos_[1] += yoffset;
    if (g_renderer_->ui_ != NULL) {
      g_renderer_->ui_->mouseWheelCB(xoffset, yoffset);
    }
  }

}  // namespace renderer
}  // namespace jtil

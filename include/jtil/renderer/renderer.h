//
//  renderer.h
//
//  Created by Jonathan Tompson on 5/29/12.
//

#pragma once

#include <thread>
#include <mutex>
#include "jtil/renderer/gl_include.h"
#include "jtil/math/math_types.h"
#include "jtil/windowing/window_cb.h"  // For CBFuncPtr types
#include "jtil/windowing/window_interface.h"  // For WindowInterface class

#ifndef BUFFER_OFFSET
	#define BUFFER_OFFSET(bytes) ((GLubyte*) NULL + bytes)
#endif

namespace jtil {

namespace clk { class Clk; }
namespace settings { class SettingsManager; }
namespace data_str { template <typename T> class VectorManaged; }
namespace settings { class SettingsManager; }
namespace windowing { class Window; }
namespace ui { class UI; }

namespace renderer {

  typedef enum {
    FULL_SCENE,
    GBUFFER,
	  ALBEDO,
	  NORMAL,
    DEPTH,
    VIEW_POS,
    LIGHT_ACCUMULATION_DIFFUSE,
    LIGHT_ACCUMULATION_SPECULAR,
    LIGHTING_STENCIL,
    SHADOW_MAP,
    AMBIENT_OCCLUSION,
    VELOCITY,
    LUMINANCE,
  } RenderOutput;
  
  class Geometry;
  class GeometryInstance;
  class Camera;
  class Shader;
  class ShaderProgram;
  class GBuffer;
  class Lighting;
  class PostProcessing;
  class GeometryVertices;
  class Light;
  class GeometryManager;
  class LightSpotCVSM;
  class Texture;

  typedef void (*ResetScreenCBFuncPtr) ();

  class Renderer : public windowing::WindowInterface {
  public:
    // Initialization and status change functions
    static void InitRenderer();
    static void ShutdownRenderer();
    static void requestReloadRenderer();  // Reload next time it is safe

    void renderFrame();

    // Window interface methods:
    // The renderer will manage it's own window, so all window requests should
    // be done through the Renderer class (this class).
    virtual const bool getKeyState(const int key) const;
    virtual const bool getMousePosition(math::Double2& pos) const;
    virtual const bool getMouseButtonStateRight() const;
    virtual const bool getMouseButtonStateLeft() const;
    virtual const bool getMouseButtonStateMiddle() const;
    virtual const int width() const;
    virtual const int height() const;
    virtual const int viewport_width() const;
    virtual const int viewport_height() const;
    virtual const bool fullscreen() const;
    virtual const bool isOpen() const;
    virtual const bool getDoubleBuffering() const;
    virtual void setDoubleBuffering(const bool double_buffer);
    virtual void registerKeyboardCB(windowing::KeyboardCBFuncPtr callback);
    virtual void registerMousePosCB(windowing::MousePosCBFuncPtr callback);
    virtual void registerMouseButCB(windowing::MouseButCBFuncPtr callback);
    virtual void registerMouseWheelCB(windowing::MouseWheelCBFuncPtr callback);
    virtual void registerCloseWndCB(windowing::CloseWndCBFuncPtr callback);
    void registerResetScreenCB(ResetScreenCBFuncPtr callback);
    
    // Common getter methods
    inline static Renderer* g_renderer() { return g_renderer_; }
    static inline GeometryManager* geometry_manager() { return g_renderer_->gm_; }
    static inline ui::UI* ui() { return g_renderer_->ui_; }
    static inline Camera* camera() { return g_renderer_->camera_; }
    static const double getTime();

    // Less common getter methods (to internal structures you wouldn't normally
    // need)
    inline GBuffer* g_buffer() { return g_buffer_; }
    inline Lighting* lighting() { return lighting_; }
    inline PostProcessing* post_processing() { return post_processing_; }
    inline const LightSpotCVSM* flashlight() { return flashlight_; }

    // Light management functions
    void addLight(Light* light);
    const data_str::VectorManaged<Light*>& lights() const;

    // Background / Skybox functions
    void setBackgroundTexture(Texture* tex);  // Transfers Ownership
    Texture* background_tex() { return background_tex_; }

    // TO DO: MOVE THIS SOMEWHERE ELSE (or make it private)
    // Used to fit the camera near and far planes tightly (to make best use of
    // limited depth map resolution), and by VSM lights to fit their own view
    // near and far planes.
    void fitViewNearFarToObjects(math::Float2& near_far, 
      const math::Float4x4& view, const float near_min, const float far_max,
      const bool inc_light_vols) const;

  private:
    static Renderer* g_renderer_;  // The singleton renderer object

    windowing::Window* wnd_;
    ui::UI* ui_;
    clk::Clk* clk_;
    GeometryManager* gm_;
    Camera* camera_;
    GBuffer* g_buffer_;
    Lighting* lighting_;
    PostProcessing* post_processing_;
    uint64_t frame_counter_;
    double t0_;
    double t1_;
    LightSpotCVSM* flashlight_;  // Not owned here
    GeometryInstance* flashlight_model_;  // Not owned here
    Texture* background_tex_;

    // Some renderer states, including request flags
    bool reload_renderer_;
    jtil::math::Double2 mouse_wheel_pos_;
    jtil::math::Double2 mouse_wheel_pos_old;

    windowing::KeyboardCBFuncPtr keyboard_cb_;
    windowing::MousePosCBFuncPtr mouse_pos_cb_;
    windowing::MouseButCBFuncPtr mouse_button_cb_;
    windowing::MouseWheelCBFuncPtr mouse_wheel_cb_;
    windowing::CloseWndCBFuncPtr close_cb_;
    ResetScreenCBFuncPtr reset_screen_cb_;
 
    // Constructor / Destructor --> Private!  Use Init/ShutdownRenderer
    Renderer();
    ~Renderer();

    static void createNewRenderer();  // startup and when changing resolutions
    void init();
    void shutdown();
    void createWindow();
    static int windowClose();
    static void reloadRenderer();

    void resize(const uint32_t width, const uint32_t height);  // camera param
    void renderOutputFrameToScreen();
    void updateCameraFOVScreenSize();
    void fitCameraNearFarToObjects();
    void updateMatrices();
    void updateBoundingVolumes();
    void updateFlashlight(float dt);  // perform after updating camera view

    // Keyboard callbacks --> Renderer intersepts them first, then sends on
    // if the application has registered a callback.
    static void keyboardCB(int key, int scancode, int action, int mods);
    static void mousePosCB(double x, double y);
    static void mouseButtonCB(int button, int action, int mods);
    static void mouseWheelCB(double xoffset, double yoffset);

    // Non-copyable, non-assignable.
    Renderer(Renderer&);
    Renderer& operator=(const Renderer&);
  };
};  // namespace renderer
};  // namespace jtil

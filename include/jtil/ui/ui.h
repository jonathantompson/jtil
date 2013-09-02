//
//  ui.h
//
//  Created by Jonathan Tompson on 5/30/12.
//

#pragma once

#include "jtil/windowing/window.h"  // for Key
#include "jtil/renderer/renderer.h"
#include "jtil/data_str/vector.h"

#define UI_SETTINGS_DOC "settings.rml"
#define UI_FPS_DOC "fps_counter.rml"
#define UI_TEXT_DOC "text.rml"
#define UI_CROSSHAIRS_TEXTURE "resource_files/crosshairs.png"
#define UI_BUTTON_CALLBACK_START_HASH_SIZE 11
#define UI_APP_WND_START_HASH_SIZE 11
#define UI_FPS_POLLTIME 2  // seconds

namespace Rocket {namespace Core {template< typename T > class StringBase;};};
namespace Rocket { namespace Core { typedef StringBase< char > String; }; };
namespace Rocket { namespace Core { class Context; }; };
namespace Rocket { namespace Core { class Event; }; };
namespace Rocket { namespace Core { class Element; }; };
namespace Rocket { namespace Core { class ElementDocument; }; };

namespace jtil {
namespace data_str { template <class TKey, class TValue> class HashMap; }

namespace ui {

  class RocketSystemInterface;
  class RocketRenderInterface;
  class RocketFileInterface;
  class RocketEventListener;
  class RocketEvent;

  struct UIEnumVal {
  public:
    int          Value;
    const char * Label;
    UIEnumVal(const int Value_, const char * Label_) : Value(Value_), Label(Label_) {}
  };

  // Function pointer for button callbacks
  typedef void (*ButtonCallback) (void);

  class UI {
  public:
    // Constructor / Destructor
    UI();
    ~UI();

    // Functions used by the Renderer: You shouldn't need to touch these
    void init();
    void renderFrame() const;
    void setWindowSize(const int width, const int height);
    void update(const double dt);
    bool debugger_visible() const { return debugger_visible_; }
    void debugger_visible(const bool val);
    void setSettingsVisibility(const bool visible) const;
    void setRendererCheckboxVal(const std::string& name, 
      const bool val) const;
    void reloadUIContent();

    // Event handlers
    void keyboardInputCB(int key, int scancode, int action, int mods) const;
    void mousePosCB(double x, double y) const;
    void mouseButtonCB(int button, int action, int mods) const;
    void mouseWheelCB(double xoffset, double yoffset) const;
    void processEvent(const Rocket::Core::Event& event);

    inline bool ui_running() { return ui_running_; }
    // mouse_over_ui - Query if the mouse is currently over the UI
    inline bool mouse_over_ui() { return mouse_over_count_ > 1; }

    // UI Manipulation functions: Addition of elements
    void addCheckbox(const char* bool_setting_name, const char* text) const;
    void addHeadingText(const char* text, const char* elem_name = NULL) const;
    void addSelectbox(const char* int_setting_name, const char* text) const;
    void addSelectboxItem(const char* int_setting_name, const UIEnumVal& item) 
      const;
    void setCheckboxVal(const std::string& name, const bool val) const;
    void setSelectboxVal(const std::string& name, const int val) const;
    void addButton(const char* elem_name, const char* text, 
      ButtonCallback func) const;

    void createTextWindow(const char* wnd_name, const char* str);
    void setTextWindowString(const char* wnd_name, const char* text);
    void setTextWindowPos(const char* wnd_name, const math::Int2& pos);
    void setTextWindowVisibility(const char* wnd_name, const bool visible);

  private:
    bool ui_running_;
    bool debugger_visible_;
    RocketRenderInterface* render_interface_;
    RocketSystemInterface* system_interface_;
    RocketFileInterface* file_interface_;
    Rocket::Core::Context* context_;
    RocketEventListener* event_listener_;
    Rocket::Core::ElementDocument* app_doc_;
    Rocket::Core::ElementDocument* renderer_doc_;
    Rocket::Core::ElementDocument* fps_doc_;
    Rocket::Core::Element* app_content_;
    Rocket::Core::Element* renderer_content_;
    Rocket::Core::Element* fps_content_;
    data_str::HashMap<std::string, uint32_t>* app_text_str2ind_;
    data_str::Vector<Rocket::Core::ElementDocument*> app_text_doc_;
    data_str::Vector<Rocket::Core::Element*> app_text_content_;
    int mouse_over_count_;
    static const UIEnumVal res_combobox_vals_[];
    static const uint32_t num_res_combobox_vals_;
    static const UIEnumVal sm_res_combobox_vals_[];
    static const uint32_t num_sm_res_combobox_vals_;
    static const UIEnumVal render_output_combobox_vals_[];
    static const uint32_t num_render_output_combobox_vals_;
    static const UIEnumVal aa_combobox_vals_[];
    static const uint32_t num_aa_combobox_vals_;
    static const UIEnumVal tess_combobox_vals_[];
    static const uint32_t num_tess_combobox_vals_;
    data_str::HashMap<std::string, ButtonCallback>* button_callbacks_;
    static uint64_t event_count_;
    double frame_time_accum_;
    double min_fps_;
    double max_fps_;
    char fps_buffer_[256];
    uint64_t frame_count_;

    // Internal functions to create document elements: Here we can specify a
    // UI document destination.
    void addCheckbox(const char* bool_setting_name, const char* text, 
      Rocket::Core::ElementDocument* doc, 
      Rocket::Core::Element* content) const;
    void addHeadingText(const char* text,
      Rocket::Core::ElementDocument* doc, 
      Rocket::Core::Element* content,
      const char* elem_name = NULL)  const;
    void addSelectbox(const char* int_setting_name, const char* text, 
      const UIEnumVal box_vals[], const uint32_t num_vals, 
      Rocket::Core::ElementDocument* doc, 
      Rocket::Core::Element* content) const;
    // TO DO: Fix slider
    void addSlider(const char* name, const char* text, float min, float max, 
      Rocket::Core::ElementDocument* doc, 
      Rocket::Core::Element* content) const;
    void addButton(const char* elem_name, const char* text, 
      ButtonCallback func, Rocket::Core::ElementDocument* doc, 
      Rocket::Core::Element* content) const;
    void addLineBreak(Rocket::Core::ElementDocument* doc, 
      Rocket::Core::Element* content) const;
    void setCheckboxVal(const std::string& name, const bool val,
      Rocket::Core::ElementDocument* doc) const;
    void setSelectboxVal(const std::string& name, const int val, 
      Rocket::Core::ElementDocument* doc) const;

    // Inner helper functions
    static void loadRocketFontSafe(const Rocket::Core::String& file_name);
    void loadRocketDocument(const char* file, 
      Rocket::Core::ElementDocument*& doc, Rocket::Core::Element*& content) 
      const;
    void loadRendererElements() const;
    void loadAppElements() const;
    void loadFPSElements() const;
    static void setVisibility(const bool visible, 
      Rocket::Core::ElementDocument*& doc);
    void loadUIContent();
    void createUIContext();
    static void showRendererDoc();
    static void hideRendererDoc();
    static void showAppDoc();
    static void hideAppDoc();
    void setDocumentTitle(Rocket::Core::ElementDocument* doc,
      const std::string& title) const;
    void addCloseButton(Rocket::Core::ElementDocument* doc,
      ButtonCallback close_func, const char* close_button_name) const;
    void setDocSize(const math::Int2& size, 
      Rocket::Core::ElementDocument* doc);
    const uint32_t getTextWindowID(const std::string& name) const;

    // Non-copyable, non-assignable.
    UI(UI&);
    UI& operator=(const UI&);
  };
};  // namespace ui
};  // namespace jtil

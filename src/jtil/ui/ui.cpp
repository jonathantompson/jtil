#include <sstream>
#include <iostream>
#include <string>
#include <Rocket/Core/Input.h>
#include <Rocket/Controls.h>
#include <Rocket/Debugger/Debugger.h>
#include <Rocket/Core.h>
#include <Rocket/Core/Types.h>
#include "jtil/ui/ui.h"
#include "jtil/ui/rocket_render_interface.h"
#include "jtil/ui/rocket_system_interface.h"
#include "jtil/ui/rocket_event_listener.h"
#include "jtil/ui/rocket_file_interface.h"
#include "jtil/settings/settings_manager.h"
#include "jtil/renderer/renderer.h"
#include "jtil/renderer/lights/light_spot_cvsm.h"
#include "jtil/renderer/post_processing.h"  // for FXAAType
#include "jtil/windowing/window.h"
#include "jtil/exceptions/wruntime_error.h"
#include "jtil/data_str/hash_map.h"
#include "jtil/data_str/vector.h"
#include "jtil/data_str/hash_funcs.h"
#include "jtil/string_util/string_util.h"

#define ROCKET_RESOURCE_PATH "jtil_resource_files/"
#define SAFE_REMOVE_REFERENCE(x) if (x) { x->RemoveReference(); x = NULL; }
#define SAFE_DELETE(x) if (x) { delete x; x = NULL; }

using std::wstring;
using std::wruntime_error;
using Rocket::Core::String;
using Rocket::Core::Element;
using Rocket::Core::ElementText;
using Rocket::Core::Factory;
using Rocket::Core::Variant;
using Rocket::Controls::ElementFormControlSelect;
using Rocket::Core::Vector2i;
using Rocket::Core::Vector2f;

namespace jtil {

using windowing::Window;
using renderer::Renderer;
using renderer::RenderOutput;
using renderer::FXAAType;
using data_str::HashMap;
using data_str::HashString;
using data_str::Vector;
using string_util::Str2Num;

namespace ui {

  const UIEnumVal UI::res_combobox_vals_[] = { 
    UIEnumVal(windowing::RES_640_480, "640 x 480"), 
    UIEnumVal(windowing::RES_800_600, "800 x 600"), 
    UIEnumVal(windowing::RES_1280_1024, "1280 x 1024"), 
    UIEnumVal(windowing::RES_1920_1200, "1920 x 1200"),
    UIEnumVal(windowing::RES_1280_720, "1280 x 720"),
    UIEnumVal(windowing::RES_1920_1080, "1920 x 1080"),
    UIEnumVal(windowing::RES_1000_1000, "1000 x 1000"),
    UIEnumVal(windowing::RES_960_720, "960 x 720"),
    UIEnumVal(windowing::RES_1280_960, "1280 x 960"),
    UIEnumVal(windowing::RES_1920_1440, "1920 x 1440"),
  };
  const uint32_t UI::num_res_combobox_vals_ = 10;

  const UIEnumVal UI::sm_res_combobox_vals_[] = { 
    UIEnumVal(renderer::VSM_RES_256, "256"), 
    UIEnumVal(renderer::VSM_RES_512, "512"), 
    UIEnumVal(renderer::VSM_RES_1024, "1024"), 
    UIEnumVal(renderer::VSM_RES_2048, "2048"),
  };
  const uint32_t UI::num_sm_res_combobox_vals_ = 4;

  const UIEnumVal UI::render_output_combobox_vals_[] = { 
    UIEnumVal(RenderOutput::FULL_SCENE, "Full Scene"), 
    UIEnumVal(RenderOutput::GBUFFER, "G-Buffer"), 
    UIEnumVal(RenderOutput::ALBEDO, "Albedo"), 
    UIEnumVal(RenderOutput::NORMAL, "Normal"), 
    UIEnumVal(RenderOutput::DEPTH, "Depth"), 
    UIEnumVal(RenderOutput::VIEW_POS, "View Pos."),
    UIEnumVal(RenderOutput::LIGHTING_STENCIL, "Lighting Stencil"), 
    UIEnumVal(RenderOutput::LIGHT_ACCUMULATION_DIFFUSE, "Light Diffuse"), 
    UIEnumVal(RenderOutput::LIGHT_ACCUMULATION_SPECULAR, "Light Specular"), 
    UIEnumVal(RenderOutput::SHADOW_MAP, "Shadow Map"), 
    UIEnumVal(RenderOutput::AMBIENT_OCCLUSION, "Ambient Occl."), 
    UIEnumVal(RenderOutput::VELOCITY, "Velocity"),
    UIEnumVal(RenderOutput::LUMINANCE, "Luminance"), 
  };
  const uint32_t UI::num_render_output_combobox_vals_ = 13;

  const UIEnumVal UI::aa_combobox_vals_[] = { 
    UIEnumVal(FXAAType::AA_OFF, "Off"), 
    UIEnumVal(FXAAType::FXAA_LQ_FULL, "FXAA LQ"), 
    UIEnumVal(FXAAType::FXAA_LQ_EDGE_AWARE, "FXAA LQ Edge"), 
    UIEnumVal(FXAAType::FXAA_HQ_FULL, "FXAA HQ"), 
    UIEnumVal(FXAAType::FXAA_HQ_EDGE_AWARE, "FXAA HQ Edge"), 
  };
  const uint32_t UI::num_aa_combobox_vals_ = 5;
  const UIEnumVal UI::tess_combobox_vals_[] = { 
    UIEnumVal(1, "1"), 
    UIEnumVal(2, "2"), 
    UIEnumVal(4, "4"), 
    UIEnumVal(8, "8"), 
    UIEnumVal(16, "16"), 
    UIEnumVal(32, "32"),
  };
  const uint32_t UI::num_tess_combobox_vals_ = 6;
   
  UI::UI() {
    ui_running_ = false;
    render_interface_ = NULL;
    system_interface_ = NULL;
    file_interface_ = NULL;
    event_listener_ = NULL;
    renderer_doc_ = NULL;
    app_doc_ = NULL;
    fps_doc_ = NULL;
    mouse_over_count_ = 0;
    context_ = NULL;
    button_callbacks_ = NULL;
    app_text_str2ind_ = NULL;
    app_content_ = NULL;
    renderer_content_ = NULL;
    fps_content_ = NULL;
    frame_time_accum_ = 0;
    frame_count_ = 0;
    min_fps_ = std::numeric_limits<double>::infinity();
    max_fps_ = 0;
  }

  UI::~UI() {
    // The order of calls in this destructor is VERY important.  You must
    // remove document and context references first.  Request shutdown, THEN
    // delete the interface elements.
    SAFE_REMOVE_REFERENCE(app_doc_);
    SAFE_REMOVE_REFERENCE(renderer_doc_);
    SAFE_REMOVE_REFERENCE(fps_doc_);
    for (uint32_t i = 0; i < app_text_doc_.size(); i++) {
      SAFE_REMOVE_REFERENCE(app_text_doc_[i]);
    }
    SAFE_REMOVE_REFERENCE(context_);
    Rocket::Core::Shutdown();
    
    ui_running_ = false;
    SAFE_DELETE(render_interface_);
    SAFE_DELETE(file_interface_);
    SAFE_DELETE(event_listener_);
    SAFE_DELETE(system_interface_);
    SAFE_DELETE(button_callbacks_);
    SAFE_DELETE(app_text_str2ind_);
  }

  void UI::loadRocketFontSafe(const String& file_name) {
    if (!Rocket::Core::FontDatabase::LoadFontFace(file_name)) {
      wstring err = wstring(L"UI::loadRocketFontSafe() - ERROR: LoadFontFace"
        L" failed trying to load font: ") + 
        string_util::ToWideString(file_name.CString());
      throw wruntime_error(err);
    }
  }

  void UI::init() {
    if (ui_running_) {
      throw std::wruntime_error(L"UI::init() - UI is already running!");
    }

    button_callbacks_ = new HashMap<std::string, ButtonCallback>(
      UI_BUTTON_CALLBACK_START_HASH_SIZE, &data_str::HashString);
    app_text_str2ind_ = new HashMap<std::string, uint32_t>(
      UI_APP_WND_START_HASH_SIZE, &data_str::HashString);
    render_interface_ = new RocketRenderInterface(Renderer::g_renderer());
    system_interface_ = new RocketSystemInterface();
    file_interface_ = new RocketFileInterface(string_util::getJTilDirEnvVar() +
      ROCKET_RESOURCE_PATH);
    event_listener_ = new RocketEventListener(this);

    Rocket::Core::SetFileInterface(file_interface_);
    Rocket::Core::SetRenderInterface(render_interface_);
    Rocket::Core::SetSystemInterface(system_interface_);

    if(!Rocket::Core::Initialise()) {
		  throw wruntime_error(L"UI::init() - ERROR: Rocket::Core::"
        L"Initialise() failed!");
    }
    Rocket::Controls::Initialise();

    loadRocketFontSafe("Delicious-Bold.otf");
    loadRocketFontSafe("Delicious-BoldItalic.otf");
    loadRocketFontSafe("Delicious-Italic.otf");
    loadRocketFontSafe("Delicious-Roman.otf");

    createUIContext();

    loadUIContent();

    ui_running_ = true;
  }

  void UI::createUIContext() {
    Vector2i dimensions(Renderer::g_renderer()->width(), 
      Renderer::g_renderer()->height());
    context_ = Rocket::Core::CreateContext("default", dimensions);
    if (context_ == NULL) {
      throw wruntime_error(L"UI::init() - ERROR: Rocket::Core::" 
        L"CreateContext() failed!");
    }
    context_->AddEventListener("mouseover", event_listener_);
    context_->AddEventListener("mouseout", event_listener_);
    context_->AddEventListener("dragstart", event_listener_);
    context_->AddEventListener("dragend", event_listener_);

    Rocket::Debugger::Initialise(context_);
    debugger_visible_ = false;
    Rocket::Debugger::SetVisible(debugger_visible_);
  }

  void UI::loadUIContent() {
    loadRocketDocument(UI_FPS_DOC, fps_doc_, fps_content_);
    loadFPSElements();
    bool vis = true;
    GET_SETTING("render_ui_fps", bool, vis);
    setVisibility(vis, fps_doc_);

    math::Int2 ui_size;
    ui_size.zeros();
    GET_SETTING("ui_starting_width", int, ui_size[0]);
    GET_SETTING("ui_starting_height", int, ui_size[1]);

    loadRocketDocument(UI_SETTINGS_DOC, renderer_doc_, renderer_content_);
    loadRendererElements();
    GET_SETTING("render_ui_renderer", bool, vis);
    setVisibility(vis, renderer_doc_);
    setDocSize(ui_size, renderer_doc_);

    loadRocketDocument(UI_SETTINGS_DOC, app_doc_, app_content_);
    loadAppElements();
    GET_SETTING("render_ui_app", bool, vis);
    setVisibility(vis, app_doc_);
    setDocSize(ui_size, app_doc_);

#if defined(__APPLE__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    
    // Shift the render document so it's off to the right
    float left_render = 0.0f;
    left_render = renderer_doc_->GetProperty("left")->value.Get<float>();
    float width_app = 0.0f;
    width_app = renderer_doc_->GetProperty("width")->value.Get<float>();
    float val_float = left_render + width_app;
    std::string val = string_util::Num2Str<float>(val_float);
    const char* val_c_str = val.c_str();
    renderer_doc_->SetProperty("left", val_c_str);
    renderer_doc_->UpdateLayout();
    
#if defined(__APPLE__)
  #pragma GCC diagnostic pop
#endif    
  }

  void UI::loadRocketDocument(const char* file, 
    Rocket::Core::ElementDocument*& doc, Rocket::Core::Element*& content) 
    const {
    if (doc) {
      doc->RemoveReference();
    }
    doc = context_->LoadDocument(file);
    if (!doc) {
      throw wruntime_error(std::string("UI::loadRocketDocument() - ERROR:"
        " failed trying to load document: ") + std::string(file));
    }
    content = doc->GetElementById("content");
    if (content == NULL) {
      throw wruntime_error(L"UI::loadRocketDocument() - couldnt find content");
    }
  }

  void UI::setDocumentTitle(Rocket::Core::ElementDocument* doc,
    const std::string& text) const {
    Element* title_elem = doc->GetElementById("title");
    if (title_elem) {
      title_elem->SetInnerRML(text.c_str());
    } else {
      throw std::wruntime_error(" UI::setDocumentTitle() - ERROR: "
        "Couldn't find title element!");
    }
  }

  void UI::setDocSize(const math::Int2& size, 
    Rocket::Core::ElementDocument* doc) {
    doc->SetProperty("width", string_util::Num2Str<int>(size[0]).c_str());
    doc->SetProperty("height", string_util::Num2Str<int>(size[1]).c_str());
    doc->UpdateLayout();
  }

  void UI::addCloseButton(Rocket::Core::ElementDocument* doc, 
    ButtonCallback close_func, const char* close_button_name) const {
    Element* title_elem = doc->GetElementById("title_bar");
    if (!title_elem) {
      throw std::wruntime_error(" UI::addCloseButton() - ERROR: "
        "Couldn't find title element!");
    }

    Element* sel_elem = doc->CreateElement("closebutton");
    if (sel_elem == NULL) {
      throw wruntime_error("UI::addCloseButton() - couldn't create close "
        "elem!");
    }
    sel_elem->SetId("close_button");
    const char* val = "button";
    const char* name = "type";
    sel_elem->SetAttribute(name, val);
    // Generate a unique identifier
    sel_elem->SetAttribute("value", close_button_name);
    sel_elem->AddEventListener("click", event_listener_, true);
    button_callbacks_->insert(close_button_name, close_func);

    title_elem->AppendChild(sel_elem);
    sel_elem->RemoveReference();

    // addLineBreak(doc, content);
    //title_elem->AppendChild();
  }

  void UI::reloadUIContent() {
    UI* g_ui = Renderer::g_renderer()->ui();
    SAFE_REMOVE_REFERENCE(g_ui->renderer_doc_);
    SAFE_REMOVE_REFERENCE(g_ui->fps_doc_);
    for (uint32_t i = 0; i < app_text_doc_.size(); i++) {
      SAFE_REMOVE_REFERENCE(app_text_doc_[i]);
    }
    g_ui->context_->UnloadAllDocuments();
    Rocket::Core::Factory::ClearStyleSheetCache();
    Rocket::Core::ReleaseTextures();
    Rocket::Core::ReleaseCompiledGeometries();

    g_ui->context_->Update();  // Releases unloaded documents

    g_ui->loadUIContent();
  }

  void UI::loadFPSElements() const {
    // Create dynamic elements here:
    addHeadingText("FPS:", fps_doc_, fps_content_, "fps_text");
  }

  void UI::addCheckbox(const char* bool_setting_name, const char* text) const {
    addCheckbox(bool_setting_name, text, app_doc_, app_content_);
  }

  void UI::addCheckbox(const char* name, const char* text, 
    Rocket::Core::ElementDocument* doc, Rocket::Core::Element* content) const {
    if (!doc) {
      doc = renderer_doc_; 
    }
    if (!content) {
      content = renderer_content_; 
    }

    // Add a checkbox input element
    Element* checkbox_elem = doc->CreateElement("input");
    checkbox_elem->SetAttribute("type", "checkbox");
    checkbox_elem->SetAttribute("value", name);
    checkbox_elem->SetProperty("overflow-x", "auto");
    checkbox_elem->SetProperty("overflow-y", "auto");
    checkbox_elem->SetProperty("clip", "auto");
    checkbox_elem->SetId(name);
    checkbox_elem->AddEventListener("change", event_listener_, false);

    bool val;
    GET_SETTING(name, bool, val);
    if (val) {
      checkbox_elem->SetAttribute("checked", "");
    } else {
      checkbox_elem->RemoveAttribute("checked");
    }
    content->AppendChild(checkbox_elem);
    checkbox_elem->RemoveReference();

    // Add a text element for the text string
    Element* text_elem = doc->CreateTextNode(text);
    if (text_elem == NULL) {
      throw wruntime_error(L"UI::addCheckbox() - couldn't create text elem!");
    }
    text_elem->SetInnerRML("<br/>");
    content->AppendChild(text_elem);
    text_elem->RemoveReference();
  }

  void UI::addHeadingText(const char* text, const char* elem_name) const {
    addHeadingText(text, app_doc_, app_content_, elem_name);
  }

  void UI::addHeadingText(const char* text, Rocket::Core::ElementDocument* doc, 
    Rocket::Core::Element* content, const char* name) const {
    // Add a text element for the text string
    Element* text_elem = doc->CreateTextNode(text);
    if (text_elem == NULL) {
      throw wruntime_error(L"UI::addText() - couldn't create text elem!");
    }
    if (name != NULL) {
      text_elem->SetId(name);
    }
    text_elem->SetInnerRML("<br/>");
    content->AppendChild(text_elem);
    text_elem->RemoveReference();
  }
  
  void UI::setSelectboxVal(const std::string& name, const int val) const {
    setSelectboxVal(name, val, app_doc_);
  }

  void UI::addButton(const char* elem_name, const char* text, 
    ButtonCallback func) const { 
    addButton(elem_name, text, func, app_doc_, app_content_);
  }

  void UI::setSelectboxVal(const std::string& name, const int val, 
    Rocket::Core::ElementDocument* doc) const {
    Element* elem = doc->GetElementById(name.c_str());
    if (elem == NULL) {
      std::wstringstream ss;
      ss << L"UI::changeCheckboxValue() - ERROR: Couldn't find an element";
      ss << L" with the ID: " << name.c_str();
      throw wruntime_error(ss.str());
    }
    String elem_type = elem->GetAttribute("type")->Get<String>();
    if (elem_type != "selectbox") {
      std::wstringstream ss;
      ss << L"UI::setSelectboxVal() - ERROR: Found an element";
      ss << L" with the ID: " << name.c_str() << L", but it is not a";
      ss << L" selectbox";
      throw wruntime_error(ss.str());
    }

    int cval = -1;
    for (int32_t i = 0; i < elem->GetNumChildren() && cval == -1; i++) {
      Element* child_elem = elem->GetChild(i);
      if (Str2Num<int>(child_elem->GetAttribute("value")->Get<String>().CString()) == 
        val) {
        cval = i;
      }
    }
    if (cval != -1) {
      dynamic_cast<ElementFormControlSelect*>(elem)->SetSelection(cval);
    }
  }

  void UI::addSelectbox(const char* int_setting_name, const char* text, 
    const UIEnumVal box_vals[], const uint32_t num_vals, 
    Rocket::Core::ElementDocument* doc, Rocket::Core::Element* content) const {
    // Create the containing select element
    Element* sel_elem = doc->CreateElement("select");
    if (sel_elem == NULL) {
      throw wruntime_error(L"UI::addSelectbox() - couldn't create select "
        L"elem!");
    }
    sel_elem->SetId(int_setting_name);
    sel_elem->SetAttribute("type", "selectbox");
    sel_elem->SetAttribute("value", int_setting_name);
    content->AppendChild(sel_elem);

    // Add the option elements:
    for (uint32_t i = 0; i < num_vals; i ++) {
      Element* option_elem = doc->CreateElement("option");
      if (option_elem == NULL) {
        throw wruntime_error(L"UI::addSelectbox() - couldn't create option "
          L"elem!");
      }
      option_elem->SetInnerRML(box_vals[i].Label);
      option_elem->SetAttribute("value", box_vals[i].Value);
      sel_elem->AppendChild(option_elem);
      option_elem->RemoveReference();
    }

    int value = 0;
    GET_SETTING(int_setting_name, int, value);
    // Now we need to find out which selection value in the array corresponds
    // to the selection (they might be out of order compared to the enum val)
    int cval = -1;
    for (uint32_t i = 0; i < num_vals && cval == -1; i++) {
      if (box_vals[i].Value == value) {
        cval = i;
      }
    }
    if (cval == -1) {
      throw wruntime_error("UI::addSelectbox() - A combo-box value "
        "corresponding to the current setting value does not exist!");
    }
    dynamic_cast<ElementFormControlSelect*>(sel_elem)->SetSelection(cval);

    sel_elem->AddEventListener("change", event_listener_, false);
    sel_elem->RemoveReference();

    // Add a text element for the text string
    Element* text_elem = doc->CreateTextNode(text);
    if (text_elem == NULL) {
      throw wruntime_error(L"UI::addSelectbox() - couldn't create text elem!");
    }
    text_elem->SetInnerRML("<br/>");
    content->AppendChild(text_elem);
    text_elem->RemoveReference();
  }

  void UI::addSelectboxItem(const char* int_setting_name, 
    const UIEnumVal& item) const {
    Element* sel_elem = app_content_->GetElementById(int_setting_name);
    if (app_content_ == NULL) {
      throw wruntime_error(L"UI::addSelectboxVal() - couldn't find selectbox "
        L"elem!");
    }

    // Add the option element:
    Element* option_elem = app_doc_->CreateElement("option");
    if (option_elem == NULL) {
      throw wruntime_error(L"UI::addSelectbox() - couldn't create option "
        L"elem!");
    }
    option_elem->SetInnerRML(item.Label);
    option_elem->SetAttribute("value", item.Value);
    sel_elem->AppendChild(option_elem);
    option_elem->RemoveReference();

    int value = 0;
    GET_SETTING(int_setting_name, int, value);
    // Now we need to find out which selection value in the array corresponds
    // to the selection (they might be out of order compared to the enum val)
    int cval = -1;
    for (int32_t i = 0; i < sel_elem->GetNumChildren() && cval == -1; i++) {
      Element* elem = sel_elem->GetChild(i);
      if (Str2Num<int>(elem->GetAttribute("value")->Get<String>().CString()) == 
        value) {
        cval = i;
      }
    }
    if (cval != -1) {
      dynamic_cast<ElementFormControlSelect*>(sel_elem)->SetSelection(cval);
    }
  }

  void UI::addSelectbox(const char* int_setting_name, const char* text) const {
    // Create the containing select element
    Element* sel_elem = app_doc_->CreateElement("select");
    if (sel_elem == NULL) {
      throw wruntime_error(L"UI::addSelectbox() - couldn't create select "
        L"elem!");
    }
    sel_elem->SetId(int_setting_name);
    sel_elem->SetAttribute("type", "selectbox");
    sel_elem->SetAttribute("value", int_setting_name);
    app_content_->AppendChild(sel_elem);

    sel_elem->AddEventListener("change", event_listener_, false);
    sel_elem->RemoveReference();

    // Add a text element for the text string
    Element* text_elem = app_doc_->CreateTextNode(text);
    if (text_elem == NULL) {
      throw wruntime_error(L"UI::addSelectbox() - couldn't create text elem!");
    }
    text_elem->SetInnerRML("<br/>");
    app_content_->AppendChild(text_elem);
    text_elem->RemoveReference();
  }

  void UI::addSlider(const char* name, const char* text, float min, float max, 
    Rocket::Core::ElementDocument* doc, Rocket::Core::Element* content) const {
    /*
    if (!doc) {
      doc = renderer_doc_; 
    }
    if (!content) {
      content = renderer_content_; 
    }

    // Create the containing select element
    Element* sel_elem = doc->CreateElement("input");
    if (sel_elem == NULL) {
      throw wruntime_error(L"UI::addSlider() - couldn't create range elem!");
    }
    sel_elem->SetId(name);
    sel_elem->SetAttribute("type", "range.horizontal");
    sel_elem->SetAttribute("min", "0");
    sel_elem->SetAttribute("max", "100");
    //sel_elem->SetAttribute("value", name);
    content->AppendChild(sel_elem);
    */
    /*
    // Add the option elements:
    for (uint32_t i = 0; i < num_vals; i ++) {
      Element* option_elem = doc->CreateElement("option");
      if (option_elem == NULL) {
        throw wruntime_error(L"UI::addSelectbox() - couldn't create option "
          L"elem!");
      }
      option_elem->SetInnerRML(box_vals[i].Label);
      option_elem->SetAttribute("value", box_vals[i].Value);
      sel_elem->AppendChild(option_elem);
      option_elem->RemoveReference();
    }

    int value = 0;
    GET_SETTING(name, int, value);
    // Now we need to find out which selection value in the array corresponds
    // to the selection (they might be out of order compared to the enum val)
    int cval = -1;
    for (uint32_t i = 0; i < num_vals && cval == -1; i++) {
      if (box_vals[i].Value == value) {
        cval = i;
      }
    }
    if (cval == -1) {
      throw wruntime_error("UI::addSelectbox() - A combo-box value "
        "corresponding to the current setting value does not exist!");
    }
    dynamic_cast<ElementFormControlSelect*>(sel_elem)->SetSelection(cval);

    sel_elem->AddEventListener("change", event_listener_, false);
    sel_elem->RemoveReference();

    // Add a text element for the text string
    Element* text_elem = doc->CreateTextNode(text);
    if (text_elem == NULL) {
      throw wruntime_error(L"UI::addSelectbox() - couldn't create text elem!");
    }
    text_elem->SetInnerRML("<br/>");
    content->AppendChild(text_elem);
    text_elem->RemoveReference();
    */
  }

  void UI::addButton(const char* name, const char* text, ButtonCallback func,
    Rocket::Core::ElementDocument* doc, Rocket::Core::Element* content) const {
    // Create the containing select element
    Element* sel_elem = doc->CreateElement("button");
    if (sel_elem == NULL) {
      throw wruntime_error(L"UI::addButton() - couldn't create button elem!");
    }
    sel_elem->SetId(name);
    sel_elem->SetAttribute("type", "button");
    sel_elem->SetAttribute("value", name);
    sel_elem->AddEventListener("click", event_listener_, true);
    button_callbacks_->insert(name, func);

    Element* text_elem = doc->CreateTextNode(text);
    if (text_elem == NULL) {
      throw wruntime_error(L"UI::addButton() - couldn't create text elem!");
    }
    sel_elem->AppendChild(text_elem);
    text_elem->RemoveReference();

    content->AppendChild(sel_elem);
    sel_elem->RemoveReference();

    addLineBreak(doc, content);
  }

  void UI::createTextWindow(const char* wnd_name, const char* str) {
    uint32_t index;
    if (app_text_str2ind_->lookup(wnd_name, index)) {
      throw std::wruntime_error("UI::createTextWindow() - ERROR: "
        "A window with this name already exists");
    }
    app_text_doc_.pushBack(NULL);
    app_text_content_.pushBack(NULL);
    index = app_text_doc_.size() - 1;
    app_text_str2ind_->insert(wnd_name, index);

    loadRocketDocument(UI_TEXT_DOC, app_text_doc_[index], 
      app_text_content_[index]);
    // setDocumentTitle(app_text_doc_[index], wnd_name);
    addHeadingText(str, app_text_doc_[index], app_text_content_[index], 
      "app_text");
  }

  const uint32_t UI::getTextWindowID(const std::string& name) const {
    uint32_t index;
    if (!app_text_str2ind_->lookup(name, index)) {
      throw std::wruntime_error("UI::getTextWindowID() - ERROR: "
        "A window with this name does not exist!");
    }
    return index;
  }

  void UI::setTextWindowString(const char* wnd_name, const char* text) {
    uint32_t index = getTextWindowID(wnd_name);
    Element* elem = app_text_doc_[index]->GetElementById("app_text");
    if (elem == NULL) {
      throw wruntime_error("UI::setTextWindowString() - INTERNAL ERROR: "
        " Couldn't not find text element to modify!");
    }
    reinterpret_cast<ElementText*>(elem)->SetText(text);
  }

  void UI::setTextWindowPos(const char* wnd_name, const math::Int2& pos) {
    uint32_t index = getTextWindowID(wnd_name);
    app_text_doc_[index]->SetProperty("left", 
      string_util::Num2Str<int>(pos[0]).c_str());
    app_text_doc_[index]->SetProperty("top", 
      string_util::Num2Str<int>(pos[1]).c_str());
  }

  void UI::setTextWindowVisibility(const char* wnd_name, const bool visible) {
    uint32_t index = getTextWindowID(wnd_name);
    setVisibility(visible, app_text_doc_[index]);
  }

  void UI::addLineBreak(Rocket::Core::ElementDocument* doc, 
    Rocket::Core::Element* content) const {
    // Create an empty text element so we can insert a line break
    Element* text_elem = doc->CreateTextNode(" ");
    if (text_elem == NULL) {
      throw wruntime_error(L"UI::addButton() - couldn't create text elem!");
    }
    content->AppendChild(text_elem);
    text_elem->SetInnerRML("<br/>");
    text_elem->RemoveReference();
  }

  void UI::renderFrame() const {
    if (!ui_running_) {
      return;
    }
    bool ui_render_crosshairs = true;
    GET_SETTING("ui_render_crosshairs", bool, ui_render_crosshairs);
    if (ui_render_crosshairs) {
      render_interface_->renderCrosshairs();
    }
    if (context_) {
      context_->Render();
    }
  }

  void UI::keyboardInputCB(int key, int scancode, int action, int mods) const {
    if (!ui_running_) {
      return;
    }
    if (context_) {
      if (action == PRESSED) {
        context_->ProcessKeyDown(system_interface_->TranslateKey(key), 
          system_interface_->GetKeyModifiers(Renderer::g_renderer()));
      } else {
        context_->ProcessKeyUp(system_interface_->TranslateKey(key), 
          system_interface_->GetKeyModifiers(Renderer::g_renderer()));
      }
    }
  }

  void UI::mousePosCB(double x, double y) const {
    if (!ui_running_) {
      return;
    }
    if (context_) {
      context_->ProcessMouseMove((int)floor(x), (int)floor(y), 
        system_interface_->GetKeyModifiers(Renderer::g_renderer()));
    }
  }

  void UI::mouseButtonCB(int button, int action, int mods) const {
    if (!ui_running_) {
      return;
    }
    if (context_) {
      if (action == PRESSED) {
        context_->ProcessMouseButtonDown(button, 
          system_interface_->GetKeyModifiers(Renderer::g_renderer()));
      } else {
        context_->ProcessMouseButtonUp(button, 
          system_interface_->GetKeyModifiers(Renderer::g_renderer()));
      }
    }
  }

  void UI::mouseWheelCB(double xoffset, double yoffset) const {
    if (!ui_running_) {
      return;
    }
    context_->ProcessMouseWheel(-(int)floor(yoffset), 
      system_interface_->GetKeyModifiers(Renderer::g_renderer()));
  }

  void UI::setWindowSize(const int width, const int height) {
    if (!ui_running_) {
      return;
    }
    if (context_) {
      Vector2i dimensions(width, height);
      context_->SetDimensions(dimensions);
    }
    if (render_interface_) {
      render_interface_->resize(width, height);
    }
    // loadDocumentAndElements();  // Reload the document
  }

  void UI::update(const double dt) {
    bool render_ui_fps = true;
    GET_SETTING("render_ui_fps", bool, render_ui_fps);
    setVisibility(render_ui_fps, fps_doc_);
    if (render_ui_fps) {
      // Update the framerate counter
      frame_time_accum_ += dt;
      frame_count_++;
      double cur_frame_fps = 1.0 / dt;
      min_fps_ = std::min<double>(min_fps_, cur_frame_fps);
      max_fps_ = std::max<double>(max_fps_, cur_frame_fps);
      if (frame_time_accum_ >= UI_FPS_POLLTIME) {
        double fps = frame_count_ / frame_time_accum_;
        if (fps_doc_) {
          Element* elem = fps_doc_->GetElementById("fps_text");
          if (elem == NULL) {
            throw wruntime_error(L"UI::update() - couldnt find fps_text!");
          }
          snprintf(fps_buffer_, 255, "FPS: %.1f (AVE) %.1f (MIN) %.1f (MAX)",
            fps, min_fps_, max_fps_);
          reinterpret_cast<ElementText*>(elem)->SetText(fps_buffer_);
        }
        min_fps_ = std::numeric_limits<double>::infinity();
        max_fps_ = 0;
        frame_count_ = 0;
        frame_time_accum_ = 0;
      }
    }

    if (context_) {
      context_->Update();
    }
  }

  void UI::debugger_visible(bool val) {
    debugger_visible_ = val;
    Rocket::Debugger::SetVisible(val);
  }

  uint64_t UI::event_count_ = 0;
  void UI::processEvent(const Rocket::Core::Event& event) {
    Rocket::Core::String type = event.GetType();
    event_count_++;

    //if (event == "resize") {
    //  if (renderer_content_ != NULL && renderer_content_->HasChildNodes()) {
    //    renderer_content_->GetLastChild()->ScrollIntoView();
    //  }
    //  return;
    //}

    if (type == "mouseover" || type == "dragstart") {
      mouse_over_count_++;
    } else if (type == "mouseout" || type == "dragend") {
      mouse_over_count_--;
    } else if (type == "change") {
      // An input element changed:
      Element* target = event.GetTargetElement();
      if (target != NULL) {
        // All our "changable" elements need to have a type attribute (we
        // need to put them there).  Make sure this one has one and that
        // it is of type string.
        if (target->GetAttribute("type") == NULL || 
          target->GetAttribute("type")->GetType() != Variant::Type::STRING) {
            std::wstringstream ss;
            ss << L"UI::processEvent() - ERROR: the current target element";
            ss << L" either does not have an attribute named type or that";
            ss << L" attribute is not of type string";
            throw wruntime_error(ss.str());
        }
        String elem_type = target->GetAttribute("type")->Get<String>();
        // Do the same for the value element (which stores the setting string)
        if (target->GetAttribute("value") == NULL || 
          target->GetAttribute("value")->GetType() != Variant::Type::STRING) {
            std::wstringstream ss;
            ss << L"UI::processEvent() - ERROR: the current target element";
            ss << L" either does not have an attribute named value or that";
            ss << L" attribute is not of type string";
            throw wruntime_error(ss.str());
        }
        String val_str = target->GetAttribute("value")->Get<String>();

        // Now handle the different types of elements
        if (elem_type == "checkbox") {  // Handle checkbox
          bool old_value = false;
          GET_SETTING(val_str.CString(), bool, old_value);

          // Set the internal setting (in the setting manager)
          bool value = false;
          if (target->GetAttribute("checked")) {
            value = true;
          }
          SET_SETTING(val_str.CString(), bool, value);

          // Handle special cases (that need to do something immediately)
          if (val_str == "fullscreen") {
            if (old_value != value) {
              Renderer::requestReloadRenderer();
              return;
            }
          }

          // Handle special cases that require the app to do something after 
          // the variable has been set.
          if (val_str == "render_ui") {
            setVisibility(value, renderer_doc_);
          }
        } else if (elem_type == "selectbox") {
          Rocket::Controls::ElementFormControlSelect* select = 
            dynamic_cast<Rocket::Controls::ElementFormControlSelect*>(target);
          int cval = select->GetSelection();
          Rocket::Controls::SelectOption* sel_opt = select->GetOption(cval);
          int val = Str2Num<int>(sel_opt->GetValue().CString());

          SET_SETTING(val_str.CString(), int, val);

          // Handle special cases that require the app to do something after
          // the variable has been set
          if (val_str == "screen_resolution") {
            Renderer::requestReloadRenderer();
          }
        }
      }  // end if (target != NULL)
    } else if (type == "click") {
      Element* target = event.GetTargetElement();
      if (target != NULL) {
        String val_str = target->GetAttribute("value")->Get<String>();
        ButtonCallback func;
        if (!button_callbacks_->lookup(val_str.CString(), func)) {
          throw wruntime_error(std::string("UI::processEvent() - No button "
            "callback with name ") + val_str.CString());
        }
        if (func != NULL) {
          func();
        }
      } else {
        throw wruntime_error(L"UI::processEvent() - Button has no target!");
      }
    }
    if (mouse_over_count_ < 0) {
      throw wruntime_error("UI::processEvent() - ERROR: mouse_over_count < 0");
    }
  }

  void UI::setVisibility(const bool vis, Rocket::Core::ElementDocument*& doc) {
    if (vis) {
      doc->Show();
    } else {
      doc->Hide();
    }
  }

  void UI::setSettingsVisibility(const bool visible) const {
    if (visible) {
      app_doc_->Show();
    } else {
      app_doc_->Hide();
    }
  }

  void UI::setRendererCheckboxVal(const std::string& name, const bool val) 
    const {
    setCheckboxVal(name, val, renderer_doc_);
  } 

  void UI::setCheckboxVal(const std::string& name, const bool val) 
    const {
    setCheckboxVal(name, val, app_doc_);
  } 

  void UI::setCheckboxVal(const std::string& name, const bool val,
    Rocket::Core::ElementDocument* doc) const {
    Element* elem = doc->GetElementById(String(name.c_str()));
    if (elem == NULL) {
      std::wstringstream ss;
      ss << L"UI::changeCheckboxValue() - ERROR: Couldn't find an element";
      ss << L" with the ID: " << name.c_str();
      throw wruntime_error(ss.str());
    }
    String elem_type = elem->GetAttribute("type")->Get<String>();
    if (elem_type != "checkbox") {
      std::wstringstream ss;
      ss << L"UI::changeCheckboxValue() - ERROR: Found an element";
      ss << L" with the ID: " << name.c_str() << L", but it is not a";
      ss << L" checkbox";
      throw wruntime_error(ss.str());
    }
    if (val) {
      if (elem->GetAttribute("checked") == NULL) {
       elem->SetAttribute("checked", "");
      }
    } else {
      if (elem->GetAttribute("checked") != NULL) {
        elem->RemoveAttribute("checked");
      }
    }
  } 

  void UI::loadRendererElements() const {
    Rocket::Core::ElementDocument* d = renderer_doc_;
    Rocket::Core::Element* c = renderer_content_;

    setDocumentTitle(d, "Renderer Settings");
    addCloseButton(d, hideRendererDoc, "hide_render_doc_button");

    // Create dynamic elements here:
    addHeadingText("Window Settings:", d, c);
    addCheckbox("fullscreen", "Fullscreen (Cn+Sh+1)", d, c);
    addSelectbox("screen_resolution", "Resolution", res_combobox_vals_, 
      num_res_combobox_vals_, d, c);
    addCheckbox("double_buffering", "Double Buffer", d, c);

    addHeadingText("Renderer Settings:", d, c);
    addCheckbox("vsm_on", "Shadows", d, c);
    addCheckbox("ssao_on", "SSAO", d, c);
    addCheckbox("dof_on", "Depth of Field", d, c);
    addCheckbox("motion_blur_on", "Motion Blur", d, c);
    addCheckbox("tess_on", "Tessellation", d, c);
    addSelectbox("aa_type_enum", "Anti-Aliasing", aa_combobox_vals_, 
      num_aa_combobox_vals_, d, c);
    addSelectbox("render_output_enum", "Output", render_output_combobox_vals_, 
      num_render_output_combobox_vals_, d, c);
    addButton("reload_shader_button", "Reload Renderer", 
      Renderer::requestReloadRenderer, d, c);
    addCheckbox("render_wireframe", "Wireframe", d, c);
    addCheckbox("visualize_normals", "Normals", d, c);
    addCheckbox("render_aabboxes", "AABBoxes", d, c);
    addCheckbox("directional_lights", "Light: Directional", d, c);
    addCheckbox("spot_lights", "Light: Spot", d, c);
    addCheckbox("point_lights", "Light: Point", d, c);
    addCheckbox("light_pass_stencil_opt", "Light: Stencil Opt.", d, c);
    addCheckbox("render_light_volumes", "Render Light Volumes", d, c);
    addCheckbox("render_light_sources", "Render Light Sources", d, c);
    addSelectbox("vsm_resolution_enum", "SM Res", 
      sm_res_combobox_vals_, num_sm_res_combobox_vals_, d, c);
    addCheckbox("vsm_visualize_split", "Visualize SM Split", d, c);
    addCheckbox("vsm_soft_shadows", "Soft Shadows", d, c);
    addCheckbox("ssao_blur_on", "SSAO Blur", d, c);
    addCheckbox("texture_filtering_on", "Texture Filtering", d, c);
    addCheckbox("flashlight_on", "Flashlight (Cn+Sh+F)", d, c);
    addSelectbox("tess_factor", "Tess Factor", tess_combobox_vals_, 
      num_tess_combobox_vals_, d, c);
    addCheckbox("motion_blur_hq_boned", "High Quality Blur for Boned Meshes", 
      d, c);

    addHeadingText("UI Settings:", d, c);
    addCheckbox("render_ui_fps", "Render FPS", d, c);
    addCheckbox("ui_render_crosshairs", "Render Crosshairs", d, c);
  }

  void UI::loadAppElements() const {
    setDocumentTitle(app_doc_, "Settings");
    addCloseButton(app_doc_, hideAppDoc, "hide_app_doc_button");
    addButton("show_renderer_doc_button", "Renderer Settings", 
      UI::showRendererDoc, app_doc_, app_content_);
  }

  void UI::showRendererDoc() {
    UI* ui = Renderer::g_renderer()->ui();
    ui->setVisibility(true, ui->renderer_doc_);
    SET_SETTING("render_ui_renderer", bool, true);
  }

  void UI::hideRendererDoc() {
    UI* ui = Renderer::g_renderer()->ui();
    ui->setVisibility(false, ui->renderer_doc_);
    SET_SETTING("render_ui_renderer", bool, false);
  }

  void UI::showAppDoc() {
    UI* ui = Renderer::g_renderer()->ui();
    ui->setVisibility(true, ui->app_doc_);
    SET_SETTING("render_ui_app", bool, true);
  }

  void UI::hideAppDoc() {
    UI* ui = Renderer::g_renderer()->ui();
    ui->setVisibility(false, ui->app_doc_);
    SET_SETTING("render_ui_app", bool, false);
  }

}  // namespace ui
}  // namespace jtil

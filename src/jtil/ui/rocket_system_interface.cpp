#include <sstream>
#include <iostream>
#include <Rocket/Core.h>
#include "jtil/ui/rocket_system_interface.h"
#include "jtil/windowing/keys_and_buttons.h"
#include "jtil/windowing/window_interface.h"
#include "jtil/renderer/renderer.h"

namespace jtil {
namespace ui {

  int RocketSystemInterface::GetKeyModifiers(
    const windowing::WindowInterface* wnd) 
    const {
    int Modifiers = 0;

    if(wnd->getKeyState(KEY_LSHIFT) || wnd->getKeyState(KEY_RSHIFT))
      Modifiers |= Rocket::Core::Input::KM_SHIFT;

    if(wnd->getKeyState(KEY_LCTRL) || wnd->getKeyState(KEY_RCTRL))
      Modifiers |= Rocket::Core::Input::KM_CTRL;

    if(wnd->getKeyState(KEY_LALT) || wnd->getKeyState(KEY_RALT))
      Modifiers |= Rocket::Core::Input::KM_ALT;

    return Modifiers;
  };

  Rocket::Core::Input::KeyIdentifier RocketSystemInterface::TranslateKey(
    const int key_glfw) const {
    switch(key_glfw)
    {
    case 'A':
      return Rocket::Core::Input::KI_A;
      break;
    case 'B':
      return Rocket::Core::Input::KI_B;
      break;
    case 'C':
      return Rocket::Core::Input::KI_C;
      break;
    case 'D':
      return Rocket::Core::Input::KI_D;
      break;
    case 'E':
      return Rocket::Core::Input::KI_E;
      break;
    case 'F':
      return Rocket::Core::Input::KI_F;
      break;
    case 'G':
      return Rocket::Core::Input::KI_G;
      break;
    case 'H':
      return Rocket::Core::Input::KI_H;
      break;
    case 'I':
      return Rocket::Core::Input::KI_I;
      break;
    case 'J':
      return Rocket::Core::Input::KI_J;
      break;
    case 'K':
      return Rocket::Core::Input::KI_K;
      break;
    case 'L':
      return Rocket::Core::Input::KI_L;
      break;
    case 'M':
      return Rocket::Core::Input::KI_M;
      break;
    case 'N':
      return Rocket::Core::Input::KI_N;
      break;
    case 'O':
      return Rocket::Core::Input::KI_O;
      break;
    case 'P':
      return Rocket::Core::Input::KI_P;
      break;
    case 'Q':
      return Rocket::Core::Input::KI_Q;
      break;
    case 'R':
      return Rocket::Core::Input::KI_R;
      break;
    case 'S':
      return Rocket::Core::Input::KI_S;
      break;
    case 'T':
      return Rocket::Core::Input::KI_T;
      break;
    case 'U':
      return Rocket::Core::Input::KI_U;
      break;
    case 'V':
      return Rocket::Core::Input::KI_V;
      break;
    case 'W':
      return Rocket::Core::Input::KI_W;
      break;
    case 'X':
      return Rocket::Core::Input::KI_X;
      break;
    case 'Y':
      return Rocket::Core::Input::KI_Y;
      break;
    case 'Z':
      return Rocket::Core::Input::KI_Z;
      break;
    case '0':
      return Rocket::Core::Input::KI_0;
      break;
    case '1':
      return Rocket::Core::Input::KI_1;
      break;
    case '2':
      return Rocket::Core::Input::KI_2;
      break;
    case '3':
      return Rocket::Core::Input::KI_3;
      break;
    case '4':
      return Rocket::Core::Input::KI_4;
      break;
    case '5':
      return Rocket::Core::Input::KI_5;
      break;
    case '6':
      return Rocket::Core::Input::KI_6;
      break;
    case '7':
      return Rocket::Core::Input::KI_7;
      break;
    case '8':
      return Rocket::Core::Input::KI_8;
      break;
    case '9':
      return Rocket::Core::Input::KI_9;
      break;
    case KEY_KP_0:
      return Rocket::Core::Input::KI_NUMPAD0;
      break;
    case KEY_KP_1:
      return Rocket::Core::Input::KI_NUMPAD1;
      break;
    case KEY_KP_2:
      return Rocket::Core::Input::KI_NUMPAD2;
      break;
    case KEY_KP_3:
      return Rocket::Core::Input::KI_NUMPAD3;
      break;
    case KEY_KP_4:
      return Rocket::Core::Input::KI_NUMPAD4;
      break;
    case KEY_KP_5:
      return Rocket::Core::Input::KI_NUMPAD5;
      break;
    case KEY_KP_6:
      return Rocket::Core::Input::KI_NUMPAD6;
      break;
    case KEY_KP_7:
      return Rocket::Core::Input::KI_NUMPAD7;
      break;
    case KEY_KP_8:
      return Rocket::Core::Input::KI_NUMPAD8;
      break;
    case KEY_KP_9:
      return Rocket::Core::Input::KI_NUMPAD9;
      break;
    case KEY_LEFT:
      return Rocket::Core::Input::KI_LEFT;
      break;
    case KEY_RIGHT:
      return Rocket::Core::Input::KI_RIGHT;
      break;
    case KEY_UP:
      return Rocket::Core::Input::KI_UP;
      break;
    case KEY_DOWN:
      return Rocket::Core::Input::KI_DOWN;
      break;
    case KEY_KP_ADD:
      return Rocket::Core::Input::KI_ADD;
      break;
    case KEY_BACKSPACE:
      return Rocket::Core::Input::KI_BACK;
      break;
    case KEY_DEL:
      return Rocket::Core::Input::KI_DELETE;
      break;
    case KEY_KP_DIVIDE:
      return Rocket::Core::Input::KI_DIVIDE;
      break;
    case KEY_END:
      return Rocket::Core::Input::KI_END;
      break;
    case KEY_ESC:
      return Rocket::Core::Input::KI_ESCAPE;
      break;
    case KEY_F1:
      return Rocket::Core::Input::KI_F1;
      break;
    case KEY_F2:
      return Rocket::Core::Input::KI_F2;
      break;
    case KEY_F3:
      return Rocket::Core::Input::KI_F3;
      break;
    case KEY_F4:
      return Rocket::Core::Input::KI_F4;
      break;
    case KEY_F5:
      return Rocket::Core::Input::KI_F5;
      break;
    case KEY_F6:
      return Rocket::Core::Input::KI_F6;
      break;
    case KEY_F7:
      return Rocket::Core::Input::KI_F7;
      break;
    case KEY_F8:
      return Rocket::Core::Input::KI_F8;
      break;
    case KEY_F9:
      return Rocket::Core::Input::KI_F9;
      break;
    case KEY_F10:
      return Rocket::Core::Input::KI_F10;
      break;
    case KEY_F11:
      return Rocket::Core::Input::KI_F11;
      break;
    case KEY_F12:
      return Rocket::Core::Input::KI_F12;
      break;
    case KEY_F13:
      return Rocket::Core::Input::KI_F13;
      break;
    case KEY_F14:
      return Rocket::Core::Input::KI_F14;
      break;
    case KEY_F15:
      return Rocket::Core::Input::KI_F15;
      break;
    case KEY_HOME:
      return Rocket::Core::Input::KI_HOME;
      break;
    case KEY_INSERT:
      return Rocket::Core::Input::KI_INSERT;
      break;
    case KEY_LCTRL:
      return Rocket::Core::Input::KI_LCONTROL;
      break;
    case KEY_LSHIFT:
      return Rocket::Core::Input::KI_LSHIFT;
      break;
    case KEY_KP_MULTIPLY:
      return Rocket::Core::Input::KI_MULTIPLY;
      break;
    case KEY_PAUSE:
      return Rocket::Core::Input::KI_PAUSE;
      break;
    case KEY_RCTRL:
      return Rocket::Core::Input::KI_RCONTROL;
      break;
    case KEY_ENTER:
      return Rocket::Core::Input::KI_RETURN;
      break;
    case KEY_RSHIFT:
      return Rocket::Core::Input::KI_RSHIFT;
      break;
    case KEY_SPACE:
      return Rocket::Core::Input::KI_SPACE;
      break;
    case KEY_KP_SUBTRACT:
      return Rocket::Core::Input::KI_SUBTRACT;
      break;
    case KEY_TAB:
      return Rocket::Core::Input::KI_TAB;
      break;
    };

    return Rocket::Core::Input::KI_UNKNOWN;
  };

  float RocketSystemInterface::GetElapsedTime() {
    return static_cast<float>(renderer::Renderer::getTime());
  };

  bool RocketSystemInterface::LogMessage(Rocket::Core::Log::Type type, 
    const Rocket::Core::String& message) {
    std::string Type;

    switch(type)
    {
    case Rocket::Core::Log::LT_ALWAYS:
      Type = "[Always]";
      break;
    case Rocket::Core::Log::LT_ERROR:
      Type = "[Error]";
      break;
    case Rocket::Core::Log::LT_ASSERT:
      Type = "[Assert]";
      break;
    case Rocket::Core::Log::LT_WARNING:
      Type = "[Warning]";
      break;
    case Rocket::Core::Log::LT_INFO:
      Type = "[Info]";
      break;
    case Rocket::Core::Log::LT_DEBUG:
      Type = "[Debug]";
      break;
    default:
      Type = "[undefined]";
      break;
    };

    std::cout << "RocketSystemInterface::LogMessage:" << Type.c_str() << " - ";
    std::cout << message.CString() << std::endl;
    // printf("%s - %s\n", Type.c_str(), message.CString());

    return true;
  };

}  // namespace ui
}  // namespace jtil

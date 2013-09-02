//
//  rocket_system_interface_glfw.h
//
//  Origionally was an interface for SFML, but it was ported to prenderer and
//  heavily modified to support GLFW instead.
//
//  Created by Jonathan Tompson on 6/12/12.
//

//  ORIGINAL COPYWRITE NOTICE FROM THE LIBROCKET SAMPLE:
/*
* This source file is part of libRocket, the HTML/CSS Interface Middleware
*
*
* For the latest information, see http://www.librocket.com
*
* Copyright (c) 2008-2010 Nuno Silva
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
*/

#pragma once

#include <Rocket/Core/SystemInterface.h>
#include <Rocket/Core/Input.h>

namespace jtil {
namespace windowing { class WindowInterface; }

namespace ui {

  class RocketSystemInterface : public Rocket::Core::SystemInterface
  {
  public:
    RocketSystemInterface() { }
    virtual ~RocketSystemInterface() { }

    Rocket::Core::Input::KeyIdentifier TranslateKey(const int key_glfw) const;
    int GetKeyModifiers(const windowing::WindowInterface* wnd)  const;
    virtual float GetElapsedTime();
    bool LogMessage(Rocket::Core::Log::Type type, 
      const Rocket::Core::String& message);

  private:
    // Non-copyable, non-assignable.
    RocketSystemInterface(RocketSystemInterface&);
    RocketSystemInterface& operator=(const RocketSystemInterface&);
  };

};  // namespace ui
};  // namespace jtil

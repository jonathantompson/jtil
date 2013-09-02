//
//  skybox.h
//
//  Created by Jonathan Tompson on 6/8/12.
//

#ifndef RENDERER_SKYBOX_HEADER
#define RENDERER_SKYBOX_HEADER

namespace renderer {

  class Skybox {
  public:
    // Constructor / Destructor
    Skybox();
    ~Skybox();

  private:

    // Non-copyable, non-assignable.
    Skybox(Skybox&);
    Skybox& operator=(const Skybox&);
  };
};  // unnamed renderer

#endif  // RENDERER_SKYBOX_HEADER

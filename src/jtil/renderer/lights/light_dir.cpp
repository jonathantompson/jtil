#include "jtil/renderer/lights/light_dir.h"
#include "jtil/renderer/shader/shader_program.h"
#include "jtil/renderer/camera/camera.h"
#include "jtil/renderer/renderer.h"
#include "jtil/math/math_types.h"
#include "jtil/renderer/objects/aabbox.h"

namespace jtil {

using math::Float3;
using math::Float4x4;

namespace renderer {

  LightDirData::LightDirData(){
    diffuse_color.set(1, 1, 1);
    diffuse_intensity = 1.0f;
    spec_intensity = 0.2f;
  }

  LightDir::LightDir() : Light() {
    dir_world_.set(0, -1, 0);
    update();
  }

  LightDir::~LightDir() {
  }

  void LightDir::update() {
    // Direction needs to be in view space
    const Float4x4* view = &Renderer::g_renderer()->camera()->view();
    Float3::affineTransformVec(light_data_.dir_view, *view, dir_world_);
    light_data_.dir_view.normalize();  // Just in case
  }

  void LightDir::setHandles() const {
    BIND_UNIFORM("f_light.diffuse_color", light_data_.diffuse_color.m);
    BIND_UNIFORM("f_light.dir_view", light_data_.dir_view.m);
    BIND_UNIFORM("f_light.diffuse_intensity", 
      &light_data_.diffuse_intensity);
    BIND_UNIFORM("f_light.spec_intensity", &light_data_.spec_intensity);
  }

}  // namespace renderer
}  // namespace jtil 
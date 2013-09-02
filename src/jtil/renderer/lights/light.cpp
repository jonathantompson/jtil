#include "jtil/renderer/lights/light.h"
#include "jtil/renderer/objects/aabbox.h"

namespace jtil {
namespace renderer {

  Light::Light() {
    aabbox_ = NULL;
    on_ = true;
  }

  Light::~Light() {

  }

}  // namespace renderer
}  // namespace jtil

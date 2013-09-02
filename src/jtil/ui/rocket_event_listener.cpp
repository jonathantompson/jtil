#include "jtil/ui/rocket_event_listener.h"
#include "jtil/ui/ui.h"

using Rocket::Core::String;

namespace jtil {
namespace ui {

  RocketEventListener::RocketEventListener(UI* ui) {
    ui_ = ui;
  }

  RocketEventListener::~RocketEventListener() {
    // Nothing to do
  }

  // Instances a new event handle for Invaders.
  void RocketEventListener::ProcessEvent(Rocket::Core::Event& event) {
    ui_->processEvent(event);
  }

}  // namespace ui
}  // namespace jtil

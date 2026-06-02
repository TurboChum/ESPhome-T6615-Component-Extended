#include "idle_mode_switch.h"

namespace esphome::t6615 {

void IdleModeSwitch::write_state(bool state) {
  this->publish_state(state);
  this->parent_->queue_idle_mode(state);
}

}  // namespace esphome::t6615

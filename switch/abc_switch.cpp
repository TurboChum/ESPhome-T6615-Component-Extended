#include "abc_switch.h"

namespace esphome::t6615 {

void AbcSwitch::write_state(bool state) {
  // Optimistic publish; parent will confirm state from sensor ACK
  this->publish_state(state);
  this->parent_->queue_abc_logic(state);
}

}  // namespace esphome::t6615

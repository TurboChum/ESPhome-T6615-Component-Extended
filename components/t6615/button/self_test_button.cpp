#include "self_test_button.h"

namespace esphome::t6615 {

void SelfTestButton::press_action() { this->parent_->queue_self_test(); }

}  // namespace esphome::t6615

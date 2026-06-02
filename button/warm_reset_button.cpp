#include "warm_reset_button.h"

namespace esphome::t6615 {

void WarmResetButton::press_action() { this->parent_->queue_warm_reset(); }

}  // namespace esphome::t6615

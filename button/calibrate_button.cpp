#include "calibrate_button.h"

namespace esphome::t6615 {

void CalibrateButton::press_action() { this->parent_->queue_calibration(); }

}  // namespace esphome::t6615

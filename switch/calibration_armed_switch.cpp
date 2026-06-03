#include "calibration_armed_switch.h"

namespace esphome::t6615 {

void CalibrationArmedSwitch::write_state(bool state) {
  this->publish_state(state);
  this->parent_->set_cal_armed(state);
}

}  // namespace esphome::t6615

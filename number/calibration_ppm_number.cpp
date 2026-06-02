#include "calibration_ppm_number.h"

namespace esphome::t6615 {

void CalibrationPpmNumber::control(float value) {
  this->publish_state(value);
  this->parent_->queue_set_cal_ppm_target(static_cast<uint16_t>(value));
}

}  // namespace esphome::t6615

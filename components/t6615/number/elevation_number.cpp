#include "elevation_number.h"

namespace esphome::t6615 {

void ElevationNumber::control(float value) {
  this->publish_state(value);
  this->parent_->queue_set_elevation(static_cast<uint16_t>(value));
}

}  // namespace esphome::t6615

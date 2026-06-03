#pragma once

#include "esphome/components/switch/switch.h"
#include "../t6615.h"

namespace esphome::t6615 {

class CalibrationArmedSwitch : public switch_::Switch, public Parented<T6615Component> {
 public:
  CalibrationArmedSwitch() = default;

 protected:
  void write_state(bool state) override;
};

}  // namespace esphome::t6615

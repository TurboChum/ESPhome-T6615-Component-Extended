#pragma once

#include "esphome/components/switch/switch.h"
#include "../t6615.h"

namespace esphome::t6615 {

// ABC (Automatic Baseline Correction) logic switch.
// Meaningful only on the T6613 (unreferenced single-beam sensor).
// The T6615 has a sealed reference channel and does not use ABC;
// this switch is included for T6613 compatibility and ESPHome PR parity.
class AbcSwitch : public switch_::Switch, public Parented<T6615Component> {
 public:
  AbcSwitch() = default;

 protected:
  void write_state(bool state) override;
};

}  // namespace esphome::t6615

#pragma once

#include "esphome/components/number/number.h"
#include "../t6615.h"

namespace esphome::t6615 {

class CalibrationPpmNumber : public number::Number, public Parented<T6615Component> {
 public:
  CalibrationPpmNumber() = default;

 protected:
  void control(float value) override;
};

}  // namespace esphome::t6615

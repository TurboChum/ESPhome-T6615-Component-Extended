#pragma once

#include "esphome/components/number/number.h"
#include "../t6615.h"

namespace esphome::t6615 {

class ElevationNumber : public number::Number, public Parented<T6615Component> {
 public:
  ElevationNumber() = default;

 protected:
  void control(float value) override;
};

}  // namespace esphome::t6615

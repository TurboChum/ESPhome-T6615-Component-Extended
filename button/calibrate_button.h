#pragma once

#include "esphome/components/button/button.h"
#include "../t6615.h"

namespace esphome::t6615 {

class CalibrateButton : public button::Button, public Parented<T6615Component> {
 public:
  CalibrateButton() = default;

 protected:
  void press_action() override;
};

}  // namespace esphome::t6615

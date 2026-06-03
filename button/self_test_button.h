#pragma once

#include "esphome/components/button/button.h"
#include "../t6615.h"

namespace esphome::t6615 {

class SelfTestButton : public button::Button, public Parented<T6615Component> {
 public:
  SelfTestButton() = default;

 protected:
  void press_action() override;
};

}  // namespace esphome::t6615

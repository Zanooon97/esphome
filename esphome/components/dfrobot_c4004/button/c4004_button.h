#pragma once

#include "../dfrobot_c4004.h"
#include "esphome/components/button/button.h"

namespace esphome {
namespace dfrobot_c4004 {

class C4004FactoryResetButton : public button::Button, public Parented<C4004Component> {
 protected:
  void press_action() override;
};

class C4004ResetButton : public button::Button, public Parented<C4004Component> {
 protected:
  void press_action() override;
};

class C4004SetInstallInfoButton : public button::Button, public Parented<C4004Component> {
 protected:
  void press_action() override;
};

class C4004SetFourSidedRangeModeButton : public button::Button, public Parented<C4004Component> {
 protected:
  void press_action() override;
};

class C4004TrajectoryRangeModeButton : public button::Button, public Parented<C4004Component> {
 protected:
  void press_action() override;
};

class C4004ClearLiveCountButton : public button::Button, public Parented<C4004Component> {
 protected:
  void press_action() override;
};

}  // namespace dfrobot_c4004
}  // namespace esphome

#pragma once

#include "../dfrobot_c4004.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
namespace dfrobot_c4004 {

class C4004PresenceEnableSwitch : public switch_::Switch, public Parented<C4004Component> {
 protected:
  void write_state(bool state) override;
};

class C4004TrajectoryTrackEnableSwitch : public switch_::Switch, public Parented<C4004Component> {
 protected:
  void write_state(bool state) override;
};

class C4004TrajectoryRangeModeSwitch : public switch_::Switch, public Parented<C4004Component> {
 protected:
  void write_state(bool state) override;
};

class C4004TrkLEDSwitch : public switch_::Switch, public Parented<C4004Component> {
 protected:
  void write_state(bool state) override;
};

class C4004OccLEDSwitch : public switch_::Switch, public Parented<C4004Component> {
 protected:
  void write_state(bool state) override;
};

}  // namespace dfrobot_c4004
}  // namespace esphome

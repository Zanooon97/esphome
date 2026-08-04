#pragma once

#include "../dfrobot_c4004.h"
#include "esphome/components/number/number.h"

namespace esphome {
namespace dfrobot_c4004 {

class C4004InstallHeightNumber : public number::Number, public Parented<C4004Component> {
 protected:
  void control(float value) override;
};

class C4004InstallZAngleNumber : public number::Number, public Parented<C4004Component> {
 protected:
  void control(float value) override;
};

class C4004RangeXMaxNumber : public number::Number, public Parented<C4004Component> {
 protected:
  void control(float value) override;
};

class C4004RangeXMinNumber : public number::Number, public Parented<C4004Component> {
 protected:
  void control(float value) override;
};

class C4004RangeYMaxNumber : public number::Number, public Parented<C4004Component> {
 protected:
  void control(float value) override;
};

class C4004RangeYMinNumber : public number::Number, public Parented<C4004Component> {
 protected:
  void control(float value) override;
};

class C4004RealTimeReportIntervalNumber : public number::Number, public Parented<C4004Component> {
 protected:
  void control(float value) override;
};

class C4004TrajectoryGenerationDistanceNumber : public number::Number, public Parented<C4004Component> {
 protected:
  void control(float value) override;
};

class C4004TrajectoryLifetimeNumber : public number::Number, public Parented<C4004Component> {
 protected:
  void control(float value) override;
};

class C4004UnoccupiedTimeNumber : public number::Number, public Parented<C4004Component> {
 protected:
  void control(float value) override;
};

class C4004FrameGenerateCountNumber : public number::Number, public Parented<C4004Component> {
 protected:
  void control(float value) override;
};

class C4004ZoneMcuIoNumber : public number::Number, public Parented<C4004Component> {
 public:
  void set_zone_index(uint8_t index) { this->zone_index_ = index; }

 protected:
  void control(float value) override;

  uint8_t zone_index_{0};
};

}  // namespace dfrobot_c4004
}  // namespace esphome

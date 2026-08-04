#include "c4004_switch.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dfrobot_c4004 {

static const char *const TAG = "dfrobot_c4004.switch";

void C4004PresenceEnableSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  const bool previous_state = this->state;
  if (this->parent_->set_presence_enable(state)) {
    this->publish_state(state);
  } else {
    ESP_LOGW(TAG, "Failed to set presence enable");
    this->publish_state(previous_state);
  }
}

void C4004TrajectoryTrackEnableSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  const bool previous_state = this->state;
  if (this->parent_->set_trajectory_track_enable(state)) {
    this->publish_state(state);
  } else {
    ESP_LOGW(TAG, "Failed to set trajectory track enable");
    this->publish_state(previous_state);
  }
}

void C4004TrajectoryRangeModeSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  const bool previous_state = this->state;
  if (this->parent_->set_trajectory_range_mode(state)) {
    this->publish_state(this->parent_->is_trajectory_range_mode());
  } else {
    ESP_LOGW(TAG, "Failed to set trajectory range mode");
    this->publish_state(previous_state);
  }
}

void C4004TrkLEDSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  const bool previous_state = this->state;
  if (this->parent_->set_trk_led(state)) {
    this->publish_state(state);
  } else {
    ESP_LOGW(TAG, "Failed to set tracking LED");
    this->publish_state(previous_state);
  }
}

void C4004OccLEDSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  const bool previous_state = this->state;
  if (this->parent_->set_occ_led(state)) {
    this->publish_state(state);
  } else {
    ESP_LOGW(TAG, "Failed to set occupancy LED");
    this->publish_state(previous_state);
  }
}

}  // namespace dfrobot_c4004
}  // namespace esphome

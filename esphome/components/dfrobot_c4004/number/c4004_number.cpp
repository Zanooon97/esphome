#include "c4004_number.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dfrobot_c4004 {

static const char *const TAG = "dfrobot_c4004.number";

void C4004InstallHeightNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_pending_install_height(value);
  this->publish_state(value);
}

void C4004InstallZAngleNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_pending_install_z_angle(value);
  this->publish_state(value);
}

void C4004RangeXMaxNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_pending_range_x_max(value);
  this->publish_state(value);
}

void C4004RangeXMinNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_pending_range_x_min(value);
  this->publish_state(value);
}

void C4004RangeYMaxNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_pending_range_y_max(value);
  this->publish_state(value);
}

void C4004RangeYMinNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_pending_range_y_min(value);
  this->publish_state(value);
}

void C4004RealTimeReportIntervalNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  if (this->parent_->set_real_time_report_interval(value)) {
    this->publish_state(value);
  } else {
    ESP_LOGW(TAG, "Failed to set real-time report interval");
    this->publish_state(this->parent_->get_real_time_report_interval());
  }
}

void C4004TrajectoryGenerationDistanceNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  if (this->parent_->set_trajectory_generation_distance(value)) {
    this->publish_state(value);
  } else {
    ESP_LOGW(TAG, "Failed to set trajectory generation distance");
    this->publish_state(this->parent_->get_trajectory_generation_distance());
  }
}

void C4004TrajectoryLifetimeNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  if (this->parent_->set_trajectory_lifetime(value)) {
    this->publish_state(value);
  } else {
    ESP_LOGW(TAG, "Failed to set trajectory lifetime");
    this->publish_state(this->parent_->get_trajectory_lifetime());
  }
}

void C4004UnoccupiedTimeNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  if (this->parent_->set_unoccupied_time(value)) {
    this->publish_state(value);
  } else {
    ESP_LOGW(TAG, "Failed to set unoccupied time");
    this->publish_state(this->parent_->get_unoccupied_time());
  }
}

void C4004FrameGenerateCountNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  if (this->parent_->set_frame_generate_count(value)) {
    this->publish_state(value);
  } else {
    ESP_LOGW(TAG, "Failed to set frame generation count");
    this->publish_state(this->parent_->get_frame_generate_count());
  }
}

void C4004ZoneMcuIoNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  if (this->parent_->set_zone_mcu_io(this->zone_index_, value)) {
    this->publish_state(value);
  } else {
    ESP_LOGW(TAG, "Failed to set Zone %u MCU IO", this->zone_index_ + 1);
    this->publish_state(value);
  }
}

}  // namespace dfrobot_c4004
}  // namespace esphome

#include "c4004_button.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dfrobot_c4004 {

static const char *const TAG = "dfrobot_c4004.button";

void C4004FactoryResetButton::press_action() {
  if (this->parent_ != nullptr && !this->parent_->factory_reset()) {
    ESP_LOGW(TAG, "Factory reset command failed");
  }
}

void C4004ResetButton::press_action() {
  if (this->parent_ != nullptr && !this->parent_->reset()) {
    ESP_LOGW(TAG, "Reset command failed");
  }
}

void C4004SetInstallInfoButton::press_action() {
  if (this->parent_ != nullptr && !this->parent_->set_install_info()) {
    ESP_LOGW(TAG, "Set install info command failed");
  }
}

void C4004SetFourSidedRangeModeButton::press_action() {
  if (this->parent_ != nullptr && !this->parent_->set_four_sided_range_mode()) {
    ESP_LOGW(TAG, "Set four-sided range mode command failed");
  }
}

void C4004TrajectoryRangeModeButton::press_action() {
  if (this->parent_ != nullptr && !this->parent_->set_trajectory_range_mode(false)) {
    ESP_LOGW(TAG, "Set trajectory range mode command failed");
  }
}

void C4004ClearLiveCountButton::press_action() {
  if (this->parent_ != nullptr && !this->parent_->clear_live_count()) {
    ESP_LOGW(TAG, "Clear live count command failed");
  }
}

}  // namespace dfrobot_c4004
}  // namespace esphome

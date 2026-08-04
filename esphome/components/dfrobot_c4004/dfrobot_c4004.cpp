#include "dfrobot_c4004.h"
#include "esphome/core/application.h"
#include <limits>

#ifdef USE_ESP32
#include "driver/gpio.h"
#endif

namespace esphome {
namespace dfrobot_c4004 {

static const char *const TAG = "dfrobot_c4004";
static const char *const MQTT_TOPIC_STATE_MULTI_TAG_CONFIG = "state/multi_tag_config";
static const char *const MQTT_TOPIC_STATE_LEARNED_TRAJECTORY_RANGE = "state/learned_trajectory_range";
static const char *const MQTT_TOPIC_STATE_CONFIG_FILE_RANGE = "state/config_file_range";
static const char *const MQTT_TOPIC_STATE_TARGET_TRAJECTORY = "state/target_trajectory";
static const char *const MQTT_TOPIC_STATE_TAG_EVENT = "state/tag_event";
static const char *const MQTT_TOPIC_COMMAND_MULTI_TAG_CONFIG_SET = "command/multi_tag_config/set";
static const char *const MQTT_TOPIC_COMMAND_CONFIG_FILE_RANGE_SET = "command/config_file_range/set";
static const char *const MQTT_TOPIC_COMMAND_LEARNED_TRAJECTORY_RANGE_SET = "command/learned_trajectory_range/set";
static const char *const MQTT_TOPIC_COMMAND_LEARNED_TRAJECTORY_RANGE_QUERY = "command/learned_trajectory_range/query";
static const char *const MQTT_TOPIC_RESULT_MULTI_TAG_CONFIG_SET = "result/multi_tag_config/set";
static const char *const MQTT_TOPIC_RESULT_CONFIG_FILE_RANGE_SET = "result/config_file_range/set";
static const char *const MQTT_TOPIC_RESULT_LEARNED_TRAJECTORY_RANGE_SET = "result/learned_trajectory_range/set";
static const char *const MQTT_TOPIC_RESULT_LEARNED_TRAJECTORY_RANGE_QUERY = "result/learned_trajectory_range/query";
static const uint32_t HEARTBEAT_QUERY_INTERVAL_MS = 20000UL;

static const char *tag_type_to_mqtt_string(TagType tag_type) {
  switch (tag_type) {
    case TAG_BOUNDARY:
      return "boundary";
    case TAG_APPROACH_AWAY:
      return "approach_away";
    case TAG_PEOPLE_COUNTING:
      return "people_counting";
    case TAG_NOISE:
      return "noise";
    case TAG_NONE:
    default:
      return "none";
  }
}

static const char *boundary_state_to_mqtt_string(uint8_t state) {
  if (state == 0) {
    return "enter";
  }
  if (state == 1) {
    return "exit";
  }
  return "none";
}

static const char *approach_away_state_to_mqtt_string(uint8_t state) {
  if (state == 0) {
    return "approach";
  }
  if (state == 1) {
    return "away";
  }
  return "none";
}

static int8_t hex_nibble(char value) {
  if (value >= '0' && value <= '9') {
    return value - '0';
  }
  if (value >= 'A' && value <= 'F') {
    return value - 'A' + 10;
  }
  if (value >= 'a' && value <= 'f') {
    return value - 'a' + 10;
  }
  return -1;
}

static char hex_digit(uint8_t value) {
  return value < 10 ? static_cast<char>('0' + value) : static_cast<char>('A' + value - 10);
}

void C4004Component::setup() {
  this->setup_zone_gpio_pins();
  this->publish_zone_presence_states();
  this->ensure_mqtt_bridge_subscriptions_();

  this->flush_input();
  this->reset_rx_parser_();
  this->connected_ = this->begin();
  ESP_LOGI(TAG, "[online] setup result: connected=%s", TRUEFALSE(this->connected_));
  this->publish_online(this->connected_);

  if (!this->connected_) {
    ESP_LOGW(TAG, "C4004 did not respond during setup");
    this->publish_all_states();
    return;
  }

  ESP_LOGI(TAG, "C4004 setup successful");
  this->sync_device_state();
}

void C4004Component::loop() {
  const uint32_t now = App.get_loop_component_start_time();
  const ReportedEvent event = this->get_reported_event(5);

  this->ensure_mqtt_bridge_subscriptions_();
  this->update_mqtt_bridge_connection_();

  if (now - this->last_zone_gpio_poll_ms_ >= 100UL) {
    this->last_zone_gpio_poll_ms_ = now;
    this->poll_zone_gpio_pins();
  }

  if (event != EVENT_NONE) {
    if (event == EVENT_PRESENCE) {
      this->publish_presence_state();
    } else if (event == EVENT_MOTION) {
      this->publish_motion_state();
    } else if (event == EVENT_TRAJECTORY) {
      this->publish_target_count();
      this->publish_target_trajectory_mqtt_state_();
    } else if (event == EVENT_PEOPLE_COUNT) {
      this->publish_live_count();
    } else if (event == EVENT_TAG && this->tag_info_valid_) {
      this->publish_tag_zone_state(this->tag_info_.tag_index, this->tag_info_.tag_type);
      this->publish_tag_event_mqtt_state_();
    } else if (event == EVENT_HEARTBEAT) {
      const bool previous_connected = this->connected_;
      this->connected_ = true;
      if (this->connected_ != previous_connected) {
        this->publish_online(this->connected_);
      }
    }
  }

  bool active_query_ran = false;
  if (now - this->last_active_query_ms_ >= 500UL) {
    this->last_active_query_ms_ = now;
    active_query_ran = true;
    switch (this->active_query_step_) {
      case 0:
        this->get_presence_state();
        this->publish_presence_state();
        break;
      case 1:
        this->get_motion_state();
        this->publish_motion_state();
        break;
      case 2:
        this->get_live_count(GET_DATA_REPORT);
        this->publish_live_count();
        break;
      default:
        this->get_target_count_active();
        this->publish_target_count();
        break;
    }
    this->active_query_step_ = (this->active_query_step_ + 1) % 4;
  }

  if (!active_query_ran && now - this->last_heartbeat_query_ms_ >= HEARTBEAT_QUERY_INTERVAL_MS) {
    const bool previous_connected = this->connected_;
    this->last_heartbeat_query_ms_ = now;
    this->connected_ = this->is_connected();
    if (this->connected_ != previous_connected) {
      ESP_LOGI(TAG, "[online] poll(20s) state changed: %s -> %s", TRUEFALSE(previous_connected),
               TRUEFALSE(this->connected_));
    }
    if (this->connected_ != previous_connected) {
      this->publish_online(this->connected_);
    }
  }
}

void C4004Component::dump_config() {
  ESP_LOGCONFIG(TAG, "DFRobot C4004");
  ESP_LOGCONFIG(TAG, "  Connected: %s", TRUEFALSE(this->connected_));
  ESP_LOGCONFIG(TAG, "  Install mode: %s", this->install_mode_to_string(this->install_info_.mode));
  ESP_LOGCONFIG(TAG, "  Install height: %u cm", this->install_info_.height_cm);
  ESP_LOGCONFIG(TAG, "  Install Z angle: %d deg", this->install_info_.z_angle);
  ESP_LOGCONFIG(TAG, "  Detection range mode: %s", this->range_mode_to_string(this->range_mode_));
  ESP_LOGCONFIG(TAG, "  Frame generation count: %u", this->frame_generate_count_);
  ESP_LOGCONFIG(TAG, "  MQTT bridge: %s", TRUEFALSE(this->mqtt_bridge_enabled_));
  if (this->mqtt_bridge_enabled_) {
    ESP_LOGCONFIG(TAG, "  MQTT bridge key: %s", this->mqtt_bridge_key_.c_str());
    ESP_LOGCONFIG(TAG, "  MQTT bridge topic prefix: %s", this->mqtt_bridge_topic_prefix_.c_str());
    ESP_LOGCONFIG(TAG, "  MQTT bridge QoS: %u", this->mqtt_bridge_qos_);
    ESP_LOGCONFIG(TAG, "  MQTT bridge stream QoS: %u", this->mqtt_bridge_stream_qos_);
    ESP_LOGCONFIG(TAG, "  MQTT bridge retain: %s", TRUEFALSE(this->mqtt_bridge_retain_state_));
  }
  for (uint8_t i = 0; i < C4004_ZONE_GPIO_COUNT; i++) {
    ESP_LOGCONFIG(TAG, "  Zone %u MCU GPIO: %d", i + 1, this->zone_gpio_pins_[i]);
  }
}

bool C4004Component::begin() {
  const uint32_t start = millis();
  while (millis() - start < 1200UL) {
    if (this->is_init_finished()) {
      return true;
    }
    delay(5);
  }
  return this->is_connected();
}

bool C4004Component::is_init_finished() {
  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  if (this->request_frame(C4004_CTRL_WORK_STATUS, C4004_CMD_WORK_STATUS_INIT_FINISHED_QUERY, &data, 1, &packet)) {
    if (packet.len > 0) {
      this->init_finished_ = packet.data[0] == 0x01;
    }
  }
  return this->init_finished_;
}

bool C4004Component::is_connected() {
  return this->get_heartbeat(GET_DATA_ACTIVE);
}

bool C4004Component::get_heartbeat(GetDataMode mode) {
  if (mode == GET_DATA_REPORT) {
    return this->heartbeat_;
  }

  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  if (!this->request_frame(C4004_CTRL_SYSTEM, C4004_CMD_SYSTEM_HEARTBEAT_QUERY, &data, 1, &packet)) {
    ESP_LOGW(TAG, "[heartbeat] active query failed");
    this->heartbeat_ = false;
    return false;
  }
  if (packet.len > 0 && packet.data[0] != C4004_QUERY_DATA) {
    ESP_LOGW(TAG, "[heartbeat] invalid response: len=%u first=0x%02X expected=0x%02X", packet.len, packet.data[0],
             C4004_QUERY_DATA);
    this->heartbeat_ = false;
    return false;
  }
  this->heartbeat_ = true;
  return true;
}

ReportedEvent C4004Component::get_reported_event(uint16_t timeout_ms) {
  Packet &packet = this->rx_packet_;
  if (!this->read_frame(&packet, timeout_ms)) {
    return EVENT_NONE;
  }
  return this->handle_packet(&packet);
}

void C4004Component::setup_zone_gpio_pins() {
  for (uint8_t i = 0; i < C4004_ZONE_GPIO_COUNT; i++) {
    this->configure_zone_gpio_pin(i);
  }
}

bool C4004Component::configure_zone_gpio_pin(uint8_t index) {
  if (index >= C4004_ZONE_GPIO_COUNT) {
    return false;
  }

  const int16_t pin = this->zone_gpio_pins_[index];
  this->zone_presence_[index] = false;
  if (pin < 0) {
    this->publish_zone_presence_states();
    return true;
  }

#ifdef USE_ESP32
  if (pin >= GPIO_NUM_MAX || pin >= 64) {
    ESP_LOGW(TAG, "Zone %u GPIO %d is outside the ESP32 GPIO range; zone input disabled", index + 1, pin);
    this->publish_zone_presence_states();
    return false;
  }

  gpio_config_t config = {};
  config.pin_bit_mask = 1ULL << static_cast<uint8_t>(pin);
  config.mode = GPIO_MODE_INPUT;
  config.pull_up_en = GPIO_PULLUP_DISABLE;
  config.pull_down_en = GPIO_PULLDOWN_ENABLE;
  config.intr_type = GPIO_INTR_DISABLE;
  const esp_err_t err = gpio_config(&config);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to configure Zone %u GPIO %d: %d", index + 1, pin, static_cast<int>(err));
    this->publish_zone_presence_states();
    return false;
  }
  this->zone_presence_[index] = gpio_get_level(static_cast<gpio_num_t>(pin)) != 0;
  this->publish_zone_presence_states();
  return true;
#else
  ESP_LOGW(TAG, "Zone %u GPIO %d is configured, but MCU GPIO zone inputs currently require ESP32", index + 1, pin);
  this->publish_zone_presence_states();
  return false;
#endif
}

void C4004Component::poll_zone_gpio_pins() {
  bool changed = false;
  for (uint8_t i = 0; i < C4004_ZONE_GPIO_COUNT; i++) {
    const int16_t pin = this->zone_gpio_pins_[i];
    if (pin < 0) {
      continue;
    }
#ifdef USE_ESP32
    if (pin >= GPIO_NUM_MAX || pin >= 64) {
      continue;
    }
    const bool next = gpio_get_level(static_cast<gpio_num_t>(pin)) != 0;
    if (next != this->zone_presence_[i]) {
      this->zone_presence_[i] = next;
      changed = true;
    }
#endif
  }
  if (changed) {
    this->publish_zone_presence_states();
  }
}

void C4004Component::set_zone_gpio_pin(uint8_t index, int16_t pin) {
  if (index >= C4004_ZONE_GPIO_COUNT) {
    return;
  }
  this->zone_gpio_pins_[index] = pin;
}

void C4004Component::set_mqtt_bridge(const std::string &mqtt_key, const std::string &topic_prefix, uint8_t qos,
                                     uint8_t stream_qos, bool retain_state) {
  this->mqtt_bridge_enabled_ = !mqtt_key.empty();
  this->mqtt_bridge_key_ = mqtt_key;
  this->mqtt_bridge_topic_prefix_ = topic_prefix;
  this->mqtt_bridge_qos_ = qos;
  this->mqtt_bridge_stream_qos_ = stream_qos;
  this->mqtt_bridge_retain_state_ = retain_state;
}

bool C4004Component::set_zone_mcu_io(uint8_t index, float value) {
  if (index >= C4004_ZONE_GPIO_COUNT) {
    return false;
  }

  int16_t pin = static_cast<int16_t>(value);
  if (pin < -1) {
    pin = -1;
  } else if (pin > 255) {
    pin = 255;
  }
  this->zone_gpio_pins_[index] = pin;
  this->publish_zone_mcu_io_numbers();
  return this->configure_zone_gpio_pin(index);
}

bool C4004Component::decode_hex_payload(const std::string &value, uint8_t *payload, uint16_t max_payload,
                                        uint16_t *payload_len, std::string *normalized) {
  if (payload == nullptr || payload_len == nullptr || normalized == nullptr) {
    return false;
  }

  *payload_len = 0;
  normalized->clear();
  for (char ch : value) {
    if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
      continue;
    }
    const int8_t nibble = hex_nibble(ch);
    if (nibble < 0) {
      return false;
    }
    normalized->push_back(nibble < 10 ? static_cast<char>('0' + nibble) : static_cast<char>('A' + nibble - 10));
  }

  if ((normalized->size() % 2) != 0) {
    return false;
  }
  const uint16_t len = static_cast<uint16_t>(normalized->size() / 2);
  if (len > max_payload) {
    return false;
  }

  for (uint16_t i = 0; i < len; i++) {
    const int8_t high = hex_nibble((*normalized)[i * 2]);
    const int8_t low = hex_nibble((*normalized)[i * 2 + 1]);
    payload[i] = static_cast<uint8_t>((high << 4) | low);
  }
  *payload_len = len;
  return true;
}

bool C4004Component::set_multi_tag_config_hex(const std::string &value, std::string *normalized) {
  uint8_t payload[C4004_TAG_MULTI_CONFIG_MAX_PAYLOAD];
  uint16_t payload_len = 0;
  Packet packet;

  if (!this->decode_hex_payload(value, payload, sizeof(payload), &payload_len, normalized)) {
    return false;
  }
  if (payload_len < 2) {
    return false;
  }

  const uint16_t count = this->read_uint16(payload);
  if (count > C4004_TAG_CONFIG_LIMIT) {
    return false;
  }
  if (payload_len != static_cast<uint16_t>(2 + count * C4004_TAG_CONFIG_RECORD_LEN)) {
    return false;
  }

  for (uint16_t i = 0; i < count; i++) {
    const uint16_t offset = 2 + i * C4004_TAG_CONFIG_RECORD_LEN;
    const uint8_t io_index = payload[offset + 3];
    const uint8_t tag_type = payload[offset + 1];
    const uint8_t scope_type = payload[offset + 2];
    if (tag_type > TAG_NOISE || scope_type > RECTANGLE) {
      return false;
    }
    if (io_index != 0 && (io_index < 2 || io_index > 6)) {
      return false;
    }
  }

  if (!this->request_frame(C4004_CTRL_DETECTION_RANGE, C4004_CMD_DETECTION_RANGE_SET_TAGS_FROM_CONFIG, payload, payload_len,
                           &packet)) {
    return false;
  }
  this->multi_tag_config_hex_ = *normalized;
  this->multi_tag_config_count_ = count;
  this->sync_tag_zone_cache();
  this->publish_tag_zone_states();
  this->publish_multi_tag_config_mqtt_state_();
  return true;
}

bool C4004Component::set_config_file_range_hex(const std::string &value, std::string *normalized) {
  uint8_t payload[C4004_MAX_PAYLOAD];
  uint16_t payload_len = 0;
  Packet packet;
  uint16_t point_count = 0;

  if (!this->decode_hex_payload(value, payload, sizeof(payload), &payload_len, normalized)) {
    return false;
  }
  if (payload_len < 3) {
    return false;
  }
  if (static_cast<DetectionRangeMode>(payload[0]) != RANGE_CONFIG_FILE) {
    return false;
  }
  point_count = this->read_uint16(&payload[1]);
  if (point_count > C4004_MAX_POINTS) {
    return false;
  }
  if (payload_len != static_cast<uint16_t>(3 + point_count * 4)) {
    return false;
  }
  if (!this->validate_range_points_payload(payload, payload_len, RANGE_CONFIG_FILE, normalized, &point_count)) {
    return false;
  }
  if (!this->request_frame(C4004_CTRL_DETECTION_RANGE, C4004_CMD_DETECTION_RANGE_SET_RANGE, payload, payload_len, &packet,
                           C4004_SET_RANGE_TIMEOUT_MS)) {
    return false;
  }

  this->config_file_range_hex_ = *normalized;
  this->config_file_range_point_count_ = point_count;
  this->range_mode_ = RANGE_CONFIG_FILE;
  this->trajectory_range_mode_ = false;
  this->publish_config_file_range_mqtt_state_();
  this->publish_detection_range_mode();
  this->publish_switch_states();
  return true;
}

bool C4004Component::query_learned_trajectory_range() {
  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  bool learning_enabled = false;
  if (!this->request_frame(C4004_CTRL_DETECTION_RANGE, C4004_CMD_DETECTION_RANGE_QUERY_RANGE, &data, 1, &packet,
                           C4004_TRAJECTORY_RANGE_TIMEOUT_MS)) {
    return false;
  }
  if (packet.len < 1 || static_cast<DetectionRangeMode>(packet.data[0]) != RANGE_TRAJECTORY) {
    this->publish_detection_range_mode();
    this->publish_switch_states();
    return false;
  }
  if (!this->validate_trajectory_range_payload(packet.data, packet.len, &this->learned_trajectory_range_hex_,
                                               &learning_enabled, &this->learned_trajectory_range_point_count_)) {
    return false;
  }

  this->learned_trajectory_learning_enabled_ = learning_enabled;
  this->publish_learned_trajectory_range_mqtt_state_();
  this->publish_detection_range_mode();
  this->publish_switch_states();
  return true;
}

bool C4004Component::query_config_file_range() {
  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  if (!this->request_frame(C4004_CTRL_DETECTION_RANGE, C4004_CMD_DETECTION_RANGE_QUERY_RANGE, &data, 1, &packet)) {
    return false;
  }
  if (packet.len < 1 || static_cast<DetectionRangeMode>(packet.data[0]) != RANGE_CONFIG_FILE) {
    this->publish_detection_range_mode();
    this->publish_switch_states();
    return false;
  }
  if (!this->cache_range_points_payload(packet.data, packet.len, RANGE_CONFIG_FILE, &this->config_file_range_hex_,
                                        nullptr, &this->config_file_range_point_count_)) {
    return false;
  }

  this->publish_config_file_range_mqtt_state_();
  this->publish_detection_range_mode();
  this->publish_switch_states();
  return true;
}

bool C4004Component::reset() {
  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  return this->request_frame(C4004_CTRL_SYSTEM, C4004_CMD_SYSTEM_RESET, &data, 1, &packet, C4004_RESET_TIMEOUT_MS);
}

bool C4004Component::factory_reset() {
  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  return this->request_frame(C4004_CTRL_SYSTEM, C4004_CMD_SYSTEM_FACTORY_RESET, &data, 1, &packet, C4004_FACTORY_RESET_TIMEOUT_MS);
}

bool C4004Component::set_install_info() {
  const bool ok = this->set_install_info(this->install_info_);
  if (ok) {
    this->publish_install_info();
  }
  return ok;
}

bool C4004Component::set_four_sided_range_mode() {
  this->range_mode_ = RANGE_FOUR_SIDE;
  const bool ok = [&]() {
    uint8_t data[9];
    Packet packet;
    data[0] = RANGE_FOUR_SIDE;
    this->write_sign_bit_int16(&data[1], this->range_info_.x_max);
    this->write_sign_bit_int16(&data[3], this->range_info_.x_min);
    this->write_sign_bit_int16(&data[5], this->range_info_.y_max);
    this->write_sign_bit_int16(&data[7], this->range_info_.y_min);
    return this->request_frame(C4004_CTRL_DETECTION_RANGE, C4004_CMD_DETECTION_RANGE_SET_RANGE, data, sizeof(data), &packet,
                               C4004_SET_RANGE_TIMEOUT_MS);
  }();

  if (ok) {
    this->range_mode_ = RANGE_FOUR_SIDE;
    this->trajectory_range_mode_ = false;
    this->publish_boundary_range();
    this->publish_switch_states();
    this->publish_detection_range_mode();
  }
  return ok;
}

bool C4004Component::verify_trajectory_range_mode_state(bool learning) {
  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  if (!this->request_frame(C4004_CTRL_DETECTION_RANGE, C4004_CMD_DETECTION_RANGE_QUERY_RANGE, &data, 1, &packet,
                           C4004_DEFAULT_TIMEOUT_MS)) {
    return false;
  }

  std::string normalized;
  bool learning_enabled = false;
  if (!this->validate_trajectory_range_payload(packet.data, packet.len, &normalized, &learning_enabled)) {
    return false;
  }
  return learning ? learning_enabled : !learning_enabled;
}

bool C4004Component::set_trajectory_range_mode(bool learning) {
  uint8_t data[2];
  Packet packet;

  data[0] = RANGE_TRAJECTORY;
  data[1] = learning ? 1 : 0;

  if (!this->send_command(C4004_CTRL_DETECTION_RANGE, C4004_CMD_DETECTION_RANGE_SET_RANGE, data, sizeof(data))) {
    return false;
  }

  const uint32_t start = millis();
  while (millis() - start < C4004_TRAJECTORY_RANGE_TIMEOUT_MS) {
    const uint16_t elapsed = static_cast<uint16_t>(millis() - start);
    if (elapsed >= C4004_TRAJECTORY_RANGE_TIMEOUT_MS) {
      break;
    }
    const uint16_t left = C4004_TRAJECTORY_RANGE_TIMEOUT_MS - elapsed;

    if (!this->read_frame(&packet, left)) {
      continue;
    }
    this->handle_packet(&packet);

    if (packet.control != C4004_CTRL_DETECTION_RANGE) {
      continue;
    }
    if (packet.cmd == C4004_CMD_DETECTION_RANGE_SET_RANGE ||
        (packet.cmd == C4004_CMD_DETECTION_RANGE_QUERY_RANGE && packet.len > 0 &&
         static_cast<DetectionRangeMode>(packet.data[0]) == RANGE_TRAJECTORY)) {
      this->range_mode_ = RANGE_TRAJECTORY;
      this->trajectory_range_mode_ = true;
      this->learned_trajectory_learning_enabled_ = learning;
      if (learning) {
        this->learned_trajectory_range_point_count_ = 0;
      }
      this->publish_switch_states();
      this->publish_detection_range_mode();
      return true;
    }
  }

  if (this->verify_trajectory_range_mode_state(learning)) {
    this->range_mode_ = RANGE_TRAJECTORY;
    this->trajectory_range_mode_ = true;
    this->learned_trajectory_learning_enabled_ = learning;
    if (learning) {
      this->learned_trajectory_range_point_count_ = 0;
    }
    this->publish_switch_states();
    this->publish_detection_range_mode();
    return true;
  }

  return false;
}

bool C4004Component::clear_all_tags() {
  const uint8_t data = 0xFF;
  Packet packet;
  if (!this->request_frame(C4004_CTRL_DETECTION_RANGE, C4004_CMD_DETECTION_RANGE_CLEAR_TAG, &data, 1, &packet)) {
    return false;
  }
  if (packet.len > 0) {
    if (packet.data[0] == 0xFE) {
      return false;
    }
    if (packet.data[0] != 0xFF) {
      return false;
    }
  }
  this->multi_tag_config_hex_ = "0000";
  this->multi_tag_config_count_ = 0;
  for (uint8_t i = 0; i < C4004_TAG_ZONE_COUNT; i++) {
    this->tag_zone_states_[i] = TagZoneState{};
  }
  this->publish_tag_zone_states();
  this->publish_multi_tag_config_mqtt_state_();
  return true;
}

bool C4004Component::clear_live_count() {
  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  const bool ok = this->request_frame(C4004_CTRL_PEOPLE_COUNT, C4004_CMD_PEOPLE_COUNT_CLEAR_COUNT, &data, 1, &packet);
  if (ok) {
    this->live_count_ = 0;
    this->publish_live_count();
  }
  return ok;
}

bool C4004Component::set_presence_enable(bool enable) {
  if (!this->set_byte(C4004_CTRL_PRESENCE, C4004_CMD_PRESENCE_SET_ENABLE, enable ? 1 : 0)) {
    return false;
  }
  this->presence_enable_ = enable;
  this->publish_switch_states();
  return true;
}

bool C4004Component::set_trajectory_track_enable(bool enable) {
  if (!this->set_byte(C4004_CTRL_TRAJECTORY, C4004_CMD_TRAJECTORY_SET_ENABLE, enable ? 1 : 0)) {
    return false;
  }
  this->trajectory_track_enable_ = enable;
  this->publish_switch_states();
  return true;
}

bool C4004Component::set_trk_led(bool enable) {
  if (!this->set_byte(C4004_CTRL_TRAJECTORY, C4004_CMD_TRAJECTORY_SET_TRAJECTORY_LED, enable ? 1 : 0)) {
    return false;
  }
  this->trk_led_ = enable;
  this->publish_switch_states();
  return true;
}

bool C4004Component::set_occ_led(bool enable) {
  if (!this->set_byte(C4004_CTRL_TRAJECTORY, C4004_CMD_TRAJECTORY_SET_MOTION_LED, enable ? 1 : 0)) {
    return false;
  }
  this->occ_led_ = enable;
  this->publish_switch_states();
  return true;
}

bool C4004Component::set_real_time_report_interval(float value) {
  const uint32_t new_value = static_cast<uint32_t>(value);
  if (!this->set_uint32(C4004_CTRL_PEOPLE_COUNT, C4004_CMD_PEOPLE_COUNT_SET_REPORT_INTERVAL, new_value)) {
    return false;
  }
  this->real_time_report_interval_ = new_value;
  this->publish_people_setting_numbers();
  return true;
}

bool C4004Component::set_trajectory_generation_distance(float value) {
  const uint32_t new_value = static_cast<uint32_t>(value);
  if (!this->set_uint32(C4004_CTRL_PEOPLE_COUNT, C4004_CMD_PEOPLE_COUNT_SET_TRAJECTORY_DISTANCE, new_value)) {
    return false;
  }
  this->trajectory_generation_distance_ = new_value;
  this->publish_people_setting_numbers();
  return true;
}

bool C4004Component::set_trajectory_lifetime(float value) {
  const uint32_t new_value = static_cast<uint32_t>(value);
  if (!this->set_uint32(C4004_CTRL_PEOPLE_COUNT, C4004_CMD_PEOPLE_COUNT_SET_TRAJECTORY_HOLD_TIME, new_value)) {
    return false;
  }
  this->trajectory_lifetime_ = new_value;
  this->publish_people_setting_numbers();
  return true;
}

bool C4004Component::set_unoccupied_time(float value) {
  const uint32_t new_value = static_cast<uint32_t>(value);
  if (!this->set_uint32(C4004_CTRL_PEOPLE_COUNT, C4004_CMD_PEOPLE_COUNT_SET_NO_PERSON_DELAY, new_value)) {
    return false;
  }
  this->unoccupied_time_ = new_value;
  this->publish_people_setting_numbers();
  return true;
}

bool C4004Component::set_frame_generate_count(float value) {
  const uint8_t frames = static_cast<uint8_t>(value);
  if (!this->set_frame_generate_count_raw(frames)) {
    return false;
  }
  this->frame_generate_count_ = frames;
  this->publish_frame_generate_count();
  return true;
}

void C4004Component::set_pending_install_mode(const std::string &value) {
  if (value == "Top") {
    this->install_info_.mode = INSTALL_TOP;
  } else {
    this->install_info_.mode = INSTALL_SIDE;
  }
  this->publish_install_info();
}

void C4004Component::set_pending_install_height(float value) {
  this->install_info_.height_cm = static_cast<uint16_t>(value);
  this->publish_install_info();
}

void C4004Component::set_pending_install_z_angle(float value) {
  this->install_info_.z_angle = static_cast<int16_t>(value);
  this->publish_install_info();
}

void C4004Component::set_pending_range_x_max(float value) {
  this->range_info_.x_max = static_cast<int16_t>(value);
  this->publish_boundary_range();
}

void C4004Component::set_pending_range_x_min(float value) {
  this->range_info_.x_min = static_cast<int16_t>(value);
  this->publish_boundary_range();
}

void C4004Component::set_pending_range_y_max(float value) {
  this->range_info_.y_max = static_cast<int16_t>(value);
  this->publish_boundary_range();
}

void C4004Component::set_pending_range_y_min(float value) {
  this->range_info_.y_min = static_cast<int16_t>(value);
  this->publish_boundary_range();
}

bool C4004Component::get_install_info(InstallInfo *info) {
  if (info == nullptr) {
    return false;
  }

  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  InstallInfo next;

  if (!this->request_frame(C4004_CTRL_INSTALL_INFO, C4004_CMD_INSTALL_QUERY_ANGLE, &data, 1, &packet) || packet.len < 6) {
    return false;
  }
  next.x_angle = this->read_int16(&packet.data[0]) / 100;
  next.y_angle = this->read_int16(&packet.data[2]) / 100;
  next.z_angle = this->read_int16(&packet.data[4]) / 100;

  if (!this->request_frame(C4004_CTRL_INSTALL_INFO, C4004_CMD_INSTALL_QUERY_HEIGHT, &data, 1, &packet) || packet.len < 2) {
    return false;
  }
  next.height_cm = this->read_uint16(packet.data);

  if (!this->request_frame(C4004_CTRL_INSTALL_INFO, C4004_CMD_INSTALL_QUERY_MODE, &data, 1, &packet) || packet.len < 1) {
    return false;
  }
  next.mode = static_cast<InstallMode>(packet.data[0]);
  *info = next;
  this->install_info_ = next;
  return true;
}

bool C4004Component::set_install_info(const InstallInfo &info) {
  if (info.mode != INSTALL_SIDE && info.mode != INSTALL_TOP) {
    return false;
  }

  uint8_t angle_data[6];
  uint8_t height_data[2];
  uint8_t mode_data = static_cast<uint8_t>(info.mode);
  Packet packet;

  int32_t x_angle = static_cast<int32_t>(info.x_angle) * 100;
  int32_t y_angle = static_cast<int32_t>(info.y_angle) * 100;
  int32_t z_angle = static_cast<int32_t>(info.z_angle) * 100;
  x_angle = x_angle > 18000 ? 18000 : (x_angle < -18000 ? -18000 : x_angle);
  y_angle = y_angle > 18000 ? 18000 : (y_angle < -18000 ? -18000 : y_angle);
  z_angle = z_angle > 18000 ? 18000 : (z_angle < -18000 ? -18000 : z_angle);

  this->write_int16(&angle_data[0], static_cast<int16_t>(x_angle));
  this->write_int16(&angle_data[2], static_cast<int16_t>(y_angle));
  this->write_int16(&angle_data[4], static_cast<int16_t>(z_angle));
  this->write_uint16(height_data, info.height_cm);

  if (!this->request_frame(C4004_CTRL_INSTALL_INFO, C4004_CMD_INSTALL_SET_MODE, &mode_data, 1, &packet)) {
    return false;
  }
  if (!this->request_frame(C4004_CTRL_INSTALL_INFO, C4004_CMD_INSTALL_SET_ANGLE, angle_data, sizeof(angle_data), &packet)) {
    return false;
  }
  return this->request_frame(C4004_CTRL_INSTALL_INFO, C4004_CMD_INSTALL_SET_HEIGHT, height_data, sizeof(height_data), &packet);
}

bool C4004Component::get_presence_enable(bool *enable) {
  uint8_t value = 0;
  if (enable == nullptr || !this->query_byte(C4004_CTRL_PRESENCE, C4004_CMD_PRESENCE_QUERY_ENABLE, &value)) {
    return false;
  }
  this->presence_enable_ = value != 0;
  *enable = this->presence_enable_;
  return true;
}

PresenceState C4004Component::get_presence_state(GetDataMode mode) {
  uint8_t value = NO_PRESENCE;
  if (mode == GET_DATA_ACTIVE) {
    if (this->query_byte(C4004_CTRL_PRESENCE, C4004_CMD_PRESENCE_QUERY_STATE, &value)) {
      this->presence_state_ = static_cast<PresenceState>(value);
    }
  }
  return this->presence_state_;
}

MotionState C4004Component::get_motion_state(GetDataMode mode) {
  uint8_t value = MOTION_NONE;
  if (mode == GET_DATA_ACTIVE) {
    if (this->query_byte(C4004_CTRL_PRESENCE, C4004_CMD_PRESENCE_QUERY_MOTION, &value)) {
      this->motion_state_ = static_cast<MotionState>(value);
    }
  }
  return this->motion_state_;
}

bool C4004Component::get_trajectory_track_enable(bool *enable) {
  uint8_t value = 0;
  if (enable == nullptr || !this->query_byte(C4004_CTRL_TRAJECTORY, C4004_CMD_TRAJECTORY_QUERY_ENABLE, &value)) {
    return false;
  }
  this->trajectory_track_enable_ = value != 0;
  *enable = this->trajectory_track_enable_;
  return true;
}

bool C4004Component::set_frame_generate_count_raw(uint8_t frames) {
  if (frames < 1 || frames > 7) {
    return false;
  }
  return this->set_byte(C4004_CTRL_TRAJECTORY, C4004_CMD_TRAJECTORY_SET_CHECK_TO_ACTIVE_FRAMES, frames);
}

bool C4004Component::get_frame_generate_count(uint8_t *frames) {
  uint8_t value = 0;
  if (frames == nullptr || !this->query_byte(C4004_CTRL_TRAJECTORY, C4004_CMD_TRAJECTORY_QUERY_CHECK_TO_ACTIVE_FRAMES, &value)) {
    return false;
  }
  this->frame_generate_count_ = value;
  *frames = value;
  return true;
}

bool C4004Component::get_trk_led(bool *enable) {
  uint8_t value = 0;
  if (enable == nullptr || !this->query_byte(C4004_CTRL_TRAJECTORY, C4004_CMD_TRAJECTORY_QUERY_TRAJECTORY_LED, &value)) {
    return false;
  }
  this->trk_led_ = value != 0;
  *enable = this->trk_led_;
  return true;
}

bool C4004Component::get_occ_led(bool *enable) {
  uint8_t value = 0;
  if (enable == nullptr || !this->query_byte(C4004_CTRL_TRAJECTORY, C4004_CMD_TRAJECTORY_QUERY_MOTION_LED, &value)) {
    return false;
  }
  this->occ_led_ = value != 0;
  *enable = this->occ_led_;
  return true;
}

uint8_t C4004Component::get_target_count_active() {
  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  this->request_frame(C4004_CTRL_TRAJECTORY, C4004_CMD_TRAJECTORY_QUERY_TARGET, &data, 1, &packet);
  return this->target_count_;
}

DetectionRangeMode C4004Component::get_detection_range_mode() {
  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  if (this->request_frame(C4004_CTRL_DETECTION_RANGE, C4004_CMD_DETECTION_RANGE_QUERY_RANGE, &data, 1, &packet)) {
    return this->range_mode_;
  }
  return this->range_mode_;
}

bool C4004Component::get_four_sided_range_mode(FourSidedRange *range) {
  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  if (range == nullptr) {
    return false;
  }
  if (!this->request_frame(C4004_CTRL_DETECTION_RANGE, C4004_CMD_DETECTION_RANGE_QUERY_RANGE, &data, 1, &packet)) {
    return false;
  }
  *range = this->range_info_;
  return true;
}

uint8_t C4004Component::get_tag_configs(TagConfig *tags, uint8_t max_tags) {
  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  if (!this->request_frame(C4004_CTRL_DETECTION_RANGE, C4004_CMD_DETECTION_RANGE_QUERY_TAGS, &data, 1, &packet)) {
    return 0;
  }
  return this->parse_tag_list(packet.data, packet.len, tags, max_tags);
}

uint8_t C4004Component::get_live_count(GetDataMode mode) {
  if (mode == GET_DATA_ACTIVE) {
    const uint8_t data = C4004_QUERY_DATA;
    Packet packet;
    this->request_frame(C4004_CTRL_PEOPLE_COUNT, C4004_CMD_PEOPLE_COUNT_QUERY_COUNT, &data, 1, &packet);
  }
  return this->live_count_;
}

bool C4004Component::get_real_time_report_interval(uint32_t *value) {
  return this->query_uint32(C4004_CTRL_PEOPLE_COUNT, C4004_CMD_PEOPLE_COUNT_QUERY_REPORT_INTERVAL, value);
}

bool C4004Component::get_trajectory_generation_distance(uint32_t *value) {
  return this->query_uint32(C4004_CTRL_PEOPLE_COUNT, C4004_CMD_PEOPLE_COUNT_QUERY_TRAJECTORY_DISTANCE, value);
}

bool C4004Component::get_trajectory_lifetime(uint32_t *value) {
  return this->query_uint32(C4004_CTRL_PEOPLE_COUNT, C4004_CMD_PEOPLE_COUNT_QUERY_TRAJECTORY_HOLD_TIME, value);
}

bool C4004Component::get_unoccupied_time(uint32_t *value) {
  return this->query_uint32(C4004_CTRL_PEOPLE_COUNT, C4004_CMD_PEOPLE_COUNT_QUERY_NO_PERSON_DELAY, value);
}

bool C4004Component::set_byte(uint8_t control, uint8_t cmd, uint8_t value) {
  Packet packet;
  return this->request_frame(control, cmd, &value, 1, &packet);
}

bool C4004Component::query_byte(uint8_t control, uint8_t cmd, uint8_t *value) {
  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  if (value == nullptr || !this->request_frame(control, cmd, &data, 1, &packet) || packet.len < 1) {
    return false;
  }
  *value = packet.data[0];
  return true;
}

bool C4004Component::set_uint32(uint8_t control, uint8_t cmd, uint32_t value) {
  uint8_t data[4];
  Packet packet;
  this->write_uint32(data, value);
  return this->request_frame(control, cmd, data, sizeof(data), &packet);
}

bool C4004Component::query_uint32(uint8_t control, uint8_t cmd, uint32_t *value) {
  const uint8_t data = C4004_QUERY_DATA;
  Packet packet;
  if (value == nullptr || !this->request_frame(control, cmd, &data, 1, &packet) || packet.len < 4) {
    return false;
  }
  *value = this->read_uint32(packet.data);
  return true;
}

bool C4004Component::send_command(uint8_t control, uint8_t cmd, const uint8_t *data, uint16_t len) {
  if (len > C4004_MAX_PAYLOAD) {
    return false;
  }

  uint8_t frame[C4004_MAX_PAYLOAD + 9];
  uint16_t offset = 0;
  uint8_t checksum = 0;

  frame[offset++] = C4004_FRAME_HEAD1;
  frame[offset++] = C4004_FRAME_HEAD2;
  frame[offset++] = control;
  frame[offset++] = cmd;
  frame[offset++] = static_cast<uint8_t>(len >> 8);
  frame[offset++] = static_cast<uint8_t>(len & 0xFF);
  for (uint16_t i = 0; i < len; i++) {
    frame[offset++] = data == nullptr ? 0 : data[i];
  }
  for (uint16_t i = 0; i < offset; i++) {
    checksum += frame[i];
  }
  frame[offset++] = checksum;
  frame[offset++] = C4004_FRAME_TAIL1;
  frame[offset++] = C4004_FRAME_TAIL2;
  this->write_array(frame, offset);
  return true;
}

bool C4004Component::request_frame(uint8_t control, uint8_t cmd, const uint8_t *data, uint16_t len, Packet *response,
                                   uint16_t timeout_ms) {
  if (response == nullptr) {
    return false;
  }
  std::memset(response, 0, sizeof(Packet));

  if (!this->send_command(control, cmd, data, len)) {
    return false;
  }

  const uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    const uint16_t elapsed = static_cast<uint16_t>(millis() - start);
    const uint16_t left = elapsed >= timeout_ms ? 1 : timeout_ms - elapsed;
    if (!this->read_frame(response, left)) {
      continue;
    }
    this->handle_packet(response);
    if (response->control == control && response->cmd == cmd) {
      return true;
    }
  }
  return false;
}

void C4004Component::reset_rx_parser_() {
  this->asm_state_ = RX_ASM_SYNC_H1;
  this->asm_idx_ = 0;
  this->asm_checksum_ = 0;
  this->asm_recv_checksum_ = 0;
}

void C4004Component::discard_rx_ring_() {
  this->rx_head_ = 0;
  this->rx_tail_ = 0;
}

void C4004Component::rx_push_byte_(uint8_t value) {
  const uint16_t next_head = static_cast<uint16_t>((this->rx_head_ + 1) % C4004_RX_RING_SIZE);
  if (next_head == this->rx_tail_) {
    this->rx_tail_ = static_cast<uint16_t>((this->rx_tail_ + 1) % C4004_RX_RING_SIZE);
  }
  this->rx_ring_[this->rx_head_] = value;
  this->rx_head_ = next_head;
}

bool C4004Component::rx_pop_byte_(uint8_t *value) {
  if (value == nullptr || this->rx_tail_ == this->rx_head_) {
    return false;
  }
  *value = this->rx_ring_[this->rx_tail_];
  this->rx_tail_ = static_cast<uint16_t>((this->rx_tail_ + 1) % C4004_RX_RING_SIZE);
  return true;
}

void C4004Component::feed_asm_byte_(uint8_t value) {
  switch (this->asm_state_) {
    case RX_ASM_SYNC_H1:
      if (value == C4004_FRAME_HEAD1) {
        this->asm_checksum_ = value;
        this->asm_state_ = RX_ASM_SYNC_H2;
      }
      break;

    case RX_ASM_SYNC_H2:
      if (value == C4004_FRAME_HEAD2) {
        this->asm_checksum_ += value;
        this->asm_state_ = RX_ASM_CTRL;
      } else {
        this->asm_state_ = RX_ASM_SYNC_H1;
        if (value == C4004_FRAME_HEAD1) {
          this->asm_checksum_ = value;
          this->asm_state_ = RX_ASM_SYNC_H2;
        }
      }
      break;

    case RX_ASM_CTRL:
      this->pending_packet_.control = value;
      this->asm_checksum_ += value;
      this->asm_state_ = RX_ASM_CMD;
      break;

    case RX_ASM_CMD:
      this->pending_packet_.cmd = value;
      this->asm_checksum_ += value;
      this->asm_state_ = RX_ASM_LEN_HI;
      break;

    case RX_ASM_LEN_HI:
      this->pending_packet_.len = static_cast<uint16_t>(value) << 8;
      this->asm_checksum_ += value;
      this->asm_state_ = RX_ASM_LEN_LO;
      break;

    case RX_ASM_LEN_LO:
      this->pending_packet_.len |= value;
      this->asm_checksum_ += value;
      if (this->pending_packet_.len > C4004_MAX_PAYLOAD) {
        this->reset_rx_parser_();
        this->discard_rx_ring_();
        break;
      }
      this->asm_idx_ = 0;
      this->asm_state_ = this->pending_packet_.len == 0 ? RX_ASM_CHECKSUM : RX_ASM_PAYLOAD;
      break;

    case RX_ASM_PAYLOAD:
      this->pending_packet_.data[this->asm_idx_++] = value;
      this->asm_checksum_ += value;
      if (this->asm_idx_ >= this->pending_packet_.len) {
        this->asm_state_ = RX_ASM_CHECKSUM;
      }
      break;

    case RX_ASM_CHECKSUM:
      this->asm_recv_checksum_ = value;
      this->asm_state_ = RX_ASM_TAIL1;
      break;

    case RX_ASM_TAIL1:
      if (value != C4004_FRAME_TAIL1) {
        this->reset_rx_parser_();
        break;
      }
      this->asm_state_ = RX_ASM_TAIL2;
      break;

    case RX_ASM_TAIL2:
      if (value != C4004_FRAME_TAIL2 || this->asm_checksum_ != this->asm_recv_checksum_) {
        this->reset_rx_parser_();
        break;
      }
      this->pending_valid_ = true;
      this->reset_rx_parser_();
      break;

    default:
      this->reset_rx_parser_();
      break;
  }
}

void C4004Component::pump_rx_() {
  uint8_t value = 0;

  while (this->available() > 0) {
    this->read_array(&value, 1);
    this->rx_push_byte_(value);
  }

  while (!this->pending_valid_ && this->rx_pop_byte_(&value)) {
    this->feed_asm_byte_(value);
  }
}

bool C4004Component::take_pending_frame_(Packet *packet) {
  if (packet == nullptr || !this->pending_valid_) {
    return false;
  }
  *packet = this->pending_packet_;
  this->pending_valid_ = false;
  return true;
}

bool C4004Component::read_frame(Packet *packet, uint16_t timeout_ms) {
  if (packet == nullptr) {
    return false;
  }
  std::memset(packet, 0, sizeof(Packet));

  const uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    this->pump_rx_();
    if (this->take_pending_frame_(packet)) {
      return true;
    }
    delay(1);
  }
  return false;
}

bool C4004Component::read_byte(uint8_t *value, uint16_t timeout_ms) {
  if (value == nullptr) {
    return false;
  }

  const uint32_t start = millis();
  do {
    if (this->available() > 0) {
      this->read_array(value, 1);
      return true;
    }
    delay(1);
  } while (millis() - start < timeout_ms);
  return false;
}

void C4004Component::flush_input() {
  this->discard_rx_ring_();
  this->reset_rx_parser_();
  this->pending_valid_ = false;

  uint8_t tmp[32];
  while (this->available() > 0) {
    const size_t to_read = this->available() > sizeof(tmp) ? sizeof(tmp) : this->available();
    this->read_array(tmp, to_read);
  }
}

ReportedEvent C4004Component::handle_packet(const Packet *packet) {
  if (packet == nullptr) {
    return EVENT_ERROR;
  }

  if (packet->control == C4004_CTRL_SYSTEM &&
      (packet->cmd == C4004_CMD_SYSTEM_HEARTBEAT_REPORT || packet->cmd == C4004_CMD_SYSTEM_HEARTBEAT_QUERY)) {
    this->heartbeat_ = true;
  } else if (packet->control == C4004_CTRL_WORK_STATUS &&
             (packet->cmd == C4004_CMD_WORK_STATUS_INIT_FINISHED_REPORT ||
              packet->cmd == C4004_CMD_WORK_STATUS_INIT_FINISHED_QUERY)) {
    if (packet->len > 0) {
      this->init_finished_ = packet->data[0] == 0x01 || packet->cmd == C4004_CMD_WORK_STATUS_INIT_FINISHED_REPORT;
    }
  } else if (packet->control == C4004_CTRL_PRESENCE && packet->cmd == C4004_CMD_PRESENCE_QUERY_ENABLE && packet->len > 0) {
    this->presence_enable_ = packet->data[0] != 0;
  } else if (packet->control == C4004_CTRL_PRESENCE &&
             (packet->cmd == C4004_CMD_PRESENCE_REPORT || packet->cmd == C4004_CMD_PRESENCE_QUERY_STATE) && packet->len > 0) {
    this->presence_state_ = static_cast<PresenceState>(packet->data[0]);
  } else if (packet->control == C4004_CTRL_PRESENCE &&
             (packet->cmd == C4004_CMD_PRESENCE_MOTION_REPORT || packet->cmd == C4004_CMD_PRESENCE_QUERY_MOTION) &&
             packet->len > 0) {
    this->motion_state_ = static_cast<MotionState>(packet->data[0]);
  } else if (packet->control == C4004_CTRL_TRAJECTORY &&
             (packet->cmd == C4004_CMD_TRAJECTORY_TARGET_REPORT || packet->cmd == C4004_CMD_TRAJECTORY_QUERY_TARGET)) {
    this->parse_targets(packet->data, packet->len);
  } else if (packet->control == C4004_CTRL_TRAJECTORY && packet->cmd == C4004_CMD_TRAJECTORY_QUERY_TRAJECTORY_LED &&
             packet->len > 0) {
    this->trk_led_ = packet->data[0] != 0;
  } else if (packet->control == C4004_CTRL_TRAJECTORY && packet->cmd == C4004_CMD_TRAJECTORY_QUERY_MOTION_LED &&
             packet->len > 0) {
    this->occ_led_ = packet->data[0] != 0;
  } else if (packet->control == C4004_CTRL_DETECTION_RANGE && packet->cmd == C4004_CMD_DETECTION_RANGE_TAG_REPORT) {
    this->parse_tag_event(packet->data, packet->len);
  } else if (packet->control == C4004_CTRL_DETECTION_RANGE &&
             (packet->cmd == C4004_CMD_DETECTION_RANGE_QUERY_RANGE || packet->cmd == C4004_CMD_DETECTION_RANGE_SET_RANGE)) {
    this->parse_detection_range_payload(packet->data, packet->len);
  } else if (packet->control == C4004_CTRL_PEOPLE_COUNT &&
             (packet->cmd == C4004_CMD_PEOPLE_COUNT_REPORT || packet->cmd == C4004_CMD_PEOPLE_COUNT_QUERY_COUNT)) {
    this->parse_live_count(packet->data, packet->len);
  }

  return this->classify_packet(packet);
}

ReportedEvent C4004Component::classify_packet(const Packet *packet) const {
  if (packet == nullptr) {
    return EVENT_ERROR;
  }
  if (packet->control == C4004_CTRL_SYSTEM &&
      (packet->cmd == C4004_CMD_SYSTEM_HEARTBEAT_REPORT || packet->cmd == C4004_CMD_SYSTEM_HEARTBEAT_QUERY)) {
    return EVENT_HEARTBEAT;
  }
  if (packet->control == C4004_CTRL_WORK_STATUS && packet->cmd == C4004_CMD_WORK_STATUS_INIT_FINISHED_REPORT) {
    return EVENT_INIT_FINISHED;
  }
  if (packet->control == C4004_CTRL_PRESENCE &&
      (packet->cmd == C4004_CMD_PRESENCE_REPORT || packet->cmd == C4004_CMD_PRESENCE_QUERY_STATE)) {
    return EVENT_PRESENCE;
  }
  if (packet->control == C4004_CTRL_PRESENCE &&
      (packet->cmd == C4004_CMD_PRESENCE_MOTION_REPORT || packet->cmd == C4004_CMD_PRESENCE_QUERY_MOTION)) {
    return EVENT_MOTION;
  }
  if (packet->control == C4004_CTRL_TRAJECTORY &&
      (packet->cmd == C4004_CMD_TRAJECTORY_TARGET_REPORT || packet->cmd == C4004_CMD_TRAJECTORY_QUERY_TARGET)) {
    return EVENT_TRAJECTORY;
  }
  if (packet->control == C4004_CTRL_DETECTION_RANGE && packet->cmd == C4004_CMD_DETECTION_RANGE_TAG_REPORT) {
    return EVENT_TAG;
  }
  if (packet->control == C4004_CTRL_PEOPLE_COUNT &&
      (packet->cmd == C4004_CMD_PEOPLE_COUNT_REPORT || packet->cmd == C4004_CMD_PEOPLE_COUNT_QUERY_COUNT)) {
    return EVENT_PEOPLE_COUNT;
  }
  return EVENT_UNKNOWN;
}

void C4004Component::parse_targets(const uint8_t *data, uint16_t len) {
  if (data == nullptr) {
    this->target_count_ = 0;
    return;
  }
  uint8_t count = len / C4004_TARGET_RECORD_LEN;
  if (count > C4004_MAX_TARGETS) {
    count = C4004_MAX_TARGETS;
  }
  this->target_count_ = count;

  for (uint8_t i = 0; i < count; i++) {
    const uint16_t offset = i * C4004_TARGET_RECORD_LEN;
    this->targets_[i].index = data[offset];
    this->targets_[i].kinesia = data[offset + 1];
    this->targets_[i].target_feature = static_cast<TargetFeature>(data[offset + 2]);
    this->targets_[i].x = this->read_sign_bit_int16(&data[offset + 3]);
    this->targets_[i].y = this->read_sign_bit_int16(&data[offset + 5]);
    this->targets_[i].height = this->read_sign_bit_int16(&data[offset + 7]);
    this->targets_[i].speed = this->read_sign_bit_int16(&data[offset + 9]);
  }
}

uint8_t C4004Component::parse_tag_list(const uint8_t *data, uint16_t len, TagConfig *tags, uint8_t max_tags) {
  const uint8_t tag_len = 12;
  if (data == nullptr || len < 2) {
    return 0;
  }

  const uint16_t total = this->read_uint16(data);
  uint8_t actual_count = total > 0xFF ? 0xFF : static_cast<uint8_t>(total);
  const uint16_t available_len = len - 2;
  if (available_len < static_cast<uint16_t>(actual_count * tag_len)) {
    actual_count = available_len / tag_len;
  }

  if (tags != nullptr) {
    uint8_t copy_count = actual_count;
    if (copy_count > max_tags) {
      copy_count = max_tags;
    }
    for (uint8_t i = 0; i < copy_count; i++) {
      const uint16_t offset = 2 + i * tag_len;
      tags[i].tag_index = data[offset];
      tags[i].tag_type = static_cast<TagType>(data[offset + 1]);
      tags[i].scope_type = static_cast<TagRangeType>(data[offset + 2]);
      tags[i].io_index = data[offset + 3];
      tags[i].center_x = this->read_sign_bit_int16(&data[offset + 4]);
      tags[i].center_y = this->read_sign_bit_int16(&data[offset + 6]);
      tags[i].width = this->read_uint16(&data[offset + 8]);
      tags[i].height = this->read_uint16(&data[offset + 10]);
    }
  }
  return actual_count;
}

void C4004Component::parse_tag_event(const uint8_t *data, uint16_t len) {
  if (data == nullptr || len < 8) {
    this->tag_info_valid_ = false;
    return;
  }

  std::memset(&this->tag_info_, 0, sizeof(this->tag_info_));
  this->tag_info_.tag_index = data[0];
  this->tag_info_.tag_type = static_cast<TagType>(data[1]);
  this->tag_info_.io_index = data[2];
  this->tag_info_.center_x = this->read_sign_bit_int16(&data[3]);
  this->tag_info_.center_y = this->read_sign_bit_int16(&data[5]);
  if (this->tag_info_.tag_type == TAG_BOUNDARY) {
    this->tag_info_.enter_exit = data[7];
  } else if (this->tag_info_.tag_type == TAG_APPROACH_AWAY) {
    this->tag_info_.motion_dir = data[7];
  } else if (this->tag_info_.tag_type == TAG_PEOPLE_COUNTING) {
    this->tag_info_.motion_num = (data[7] >> 4) & 0x0F;
    this->tag_info_.static_num = data[7] & 0x0F;
  }
  this->tag_info_valid_ = true;
  this->update_tag_zone_cache(this->tag_info_);
}

void C4004Component::parse_detection_range_payload(const uint8_t *data, uint16_t len) {
  if (data == nullptr || len < 1) {
    return;
  }

  const DetectionRangeMode previous_mode = this->range_mode_;
  this->range_mode_ = static_cast<DetectionRangeMode>(data[0]);
  this->trajectory_range_mode_ = this->range_mode_ == RANGE_TRAJECTORY;
  if (this->range_mode_ != previous_mode) {
    this->publish_detection_range_mode();
  }
  if (this->range_mode_ == RANGE_TRAJECTORY) {
    // Learning publishes only its active state. Keep the last completed
    // range untouched until the explicit post-stop query returns points.
    if (this->learned_trajectory_learning_enabled_) {
      this->learned_trajectory_range_point_count_ = 0;
      this->publish_learned_trajectory_range_mqtt_state_();
      return;
    }
    if (this->cache_range_points_payload(data, len, RANGE_TRAJECTORY, &this->learned_trajectory_range_hex_,
                                         &this->learned_trajectory_learning_enabled_,
                                         &this->learned_trajectory_range_point_count_)) {
      this->publish_learned_trajectory_range_mqtt_state_();
    }
    return;
  }
  if (this->range_mode_ == RANGE_CONFIG_FILE) {
    if (this->cache_range_points_payload(data, len, RANGE_CONFIG_FILE, &this->config_file_range_hex_, nullptr,
                                         &this->config_file_range_point_count_)) {
      this->publish_config_file_range_mqtt_state_();
    }
    return;
  }
  if (this->range_mode_ != RANGE_FOUR_SIDE) {
    return;
  }

  uint8_t offset = 1;
  if (len >= 10 && data[1] == 0x00) {
    offset = 2;
  }
  if (len >= static_cast<uint16_t>(offset + 8)) {
    this->range_info_.x_max = this->read_sign_bit_int16(&data[offset]);
    this->range_info_.x_min = this->read_sign_bit_int16(&data[offset + 2]);
    this->range_info_.y_max = this->read_sign_bit_int16(&data[offset + 4]);
    this->range_info_.y_min = this->read_sign_bit_int16(&data[offset + 6]);
  }
}

void C4004Component::parse_live_count(const uint8_t *data, uint16_t len) {
  if (data == nullptr || len == 0) {
    this->live_count_ = 0;
  } else if (len >= 2) {
    this->live_count_ = data[1];
  } else {
    this->live_count_ = data[0];
  }
}

bool C4004Component::query_string(uint8_t control, uint8_t cmd, std::string *value) {
  if (value == nullptr) {
    return false;
  }
  value->clear();

  const uint8_t data = C4004_QUERY_DATA;
  Packet &packet = this->rx_packet_;
  if (!this->request_frame(control, cmd, &data, 1, &packet)) {
    return false;
  }
  for (uint16_t i = 0; i < packet.len; i++) {
    if (packet.data[i] != 0) {
      value->push_back(static_cast<char>(packet.data[i]));
    }
  }
  return true;
}

uint16_t C4004Component::read_uint16(const uint8_t *data) const {
  return static_cast<uint16_t>(data[0]) << 8 | data[1];
}

int16_t C4004Component::read_int16(const uint8_t *data) const {
  return static_cast<int16_t>(this->read_uint16(data));
}

int16_t C4004Component::read_sign_bit_int16(const uint8_t *data) const {
  const uint16_t raw = this->read_uint16(data);
  const int16_t magnitude = static_cast<int16_t>(raw & 0x7FFF);
  return (raw & 0x8000) != 0 ? static_cast<int16_t>(-magnitude) : magnitude;
}

uint32_t C4004Component::read_uint32(const uint8_t *data) const {
  return static_cast<uint32_t>(data[0]) << 24 | static_cast<uint32_t>(data[1]) << 16 |
         static_cast<uint32_t>(data[2]) << 8 | data[3];
}

void C4004Component::write_uint16(uint8_t *data, uint16_t value) const {
  data[0] = static_cast<uint8_t>(value >> 8);
  data[1] = static_cast<uint8_t>(value & 0xFF);
}

void C4004Component::write_int16(uint8_t *data, int16_t value) const {
  this->write_uint16(data, static_cast<uint16_t>(value));
}

void C4004Component::write_sign_bit_int16(uint8_t *data, int16_t value) const {
  int32_t magnitude = value;
  uint16_t raw = 0;
  if (magnitude < 0) {
    magnitude = -magnitude;
    raw = 0x8000;
  }
  if (magnitude > 0x7FFF) {
    magnitude = 0x7FFF;
  }
  raw |= static_cast<uint16_t>(magnitude);
  this->write_uint16(data, raw);
}

void C4004Component::write_uint32(uint8_t *data, uint32_t value) const {
  data[0] = static_cast<uint8_t>(value >> 24);
  data[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
  data[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
  data[3] = static_cast<uint8_t>(value & 0xFF);
}

std::string C4004Component::encode_hex_payload(const uint8_t *payload, uint16_t payload_len) const {
  if (payload == nullptr || payload_len == 0) {
    return "";
  }

  std::string hex;
  hex.reserve(payload_len * 2);
  for (uint16_t i = 0; i < payload_len; i++) {
    hex.push_back(hex_digit((payload[i] >> 4) & 0x0F));
    hex.push_back(hex_digit(payload[i] & 0x0F));
  }
  return hex;
}

bool C4004Component::validate_range_points_payload(const uint8_t *payload, uint16_t payload_len,
                                                   DetectionRangeMode expected_mode, std::string *normalized,
                                                   uint16_t *point_count) const {
  if (payload == nullptr || normalized == nullptr) {
    return false;
  }
  if (payload_len < 3) {
    return false;
  }
  if (static_cast<DetectionRangeMode>(payload[0]) != expected_mode) {
    return false;
  }

  const uint16_t count = this->read_uint16(&payload[1]);
  if (count > C4004_MAX_POINTS) {
    return false;
  }
  const uint16_t expected_len = static_cast<uint16_t>(3 + count * 4);
  if (payload_len != expected_len) {
    return false;
  }

  if (point_count != nullptr) {
    *point_count = count;
  }
  *normalized = this->encode_hex_payload(payload, payload_len);
  return true;
}

bool C4004Component::validate_trajectory_range_payload(const uint8_t *payload, uint16_t payload_len,
                                                       std::string *normalized, bool *learning_enabled,
                                                       uint16_t *point_count) const {
  if (payload == nullptr || normalized == nullptr) {
    return false;
  }
  if (payload_len < 2) {
    return false;
  }
  if (static_cast<DetectionRangeMode>(payload[0]) != RANGE_TRAJECTORY) {
    return false;
  }

  if (payload_len == 2) {
    const bool learning = payload[1] != 0;
    if (learning_enabled != nullptr) {
      *learning_enabled = learning;
    }
    if (point_count != nullptr) {
      *point_count = 0;
    }
    *normalized = this->encode_hex_payload(payload, 2);
    return true;
  }

  const uint16_t official_count = this->read_uint16(&payload[1]);
  const uint16_t official_len = static_cast<uint16_t>(3 + official_count * 4);
  if (payload_len == official_len && official_count <= C4004_MAX_POINTS) {
    if (learning_enabled != nullptr) {
      *learning_enabled = false;
    }
    if (point_count != nullptr) {
      *point_count = official_count;
    }
    *normalized = this->encode_hex_payload(payload, payload_len);
    return true;
  }

  const bool learning = payload[1] != 0;
  if (learning_enabled != nullptr) {
    *learning_enabled = learning;
  }

  if (learning) {
    if (point_count != nullptr) {
      *point_count = 0;
    }
    *normalized = this->encode_hex_payload(payload, 2);
    return true;
  }

  if (payload_len < 4) {
    return false;
  }
  const uint16_t count = this->read_uint16(&payload[2]);
  if (count > C4004_MAX_POINTS) {
    return false;
  }
  const uint16_t expected_len = static_cast<uint16_t>(4 + count * 4);
  if (payload_len != expected_len) {
    return false;
  }

  if (point_count != nullptr) {
    *point_count = count;
  }
  *normalized = this->encode_hex_payload(payload, payload_len);
  return true;
}

bool C4004Component::cache_range_points_payload(const uint8_t *payload, uint16_t payload_len,
                                                DetectionRangeMode expected_mode, std::string *cache,
                                                bool *learning_enabled, uint16_t *point_count) {
  if (cache == nullptr) {
    return false;
  }

  std::string normalized;
  if (expected_mode == RANGE_TRAJECTORY) {
    if (!this->validate_trajectory_range_payload(payload, payload_len, &normalized, learning_enabled, point_count)) {
      return false;
    }
  } else {
    if (!this->validate_range_points_payload(payload, payload_len, expected_mode, &normalized, point_count)) {
      return false;
    }
  }

  *cache = normalized;
  return true;
}

bool C4004Component::rebuild_multi_tag_config_hex_from_tags(const TagConfig *tags, uint8_t count) {
  if (count > C4004_TAG_CONFIG_LIMIT) {
    count = C4004_TAG_CONFIG_LIMIT;
  }

  uint8_t payload[C4004_TAG_MULTI_CONFIG_MAX_PAYLOAD]{};
  const uint16_t payload_len = static_cast<uint16_t>(2 + count * C4004_TAG_CONFIG_RECORD_LEN);
  this->write_uint16(payload, count);

  for (uint8_t i = 0; i < count; i++) {
    const uint16_t offset = static_cast<uint16_t>(2 + i * C4004_TAG_CONFIG_RECORD_LEN);
    payload[offset] = tags[i].tag_index;
    payload[offset + 1] = static_cast<uint8_t>(tags[i].tag_type);
    payload[offset + 2] = static_cast<uint8_t>(tags[i].scope_type);
    payload[offset + 3] = tags[i].io_index;
    this->write_sign_bit_int16(&payload[offset + 4], tags[i].center_x);
    this->write_sign_bit_int16(&payload[offset + 6], tags[i].center_y);
    this->write_uint16(&payload[offset + 8], tags[i].width);
    this->write_uint16(&payload[offset + 10], tags[i].height);
  }

  this->multi_tag_config_hex_ = this->encode_hex_payload(payload, payload_len);
  this->multi_tag_config_count_ = count;
  return true;
}

void C4004Component::sync_device_state() {
  InstallInfo install_info;
  FourSidedRange range_info;
  bool bool_value = false;
  uint8_t uint8_value = 0;
  uint32_t uint32_value = 0;

  if (this->get_install_info(&install_info)) {
    this->install_info_ = install_info;
  }
  if (this->get_presence_enable(&bool_value)) {
    this->presence_enable_ = bool_value;
  }
  if (this->get_trajectory_track_enable(&bool_value)) {
    this->trajectory_track_enable_ = bool_value;
  }
  if (this->get_trk_led(&bool_value)) {
    this->trk_led_ = bool_value;
  } else {
    this->trk_led_ = true;
  }
  if (this->get_occ_led(&bool_value)) {
    this->occ_led_ = bool_value;
  } else {
    this->occ_led_ = true;
  }
  if (this->get_four_sided_range_mode(&range_info)) {
    this->range_info_ = range_info;
  }
  this->sync_tag_zone_cache();
  this->get_detection_range_mode();
  this->get_presence_state();
  this->get_motion_state();
  this->get_target_count_active();
  this->get_live_count(GET_DATA_ACTIVE);

  if (this->get_real_time_report_interval(&uint32_value)) {
    this->real_time_report_interval_ = uint32_value;
  }
  if (this->get_trajectory_generation_distance(&uint32_value)) {
    this->trajectory_generation_distance_ = uint32_value;
  }
  if (this->get_trajectory_lifetime(&uint32_value)) {
    this->trajectory_lifetime_ = uint32_value;
  }
  if (this->get_unoccupied_time(&uint32_value)) {
    this->unoccupied_time_ = uint32_value;
  }
  if (this->get_frame_generate_count(&uint8_value)) {
    this->frame_generate_count_ = uint8_value;
  }

  this->publish_all_states();
}

void C4004Component::sync_tag_zone_cache() {
  TagConfig tags[C4004_TAG_CONFIG_LIMIT]{};
  const uint8_t count = this->get_tag_configs(tags, C4004_TAG_CONFIG_LIMIT);

  for (uint8_t i = 0; i < C4004_TAG_ZONE_COUNT; i++) {
    this->tag_zone_states_[i] = TagZoneState{};
  }

  for (uint8_t i = 0; i < count; i++) {
    if (tags[i].tag_index < C4004_TAG_ZONE_COUNT) {
      this->tag_zone_states_[tags[i].tag_index].tag_type = tags[i].tag_type;
    }
  }

  this->rebuild_multi_tag_config_hex_from_tags(tags, count);
}

void C4004Component::update_tag_zone_cache(const TagInfo &tag_info) {
  if (tag_info.tag_index >= C4004_TAG_ZONE_COUNT) {
    return;
  }

  TagZoneState &zone = this->tag_zone_states_[tag_info.tag_index];
  zone.tag_type = tag_info.tag_type;

  if (tag_info.tag_type == TAG_BOUNDARY) {
    zone.boundary_valid = true;
    zone.boundary_state = tag_info.enter_exit;
  } else if (tag_info.tag_type == TAG_APPROACH_AWAY) {
    zone.approach_away_valid = true;
    zone.approach_away_state = tag_info.motion_dir;
  } else if (tag_info.tag_type == TAG_PEOPLE_COUNTING) {
    zone.people_counting_valid = true;
    zone.moving_count = tag_info.motion_num;
    zone.static_count = tag_info.static_num;
  }
}

void C4004Component::publish_all_states() {
  this->publish_zone_presence_states();
  this->publish_presence_state();
  this->publish_motion_state(true);
  this->publish_live_count(true);
  this->publish_target_count(true);
  this->publish_all_mqtt_bridge_states_(true);
  this->publish_install_info();
  this->publish_boundary_range();
  this->publish_switch_states();
  this->publish_people_setting_numbers();
  this->publish_frame_generate_count();
  this->publish_zone_mcu_io_numbers();
  this->publish_detection_range_mode();
  this->publish_tag_zone_states();
}

void C4004Component::publish_online(bool online) {
#ifdef USE_BINARY_SENSOR
  if (this->online_binary_sensor_ != nullptr) {
    this->online_binary_sensor_->publish_state(online);
  }
#else
  (void) online;
#endif
}

void C4004Component::publish_zone_presence_states() {
#ifdef USE_BINARY_SENSOR
  for (uint8_t i = 0; i < C4004_ZONE_GPIO_COUNT; i++) {
    if (this->zone_presence_binary_sensors_[i] != nullptr) {
      this->zone_presence_binary_sensors_[i]->publish_state(this->zone_presence_[i]);
    }
  }
#endif
}

void C4004Component::publish_presence_state() {
#ifdef USE_BINARY_SENSOR
  if (this->presence_binary_sensor_ != nullptr) {
    this->presence_binary_sensor_->publish_state(this->presence_state_ == PRESENCE);
  }
#endif
}

void C4004Component::publish_motion_state(bool force) {
#ifdef USE_SENSOR
  if (!force && this->motion_state_published_ && this->motion_state_ == this->last_published_motion_state_) {
    return;
  }
  if (this->motion_state_sensor_ != nullptr) {
    this->motion_state_sensor_->publish_state(static_cast<float>(this->motion_state_));
    this->last_published_motion_state_ = this->motion_state_;
    this->motion_state_published_ = true;
  }
#endif
}

void C4004Component::publish_live_count(bool force) {
#ifdef USE_SENSOR
  if (!force && this->live_count_published_ && this->live_count_ == this->last_published_live_count_) {
    return;
  }
  if (this->live_count_sensor_ != nullptr) {
    this->live_count_sensor_->publish_state(this->live_count_);
    this->last_published_live_count_ = this->live_count_;
    this->live_count_published_ = true;
  }
#endif
}

void C4004Component::publish_install_info() {
#ifdef USE_SELECT
  if (this->install_mode_select_ != nullptr) {
    this->install_mode_select_->publish_state(this->install_mode_to_string(this->install_info_.mode));
  }
#endif
#ifdef USE_NUMBER
  if (this->install_height_number_ != nullptr) {
    this->install_height_number_->publish_state(this->install_info_.height_cm);
  }
  if (this->install_z_angle_number_ != nullptr) {
    this->install_z_angle_number_->publish_state(this->install_info_.z_angle);
  }
#endif
}

void C4004Component::publish_boundary_range() {
#ifdef USE_NUMBER
  if (this->range_x_max_number_ != nullptr) {
    this->range_x_max_number_->publish_state(this->range_info_.x_max);
  }
  if (this->range_x_min_number_ != nullptr) {
    this->range_x_min_number_->publish_state(this->range_info_.x_min);
  }
  if (this->range_y_max_number_ != nullptr) {
    this->range_y_max_number_->publish_state(this->range_info_.y_max);
  }
  if (this->range_y_min_number_ != nullptr) {
    this->range_y_min_number_->publish_state(this->range_info_.y_min);
  }
#endif
}

void C4004Component::publish_switch_states() {
#ifdef USE_SWITCH
  if (this->presence_enable_switch_ != nullptr) {
    this->presence_enable_switch_->publish_state(this->presence_enable_);
  }
  if (this->trajectory_track_enable_switch_ != nullptr) {
    this->trajectory_track_enable_switch_->publish_state(this->trajectory_track_enable_);
  }
  if (this->trk_led_switch_ != nullptr) {
    this->trk_led_switch_->publish_state(this->trk_led_);
  }
  if (this->occ_led_switch_ != nullptr) {
    this->occ_led_switch_->publish_state(this->occ_led_);
  }
  if (this->trajectory_range_mode_switch_ != nullptr) {
    this->trajectory_range_mode_switch_->publish_state(this->trajectory_range_mode_);
  }
#endif
}

void C4004Component::publish_people_setting_numbers() {
#ifdef USE_NUMBER
  if (this->real_time_report_interval_number_ != nullptr) {
    this->real_time_report_interval_number_->publish_state(this->real_time_report_interval_);
  }
  if (this->trajectory_generation_distance_number_ != nullptr) {
    this->trajectory_generation_distance_number_->publish_state(this->trajectory_generation_distance_);
  }
  if (this->trajectory_lifetime_number_ != nullptr) {
    this->trajectory_lifetime_number_->publish_state(this->trajectory_lifetime_);
  }
  if (this->unoccupied_time_number_ != nullptr) {
    this->unoccupied_time_number_->publish_state(this->unoccupied_time_);
  }
#endif
}

void C4004Component::publish_frame_generate_count() {
#ifdef USE_NUMBER
  if (this->frame_generate_count_number_ != nullptr) {
    this->frame_generate_count_number_->publish_state(this->frame_generate_count_);
  }
#endif
}

void C4004Component::publish_zone_mcu_io_numbers() {
#ifdef USE_NUMBER
  for (uint8_t i = 0; i < C4004_ZONE_GPIO_COUNT; i++) {
    if (this->zone_mcu_io_numbers_[i] != nullptr) {
      this->zone_mcu_io_numbers_[i]->publish_state(this->zone_gpio_pins_[i]);
    }
  }
#endif
}

void C4004Component::publish_target_count(bool force) {
#ifdef USE_SENSOR
  if (!force && this->target_count_published_ && this->target_count_ == this->last_published_target_count_) {
    return;
  }
  if (this->target_count_sensor_ != nullptr) {
    this->target_count_sensor_->publish_state(this->target_count_);
    this->last_published_target_count_ = this->target_count_;
    this->target_count_published_ = true;
  }
#endif
}

void C4004Component::publish_target_trajectory_mqtt_state_(bool force) {
#ifdef USE_MQTT
  if (!this->mqtt_bridge_available_()) {
    return;
  }
  if (!force && this->target_count_ == 0 && this->last_emitted_target_count_ == 0) {
    return;
  }

  const std::string topic = this->mqtt_bridge_topic_(MQTT_TOPIC_STATE_TARGET_TRAJECTORY);
  const std::string device_topic_prefix = this->mqtt_bridge_device_topic_prefix_();
  const std::string target_trajectory_hex = this->encode_target_trajectory_hex();
  if (mqtt::global_mqtt_client->publish_json(
          topic,
          [this, &device_topic_prefix, &target_trajectory_hex](JsonObject root) {
            root["schema"] = 1;
            root["type"] = "target_trajectory";
            root["device_topic_prefix"] = device_topic_prefix;
            root["mqtt_key"] = this->mqtt_bridge_key_;
            root["hex"] = target_trajectory_hex;
          },
          this->mqtt_bridge_stream_qos_, this->mqtt_bridge_retain_state_)) {
    this->last_emitted_target_count_ = this->target_count_;
  }
#else
  (void) force;
#endif
}

void C4004Component::publish_tag_event_mqtt_state_() {
#ifdef USE_MQTT
  if (!this->mqtt_bridge_available_() || !this->tag_info_valid_) {
    return;
  }

  const std::string topic = this->mqtt_bridge_topic_(MQTT_TOPIC_STATE_TAG_EVENT);
  const std::string device_topic_prefix = this->mqtt_bridge_device_topic_prefix_();
  const TagInfo tag_info = this->tag_info_;
  const bool published = mqtt::global_mqtt_client->publish_json(
      topic,
      [this, &device_topic_prefix, tag_info](JsonObject root) {
        root["schema"] = 1;
        root["type"] = "tag_event";
        root["device_topic_prefix"] = device_topic_prefix;
        root["mqtt_key"] = this->mqtt_bridge_key_;
        root["tag_index"] = tag_info.tag_index;
        root["tag_type"] = tag_type_to_mqtt_string(tag_info.tag_type);
        root["tag_type_code"] = static_cast<uint8_t>(tag_info.tag_type);
        root["io_index"] = tag_info.io_index;
        root["center_x_cm"] = tag_info.center_x;
        root["center_y_cm"] = tag_info.center_y;

        if (tag_info.tag_type == TAG_PEOPLE_COUNTING) {
          root["moving_count"] = tag_info.motion_num;
          root["static_count"] = tag_info.static_num;
        } else if (tag_info.tag_type == TAG_BOUNDARY) {
          root["boundary_state"] = boundary_state_to_mqtt_string(tag_info.enter_exit);
        } else if (tag_info.tag_type == TAG_APPROACH_AWAY) {
          root["approach_away_state"] = approach_away_state_to_mqtt_string(tag_info.motion_dir);
        }
      },
      this->mqtt_bridge_stream_qos_, false);
  if (!published) {
    ESP_LOGW(TAG, "[MQTT] tag_event publish failed, topic=%s", topic.c_str());
  }
#endif
}

bool C4004Component::mqtt_bridge_available_() const {
#ifdef USE_MQTT
  return this->mqtt_bridge_enabled_ && mqtt::global_mqtt_client != nullptr;
#else
  return false;
#endif
}

std::string C4004Component::mqtt_bridge_device_topic_prefix_() const {
  if (!this->mqtt_bridge_topic_prefix_.empty()) {
    return this->mqtt_bridge_topic_prefix_;
  }

#ifdef USE_MQTT
  if (mqtt::global_mqtt_client != nullptr) {
    return mqtt::global_mqtt_client->get_topic_prefix();
  }
#endif
  return "";
}

std::string C4004Component::mqtt_bridge_topic_base_() const {
  if (!this->mqtt_bridge_available_()) {
    return "";
  }

  const std::string prefix = this->mqtt_bridge_device_topic_prefix_();
  if (prefix.empty()) {
    return "dfrobot_c4004/" + this->mqtt_bridge_key_;
  }
  return prefix + "/dfrobot_c4004/" + this->mqtt_bridge_key_;
}

std::string C4004Component::mqtt_bridge_topic_(const char *suffix) const {
  const std::string base = this->mqtt_bridge_topic_base_();
  if (base.empty()) {
    return "";
  }
  return base + "/" + suffix;
}

void C4004Component::ensure_mqtt_bridge_subscriptions_() {
#ifdef USE_MQTT
  if (!this->mqtt_bridge_available_() || this->mqtt_bridge_subscriptions_registered_) {
    return;
  }

  mqtt::global_mqtt_client->subscribe_json(
      this->mqtt_bridge_topic_(MQTT_TOPIC_COMMAND_MULTI_TAG_CONFIG_SET),
      [this](const std::string &topic, JsonObject root) { this->handle_multi_tag_config_mqtt_command_(topic, root); },
      this->mqtt_bridge_qos_);
  mqtt::global_mqtt_client->subscribe_json(
      this->mqtt_bridge_topic_(MQTT_TOPIC_COMMAND_CONFIG_FILE_RANGE_SET),
      [this](const std::string &topic, JsonObject root) { this->handle_config_file_range_mqtt_command_(topic, root); },
      this->mqtt_bridge_qos_);
  mqtt::global_mqtt_client->subscribe_json(
      this->mqtt_bridge_topic_(MQTT_TOPIC_COMMAND_LEARNED_TRAJECTORY_RANGE_SET),
      [this](const std::string &topic, JsonObject root) { this->handle_learned_trajectory_range_set_mqtt_command_(topic, root); },
      this->mqtt_bridge_qos_);
  mqtt::global_mqtt_client->subscribe_json(
      this->mqtt_bridge_topic_(MQTT_TOPIC_COMMAND_LEARNED_TRAJECTORY_RANGE_QUERY),
      [this](const std::string &topic, JsonObject root) { this->handle_learned_trajectory_range_query_mqtt_command_(topic, root); },
      this->mqtt_bridge_qos_);
  this->mqtt_bridge_subscriptions_registered_ = true;
#endif
}

void C4004Component::update_mqtt_bridge_connection_() {
#ifdef USE_MQTT
  if (!this->mqtt_bridge_available_()) {
    this->mqtt_bridge_connected_ = false;
    return;
  }

  const bool connected = mqtt::global_mqtt_client->is_connected();
  if (connected && !this->mqtt_bridge_connected_) {
    this->publish_all_mqtt_bridge_states_(true);
  }
  this->mqtt_bridge_connected_ = connected;
#endif
}

void C4004Component::publish_all_mqtt_bridge_states_(bool force) {
  this->publish_multi_tag_config_mqtt_state_();
  this->publish_learned_trajectory_range_mqtt_state_();
  this->publish_config_file_range_mqtt_state_();
  this->publish_target_trajectory_mqtt_state_(force);
}

void C4004Component::publish_multi_tag_config_mqtt_state_() {
#ifdef USE_MQTT
  if (!this->mqtt_bridge_available_()) {
    return;
  }

  const std::string topic = this->mqtt_bridge_topic_(MQTT_TOPIC_STATE_MULTI_TAG_CONFIG);
  const std::string device_topic_prefix = this->mqtt_bridge_device_topic_prefix_();
  mqtt::global_mqtt_client->publish_json(
          topic,
          [this, &device_topic_prefix](JsonObject root) {
            root["schema"] = 1;
            root["type"] = "multi_tag_config";
            root["device_topic_prefix"] = device_topic_prefix;
            root["mqtt_key"] = this->mqtt_bridge_key_;
            root["tag_count"] = this->multi_tag_config_count_;
            root["hex"] = this->multi_tag_config_hex_;
          },
          this->mqtt_bridge_qos_, this->mqtt_bridge_retain_state_);
#endif
}

void C4004Component::publish_learned_trajectory_range_mqtt_state_() {
#ifdef USE_MQTT
  if (!this->mqtt_bridge_available_()) {
    return;
  }

  const std::string topic = this->mqtt_bridge_topic_(MQTT_TOPIC_STATE_LEARNED_TRAJECTORY_RANGE);
  const std::string device_topic_prefix = this->mqtt_bridge_device_topic_prefix_();
  mqtt::global_mqtt_client->publish_json(
          topic,
          [this, &device_topic_prefix](JsonObject root) {
            root["schema"] = 1;
            root["type"] = "learned_trajectory_range";
            root["device_topic_prefix"] = device_topic_prefix;
            root["mqtt_key"] = this->mqtt_bridge_key_;
            root["mode"] = RANGE_TRAJECTORY;
             root["learning_enabled"] = this->learned_trajectory_learning_enabled_;
             root["point_count"] = this->learned_trajectory_learning_enabled_ ? 0 : this->learned_trajectory_range_point_count_;
             if (!this->learned_trajectory_learning_enabled_) {
               root["hex"] = this->learned_trajectory_range_hex_;
             }
          },
          this->mqtt_bridge_qos_, this->mqtt_bridge_retain_state_);
#endif
}

void C4004Component::publish_config_file_range_mqtt_state_() {
#ifdef USE_MQTT
  if (!this->mqtt_bridge_available_()) {
    return;
  }

  const std::string topic = this->mqtt_bridge_topic_(MQTT_TOPIC_STATE_CONFIG_FILE_RANGE);
  const std::string device_topic_prefix = this->mqtt_bridge_device_topic_prefix_();
  mqtt::global_mqtt_client->publish_json(
          topic,
          [this, &device_topic_prefix](JsonObject root) {
            root["schema"] = 1;
            root["type"] = "config_file_range";
            root["device_topic_prefix"] = device_topic_prefix;
            root["mqtt_key"] = this->mqtt_bridge_key_;
            root["mode"] = RANGE_CONFIG_FILE;
            root["point_count"] = this->config_file_range_point_count_;
            root["hex"] = this->config_file_range_hex_;
          },
          this->mqtt_bridge_qos_, this->mqtt_bridge_retain_state_);
#endif
}

void C4004Component::publish_multi_tag_config_mqtt_result_(const char *request_id, bool ok, const char *error) {
#ifdef USE_MQTT
  if (!this->mqtt_bridge_available_()) {
    return;
  }

  const std::string topic = this->mqtt_bridge_topic_(MQTT_TOPIC_RESULT_MULTI_TAG_CONFIG_SET);
  mqtt::global_mqtt_client->publish_json(
      topic,
      [this, request_id, ok, error](JsonObject root) {
        if (request_id != nullptr && request_id[0] != '\0') {
          root["request_id"] = request_id;
        }
        root["ok"] = ok;
        if (error != nullptr && error[0] != '\0') {
          root["error"] = error;
        }
        root["tag_count"] = this->multi_tag_config_count_;
        if (ok) {
          root["hex"] = this->multi_tag_config_hex_;
        }
      },
      this->mqtt_bridge_qos_, false);
#else
  (void) request_id;
  (void) ok;
  (void) error;
#endif
}

void C4004Component::publish_config_file_range_mqtt_result_(const char *request_id, bool ok, const char *error) {
#ifdef USE_MQTT
  if (!this->mqtt_bridge_available_()) {
    return;
  }

  const std::string topic = this->mqtt_bridge_topic_(MQTT_TOPIC_RESULT_CONFIG_FILE_RANGE_SET);
  mqtt::global_mqtt_client->publish_json(
      topic,
      [this, request_id, ok, error](JsonObject root) {
        if (request_id != nullptr && request_id[0] != '\0') {
          root["request_id"] = request_id;
        }
        root["ok"] = ok;
        if (error != nullptr && error[0] != '\0') {
          root["error"] = error;
        }
        root["point_count"] = this->config_file_range_point_count_;
        if (ok) {
          root["hex"] = this->config_file_range_hex_;
        }
      },
      this->mqtt_bridge_qos_, false);
#else
  (void) request_id;
  (void) ok;
  (void) error;
#endif
}

void C4004Component::publish_learned_trajectory_range_mqtt_result_(const char *result_topic, const char *request_id,
                                                                    bool ok, const char *error, bool include_hex) {
#ifdef USE_MQTT
  if (!this->mqtt_bridge_available_() || result_topic == nullptr) {
    return;
  }
  const std::string topic = this->mqtt_bridge_topic_(result_topic);
  const std::string device_topic_prefix = this->mqtt_bridge_device_topic_prefix_();
  mqtt::global_mqtt_client->publish_json(
      topic,
      [this, &device_topic_prefix, request_id, ok, error, include_hex](JsonObject root) {
        root["schema"] = 1;
        root["type"] = "learned_trajectory_range";
        root["device_topic_prefix"] = device_topic_prefix;
        root["mqtt_key"] = this->mqtt_bridge_key_;
        if (request_id != nullptr && request_id[0] != '\0') {
          root["request_id"] = request_id;
        }
        root["ok"] = ok;
        root["learning_enabled"] = this->learned_trajectory_learning_enabled_;
        root["point_count"] = ok && !this->learned_trajectory_learning_enabled_
                                   ? this->learned_trajectory_range_point_count_
                                   : 0;
        if (error != nullptr && error[0] != '\0') {
          root["error"] = error;
        }
        if (ok && include_hex && !this->learned_trajectory_learning_enabled_) {
          root["hex"] = this->learned_trajectory_range_hex_;
        }
      },
      this->mqtt_bridge_qos_, false);
#else
  (void) result_topic;
  (void) request_id;
  (void) ok;
  (void) error;
  (void) include_hex;
#endif
}

 #ifdef USE_MQTT
void C4004Component::handle_multi_tag_config_mqtt_command_(const std::string &topic, JsonObject root) {
  (void) topic;
  const char *request_id = root["request_id"];
  const char *hex = root["hex"];
  if (hex == nullptr || hex[0] == '\0') {
    this->publish_multi_tag_config_mqtt_result_(request_id, false, "Missing hex");
    return;
  }

  std::string normalized;
  if (!this->set_multi_tag_config_hex(hex, &normalized)) {
    this->publish_multi_tag_config_mqtt_result_(request_id, false, "Failed to apply multi tag config");
    return;
  }

  this->publish_multi_tag_config_mqtt_result_(request_id, true, nullptr);
}

void C4004Component::handle_config_file_range_mqtt_command_(const std::string &topic, JsonObject root) {
  (void) topic;
  const char *request_id = root["request_id"];
  const char *hex = root["hex"];
  if (hex == nullptr || hex[0] == '\0') {
    this->publish_config_file_range_mqtt_result_(request_id, false, "Missing hex");
    return;
  }

  std::string normalized;
  if (!this->set_config_file_range_hex(hex, &normalized)) {
    this->publish_config_file_range_mqtt_result_(request_id, false, "Failed to apply config file range");
    return;
  }

  this->publish_config_file_range_mqtt_result_(request_id, true, nullptr);
}

void C4004Component::handle_learned_trajectory_range_set_mqtt_command_(const std::string &topic, JsonObject root) {
  (void) topic;
  const char *request_id = root["request_id"];
  if (!root["learning_enabled"].is<bool>()) {
    this->publish_learned_trajectory_range_mqtt_result_(MQTT_TOPIC_RESULT_LEARNED_TRAJECTORY_RANGE_SET, request_id, false,
                                                        "Missing learning_enabled");
    return;
  }
  const bool learning_enabled = root["learning_enabled"].as<bool>();
  if (!this->set_trajectory_range_mode(learning_enabled)) {
    this->publish_learned_trajectory_range_mqtt_result_(MQTT_TOPIC_RESULT_LEARNED_TRAJECTORY_RANGE_SET, request_id, false,
                                                        "Failed to set learned trajectory range mode");
    return;
  }
  this->publish_learned_trajectory_range_mqtt_result_(MQTT_TOPIC_RESULT_LEARNED_TRAJECTORY_RANGE_SET, request_id, true);
}

void C4004Component::handle_learned_trajectory_range_query_mqtt_command_(const std::string &topic, JsonObject root) {
  (void) topic;
  const char *request_id = root["request_id"];
  if (!this->query_learned_trajectory_range() || this->learned_trajectory_learning_enabled_) {
    this->publish_learned_trajectory_range_mqtt_result_(MQTT_TOPIC_RESULT_LEARNED_TRAJECTORY_RANGE_QUERY, request_id, false,
                                                        "Failed to query learned trajectory range");
    return;
  }
  this->publish_learned_trajectory_range_mqtt_result_(MQTT_TOPIC_RESULT_LEARNED_TRAJECTORY_RANGE_QUERY, request_id, true,
                                                      nullptr, true);
}
#endif

void C4004Component::publish_detection_range_mode() {
#ifdef USE_TEXT_SENSOR
  if (this->detection_range_mode_text_sensor_ != nullptr) {
    this->detection_range_mode_text_sensor_->publish_state(this->range_mode_to_string(this->range_mode_));
  }
#endif
}

void C4004Component::publish_tag_zone_state(uint8_t index, TagType tag_type) {
  if (index >= C4004_TAG_ZONE_COUNT) {
    return;
  }

  const TagZoneState &zone = this->tag_zone_states_[index];

  switch (tag_type) {
    case TAG_BOUNDARY:
#ifdef USE_TEXT_SENSOR
      if (this->zone_boundary_state_text_sensors_[index] != nullptr) {
        const char *state = "Unknown";
        if (zone.boundary_valid) {
          state = this->boundary_state_to_string(zone.boundary_state);
        }
        this->zone_boundary_state_text_sensors_[index]->publish_state(state);
      }
#endif
      break;
    case TAG_APPROACH_AWAY:
#ifdef USE_TEXT_SENSOR
      if (this->zone_approach_away_text_sensors_[index] != nullptr) {
        const char *state = "Unknown";
        if (zone.approach_away_valid) {
          state = this->approach_away_state_to_string(zone.approach_away_state);
        }
        this->zone_approach_away_text_sensors_[index]->publish_state(state);
      }
#endif
      break;
    case TAG_PEOPLE_COUNTING: {
#ifdef USE_SENSOR
      const float nan_value = std::numeric_limits<float>::quiet_NaN();
      if (this->zone_moving_count_sensors_[index] != nullptr) {
        const float value = zone.people_counting_valid ? zone.moving_count : nan_value;
        this->zone_moving_count_sensors_[index]->publish_state(value);
      }
      if (this->zone_static_count_sensors_[index] != nullptr) {
        const float value = zone.people_counting_valid ? zone.static_count : nan_value;
        this->zone_static_count_sensors_[index]->publish_state(value);
      }
#endif
      break;
    }
    default:
      break;
  }
}

void C4004Component::publish_tag_zone_states() {
  for (uint8_t i = 0; i < C4004_TAG_ZONE_COUNT; i++) {
    const TagZoneState &zone = this->tag_zone_states_[i];
    if (zone.tag_type == TAG_NONE) {
      continue;
    }
    this->publish_tag_zone_state(i, zone.tag_type);
  }
}

const char *C4004Component::install_mode_to_string(InstallMode mode) const {
  if (mode == INSTALL_TOP) {
    return "Top";
  }
  return "Side";
}

const char *C4004Component::range_mode_to_string(DetectionRangeMode mode) const {
  switch (mode) {
    case RANGE_FOUR_SIDE:
      return "Four-sided Range";
    case RANGE_TRAJECTORY:
      return "Trajectory";
    case RANGE_CONFIG_FILE:
      return "Config File";
    default:
      return "Unknown";
  }
}

const char *C4004Component::boundary_state_to_string(uint8_t enter_exit) const {
  if (enter_exit == 0) {
    return "Enter";
  }
  if (enter_exit == 1) {
    return "Exit";
  }
  if (enter_exit == 2) {
    return "None";
  }
  return "Unknown";
}

const char *C4004Component::approach_away_state_to_string(uint8_t motion_dir) const {
  if (motion_dir == 0) {
    return "Approach";
  }
  if (motion_dir == 1) {
    return "Away";
  }
  if (motion_dir == 2) {
    return "None";
  }
  return "Unknown";
}

std::string C4004Component::encode_target_trajectory_hex() const {
  uint8_t payload[C4004_TARGET_TRAJECTORY_PAYLOAD_LEN]{};
  uint8_t count = this->target_count_;
  if (count > C4004_MAX_TARGETS) {
    count = C4004_MAX_TARGETS;
  }

  payload[0] = count;
  for (uint8_t i = 0; i < count; i++) {
    const uint16_t offset = 1 + i * C4004_TARGET_RECORD_LEN;
    payload[offset] = this->targets_[i].index;
    payload[offset + 1] = this->targets_[i].kinesia;
    payload[offset + 2] = static_cast<uint8_t>(this->targets_[i].target_feature);
    this->write_sign_bit_int16(&payload[offset + 3], this->targets_[i].x);
    this->write_sign_bit_int16(&payload[offset + 5], this->targets_[i].y);
    this->write_sign_bit_int16(&payload[offset + 7], this->targets_[i].height);
    this->write_sign_bit_int16(&payload[offset + 9], this->targets_[i].speed);
  }

  const uint16_t payload_len = 1 + count * C4004_TARGET_RECORD_LEN;
  return this->encode_hex_payload(payload, payload_len);
}

}  // namespace dfrobot_c4004
}  // namespace esphome

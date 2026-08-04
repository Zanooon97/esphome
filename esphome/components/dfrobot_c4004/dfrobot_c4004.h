#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include <cstdint>
#include <cstring>
#include <string>
#ifdef USE_MQTT
#include "esphome/components/mqtt/mqtt_client.h"
#endif

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

namespace esphome {
namespace dfrobot_c4004 {

static const uint8_t C4004_MAX_TARGETS = 8;
static const uint8_t C4004_TARGET_RECORD_LEN = 11;
static const uint8_t C4004_TARGET_TRAJECTORY_PAYLOAD_LEN = 1 + C4004_MAX_TARGETS * C4004_TARGET_RECORD_LEN;
static const uint16_t C4004_TARGET_TRAJECTORY_HEX_LEN = C4004_TARGET_TRAJECTORY_PAYLOAD_LEN * 2;
static const uint8_t C4004_TAG_CONFIG_LIMIT = 32;
static const uint8_t C4004_TAG_CONFIG_RECORD_LEN = 12;
static const uint16_t C4004_TAG_MULTI_CONFIG_MAX_PAYLOAD =
    2 + C4004_TAG_CONFIG_LIMIT * C4004_TAG_CONFIG_RECORD_LEN;
static const uint8_t C4004_TAG_ZONE_COUNT = 5;
static const uint8_t C4004_ZONE_GPIO_COUNT = 6;
static const uint8_t C4004_MAX_POINTS = 150;
static const uint16_t C4004_MAX_PAYLOAD = 3 + C4004_MAX_POINTS * 4;
static const uint16_t C4004_RANGE_POINTS_HEX_LEN = C4004_MAX_PAYLOAD * 2;
static const uint8_t C4004_QUERY_DATA = 0x0F;
static const uint8_t C4004_FRAME_HEAD1 = 0x53;
static const uint8_t C4004_FRAME_HEAD2 = 0x59;
static const uint8_t C4004_FRAME_TAIL1 = 0x54;
static const uint8_t C4004_FRAME_TAIL2 = 0x43;
static const uint16_t C4004_DEFAULT_TIMEOUT_MS = 200;
static const uint16_t C4004_RESET_TIMEOUT_MS = 300;
static const uint16_t C4004_FACTORY_RESET_TIMEOUT_MS = 350;
static const uint16_t C4004_TAG_SET_TIMEOUT_MS = 400;
static const uint16_t C4004_SET_RANGE_TIMEOUT_MS = 400;
static const uint16_t C4004_TRAJECTORY_RANGE_TIMEOUT_MS = 1000;
static const uint8_t C4004_TAG_SET_WIRE_LEN = 8;
// RX ring buffer keeps incoming report frames while a command response is awaited,
// avoiding the frame loss caused by flushing the UART before every request.
static const uint16_t C4004_MAX_FRAME_SIZE = 9 + C4004_MAX_PAYLOAD;
static const uint16_t C4004_RX_RING_SIZE = C4004_MAX_FRAME_SIZE + 128;

enum ReportedEvent : uint8_t {
  EVENT_NONE = 0x00,
  EVENT_TRAJECTORY = 0x01,
  EVENT_PRESENCE = 0x02,
  EVENT_MOTION = 0x03,
  EVENT_TAG = 0x04,
  EVENT_HEARTBEAT = 0x05,
  EVENT_INIT_FINISHED = 0x06,
  EVENT_PEOPLE_COUNT = 0x07,
  EVENT_UNKNOWN = 0xFE,
  EVENT_ERROR = 0xFF,
};

enum GetDataMode : uint8_t {
  GET_DATA_ACTIVE = 0x00,
  GET_DATA_REPORT = 0x01,
};

enum InstallMode : uint8_t {
  INSTALL_UNKNOWN = 0x00,
  INSTALL_SIDE = 0x01,
  INSTALL_TOP = 0x02,
};

enum PresenceState : uint8_t {
  NO_PRESENCE = 0x00,
  PRESENCE = 0x01,
};

enum MotionState : uint8_t {
  MOTION_NONE = 0x00,
  MOTION_STATIC = 0x01,
  MOTION_ACTIVE = 0x02,
};

enum TargetFeature : uint8_t {
  TARGET_STATIC = 0x00,
  TARGET_MOTION = 0x01,
  TARGET_UNCERTAIN = 0x02,
};

enum TagType : uint8_t {
  TAG_NONE = 0x00,
  TAG_BOUNDARY = 0x01,
  TAG_APPROACH_AWAY = 0x02,
  TAG_PEOPLE_COUNTING = 0x03,
  TAG_NOISE = 0x04,
};

enum TagRangeType : uint8_t {
  CIRCLE = 0x00,
  RECTANGLE = 0x01,
};

enum TagSetStatus : uint8_t {
  TAG_SET_COMM_ERROR = 0x00,
  TAG_SET_SUCCESS = 0x01,
  TAG_SET_TRACK_COUNT_ERROR = 0x02,
  TAG_SET_ALREADY_USED = 0x03,
  TAG_SET_INDEX_OUT_OF_RANGE = 0x04,
};

enum DetectionRangeMode : uint8_t {
  RANGE_FOUR_SIDE = 0x04,
  RANGE_TRAJECTORY = 0x05,
  RANGE_CONFIG_FILE = 0x06,
  RANGE_UNKNOWN = 0xFF,
};

enum ControlCode : uint8_t {
  C4004_CTRL_SYSTEM = 0x01,
  C4004_CTRL_PRODUCT_INFO = 0x02,
  C4004_CTRL_OTA = 0x03,
  C4004_CTRL_WORK_STATUS = 0x05,
  C4004_CTRL_INSTALL_INFO = 0x06,
  C4004_CTRL_DETECTION_RANGE = 0x07,
  C4004_CTRL_PRESENCE = 0x80,
  C4004_CTRL_TRAJECTORY = 0x82,
  C4004_CTRL_FALL_DETECTION = 0x83,
  C4004_CTRL_PEOPLE_COUNT = 0x86,
};

enum CommandCode : uint8_t {
  C4004_CMD_SYSTEM_HEARTBEAT_REPORT = 0x01,
  C4004_CMD_SYSTEM_RESET = 0x02,
  C4004_CMD_SYSTEM_FACTORY_RESET = 0x03,
  C4004_CMD_SYSTEM_HEARTBEAT_QUERY = 0x80,
  C4004_CMD_PRODUCT_MODEL_QUERY = 0xA1,
  C4004_CMD_PRODUCT_ID_QUERY = 0xA2,
  C4004_CMD_PRODUCT_HARDWARE_VERSION_QUERY = 0xA3,
  C4004_CMD_PRODUCT_FIRMWARE_VERSION_QUERY = 0xA4,
  C4004_CMD_WORK_STATUS_INIT_FINISHED_REPORT = 0x01,
  C4004_CMD_WORK_STATUS_INIT_FINISHED_QUERY = 0x81,
  C4004_CMD_INSTALL_SET_ANGLE = 0x01,
  C4004_CMD_INSTALL_SET_HEIGHT = 0x02,
  C4004_CMD_INSTALL_SET_MODE = 0x06,
  C4004_CMD_INSTALL_QUERY_ANGLE = 0x81,
  C4004_CMD_INSTALL_QUERY_HEIGHT = 0x82,
  C4004_CMD_INSTALL_QUERY_MODE = 0x86,
  C4004_CMD_PRESENCE_SET_ENABLE = 0x00,
  C4004_CMD_PRESENCE_REPORT = 0x01,
  C4004_CMD_PRESENCE_MOTION_REPORT = 0x02,
  C4004_CMD_PRESENCE_QUERY_ENABLE = 0x80,
  C4004_CMD_PRESENCE_QUERY_STATE = 0x81,
  C4004_CMD_PRESENCE_QUERY_MOTION = 0x82,
  C4004_CMD_TRAJECTORY_SET_ENABLE = 0x00,
  C4004_CMD_TRAJECTORY_TARGET_REPORT = 0x02,
  C4004_CMD_TRAJECTORY_QUERY_ENABLE = 0x80,
  C4004_CMD_TRAJECTORY_QUERY_TARGET = 0x82,
  C4004_CMD_TRAJECTORY_SET_TRAJECTORY_LED = 0x0B,
  C4004_CMD_TRAJECTORY_SET_MOTION_LED = 0x0C,
  C4004_CMD_TRAJECTORY_SET_CHECK_TO_ACTIVE_FRAMES = 0x0D,
  C4004_CMD_TRAJECTORY_QUERY_TRAJECTORY_LED = 0x8B,
  C4004_CMD_TRAJECTORY_QUERY_MOTION_LED = 0x8C,
  C4004_CMD_TRAJECTORY_QUERY_CHECK_TO_ACTIVE_FRAMES = 0x8D,
  C4004_CMD_DETECTION_RANGE_QUERY_TAGS = 0x91,
  C4004_CMD_DETECTION_RANGE_SET_TAG = 0x11,
  C4004_CMD_DETECTION_RANGE_CLEAR_TAG = 0x13,
  C4004_CMD_DETECTION_RANGE_SET_TAGS_FROM_CONFIG = 0x19,
  C4004_CMD_DETECTION_RANGE_SET_RANGE = 0x1A,
  C4004_CMD_DETECTION_RANGE_QUERY_RANGE = 0x9A,
  C4004_CMD_DETECTION_RANGE_TAG_REPORT = 0x1B,
  C4004_CMD_PEOPLE_COUNT_REPORT = 0x0A,
  C4004_CMD_PEOPLE_COUNT_QUERY_COUNT = 0x8A,
  C4004_CMD_PEOPLE_COUNT_SET_REPORT_INTERVAL = 0x0B,
  C4004_CMD_PEOPLE_COUNT_QUERY_REPORT_INTERVAL = 0x8B,
  C4004_CMD_PEOPLE_COUNT_CLEAR_COUNT = 0x11,
  C4004_CMD_PEOPLE_COUNT_SET_TRAJECTORY_DISTANCE = 0x0E,
  C4004_CMD_PEOPLE_COUNT_QUERY_TRAJECTORY_DISTANCE = 0x8E,
  C4004_CMD_PEOPLE_COUNT_SET_TRAJECTORY_HOLD_TIME = 0x15,
  C4004_CMD_PEOPLE_COUNT_QUERY_TRAJECTORY_HOLD_TIME = 0x95,
  C4004_CMD_PEOPLE_COUNT_SET_NO_PERSON_DELAY = 0x17,
  C4004_CMD_PEOPLE_COUNT_QUERY_NO_PERSON_DELAY = 0x97,
};

struct InstallInfo {
  InstallMode mode{INSTALL_SIDE};
  uint16_t height_cm{180};
  int16_t x_angle{0};
  int16_t y_angle{0};
  int16_t z_angle{0};
};

struct TargetInfo {
  uint8_t index{0};
  uint8_t kinesia{0};
  TargetFeature target_feature{TARGET_STATIC};
  int16_t x{0};
  int16_t y{0};
  int16_t height{0};
  int16_t speed{0};
};

struct TagConfig {
  uint8_t tag_index{0};
  TagType tag_type{TAG_NONE};
  TagRangeType scope_type{RECTANGLE};
  uint8_t io_index{0};
  int16_t center_x{0};
  int16_t center_y{0};
  uint16_t width{0};
  uint16_t height{0};
};

struct TagInfo {
  uint8_t tag_index{0};
  TagType tag_type{TAG_NONE};
  uint8_t io_index{0};
  int16_t center_x{0};
  int16_t center_y{0};
  uint8_t enter_exit{0};
  uint8_t motion_dir{0};
  uint8_t motion_num{0};
  uint8_t static_num{0};
};

struct TagZoneState {
  TagType tag_type{TAG_NONE};
  bool boundary_valid{false};
  uint8_t boundary_state{0};
  bool approach_away_valid{false};
  uint8_t approach_away_state{0};
  bool people_counting_valid{false};
  uint8_t moving_count{0};
  uint8_t static_count{0};
};

struct Point {
  int16_t x{0};
  int16_t y{0};
};

struct FourSidedRange {
  int16_t x_max{300};
  int16_t x_min{-300};
  int16_t y_max{500};
  int16_t y_min{0};
};

struct Packet {
  uint8_t control{0};
  uint8_t cmd{0};
  uint16_t len{0};
  uint8_t data[C4004_MAX_PAYLOAD]{};
};

enum RxAsmState : uint8_t {
  RX_ASM_SYNC_H1 = 0,
  RX_ASM_SYNC_H2,
  RX_ASM_CTRL,
  RX_ASM_CMD,
  RX_ASM_LEN_HI,
  RX_ASM_LEN_LO,
  RX_ASM_PAYLOAD,
  RX_ASM_CHECKSUM,
  RX_ASM_TAIL1,
  RX_ASM_TAIL2,
};

class C4004Component : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

#ifdef USE_BINARY_SENSOR
  void set_online_binary_sensor(binary_sensor::BinarySensor *sensor) { this->online_binary_sensor_ = sensor; }
  void set_presence_binary_sensor(binary_sensor::BinarySensor *sensor) { this->presence_binary_sensor_ = sensor; }
  void set_zone_presence_binary_sensor(uint8_t index, binary_sensor::BinarySensor *sensor) {
    if (index < C4004_ZONE_GPIO_COUNT) {
      this->zone_presence_binary_sensors_[index] = sensor;
    }
  }
#endif
#ifdef USE_SENSOR
  void set_live_count_sensor(sensor::Sensor *sensor) { this->live_count_sensor_ = sensor; }
  void set_target_count_sensor(sensor::Sensor *sensor) { this->target_count_sensor_ = sensor; }
  void set_motion_state_sensor(sensor::Sensor *sensor) { this->motion_state_sensor_ = sensor; }
  void set_zone_moving_count_sensor(uint8_t index, sensor::Sensor *sensor) {
    if (index < C4004_TAG_ZONE_COUNT) {
      this->zone_moving_count_sensors_[index] = sensor;
    }
  }
  void set_zone_static_count_sensor(uint8_t index, sensor::Sensor *sensor) {
    if (index < C4004_TAG_ZONE_COUNT) {
      this->zone_static_count_sensors_[index] = sensor;
    }
  }
#endif
#ifdef USE_TEXT_SENSOR
  void set_detection_range_mode_text_sensor(text_sensor::TextSensor *sensor) {
    this->detection_range_mode_text_sensor_ = sensor;
  }
  void set_zone_boundary_state_text_sensor(uint8_t index, text_sensor::TextSensor *sensor) {
    if (index < C4004_TAG_ZONE_COUNT) {
      this->zone_boundary_state_text_sensors_[index] = sensor;
    }
  }
  void set_zone_approach_away_text_sensor(uint8_t index, text_sensor::TextSensor *sensor) {
    if (index < C4004_TAG_ZONE_COUNT) {
      this->zone_approach_away_text_sensors_[index] = sensor;
    }
  }
#endif
#ifdef USE_SWITCH
  void set_presence_enable_switch(switch_::Switch *sw) { this->presence_enable_switch_ = sw; }
  void set_trajectory_track_enable_switch(switch_::Switch *sw) { this->trajectory_track_enable_switch_ = sw; }
  void set_trk_led_switch(switch_::Switch *sw) { this->trk_led_switch_ = sw; }
  void set_occ_led_switch(switch_::Switch *sw) { this->occ_led_switch_ = sw; }
  void set_trajectory_range_mode_switch(switch_::Switch *sw) { this->trajectory_range_mode_switch_ = sw; }
#endif
#ifdef USE_SELECT
  void set_install_mode_select(select::Select *select) { this->install_mode_select_ = select; }
#endif
#ifdef USE_NUMBER
  void set_install_height_number(number::Number *number) { this->install_height_number_ = number; }
  void set_install_z_angle_number(number::Number *number) { this->install_z_angle_number_ = number; }
  void set_range_x_max_number(number::Number *number) { this->range_x_max_number_ = number; }
  void set_range_x_min_number(number::Number *number) { this->range_x_min_number_ = number; }
  void set_range_y_max_number(number::Number *number) { this->range_y_max_number_ = number; }
  void set_range_y_min_number(number::Number *number) { this->range_y_min_number_ = number; }
  void set_real_time_report_interval_number(number::Number *number) { this->real_time_report_interval_number_ = number; }
  void set_trajectory_generation_distance_number(number::Number *number) {
    this->trajectory_generation_distance_number_ = number;
  }
  void set_trajectory_lifetime_number(number::Number *number) { this->trajectory_lifetime_number_ = number; }
  void set_unoccupied_time_number(number::Number *number) { this->unoccupied_time_number_ = number; }
  void set_frame_generate_count_number(number::Number *number) { this->frame_generate_count_number_ = number; }
  void set_zone_mcu_io_number(uint8_t index, number::Number *number) {
    if (index < C4004_ZONE_GPIO_COUNT) {
      this->zone_mcu_io_numbers_[index] = number;
    }
  }
#endif

  void set_pending_install_mode(const std::string &value);
  void set_pending_install_height(float value);
  void set_pending_install_z_angle(float value);
  void set_pending_range_x_max(float value);
  void set_pending_range_x_min(float value);
  void set_pending_range_y_max(float value);
  void set_pending_range_y_min(float value);
  void set_zone_gpio_pin(uint8_t index, int16_t pin);
  void set_mqtt_bridge(const std::string &mqtt_key, const std::string &topic_prefix, uint8_t qos, uint8_t stream_qos,
                       bool retain_state);
  bool set_zone_mcu_io(uint8_t index, float value);

  bool reset();
  bool factory_reset();
  bool set_install_info();
  bool set_four_sided_range_mode();
  bool clear_all_tags();
  bool clear_live_count();
  bool set_presence_enable(bool enable);
  bool set_trajectory_track_enable(bool enable);
  bool set_trk_led(bool enable);
  bool set_occ_led(bool enable);
  bool set_trajectory_range_mode(bool learning);
  bool set_real_time_report_interval(float value);
  bool set_trajectory_generation_distance(float value);
  bool set_trajectory_lifetime(float value);
  bool set_unoccupied_time(float value);
  bool set_frame_generate_count(float value);
  bool set_multi_tag_config_hex(const std::string &value, std::string *normalized);
  bool set_config_file_range_hex(const std::string &value, std::string *normalized);
  bool query_learned_trajectory_range();
  bool query_config_file_range();

  float get_install_height() const { return this->install_info_.height_cm; }
  float get_install_z_angle() const { return this->install_info_.z_angle; }
  float get_range_x_max() const { return this->range_info_.x_max; }
  float get_range_x_min() const { return this->range_info_.x_min; }
  float get_range_y_max() const { return this->range_info_.y_max; }
  float get_range_y_min() const { return this->range_info_.y_min; }
  float get_target_count() const { return this->target_count_; }
  float get_real_time_report_interval() const { return this->real_time_report_interval_; }
  float get_trajectory_generation_distance() const { return this->trajectory_generation_distance_; }
  float get_trajectory_lifetime() const { return this->trajectory_lifetime_; }
  float get_unoccupied_time() const { return this->unoccupied_time_; }
  float get_frame_generate_count() const { return this->frame_generate_count_; }
  bool is_trajectory_range_mode() const { return this->trajectory_range_mode_; }

  void publish_target_count(bool force = false);

 protected:
  bool begin();
  bool is_init_finished();
  bool is_connected();
  bool get_heartbeat(GetDataMode mode = GET_DATA_ACTIVE);
  ReportedEvent get_reported_event(uint16_t timeout_ms = 5);
  bool get_install_info(InstallInfo *info);
  bool set_install_info(const InstallInfo &info);
  bool get_presence_enable(bool *enable);
  PresenceState get_presence_state(GetDataMode mode = GET_DATA_ACTIVE);
  MotionState get_motion_state(GetDataMode mode = GET_DATA_ACTIVE);
  bool get_trajectory_track_enable(bool *enable);
  bool set_frame_generate_count_raw(uint8_t frames);
  bool get_frame_generate_count(uint8_t *frames);
  bool get_trk_led(bool *enable);
  bool get_occ_led(bool *enable);
  uint8_t get_target_count_active();
  DetectionRangeMode get_detection_range_mode();
  bool get_four_sided_range_mode(FourSidedRange *range);
  uint8_t get_tag_configs(TagConfig *tags, uint8_t max_tags);
  uint8_t get_live_count(GetDataMode mode = GET_DATA_ACTIVE);
  bool get_real_time_report_interval(uint32_t *value);
  bool get_trajectory_generation_distance(uint32_t *value);
  bool get_trajectory_lifetime(uint32_t *value);
  bool get_unoccupied_time(uint32_t *value);

  bool set_byte(uint8_t control, uint8_t cmd, uint8_t value);
  bool query_byte(uint8_t control, uint8_t cmd, uint8_t *value);
  bool set_uint32(uint8_t control, uint8_t cmd, uint32_t value);
  bool query_uint32(uint8_t control, uint8_t cmd, uint32_t *value);
  bool decode_hex_payload(const std::string &value, uint8_t *payload, uint16_t max_payload, uint16_t *payload_len,
                          std::string *normalized);
  bool send_command(uint8_t control, uint8_t cmd, const uint8_t *data, uint16_t len);
  bool request_frame(uint8_t control, uint8_t cmd, const uint8_t *data, uint16_t len, Packet *response,
                     uint16_t timeout_ms = C4004_DEFAULT_TIMEOUT_MS);
  bool read_frame(Packet *packet, uint16_t timeout_ms);
  bool read_byte(uint8_t *value, uint16_t timeout_ms);
  void flush_input();
  void reset_rx_parser_();
  void discard_rx_ring_();
  void rx_push_byte_(uint8_t value);
  bool rx_pop_byte_(uint8_t *value);
  void feed_asm_byte_(uint8_t value);
  void pump_rx_();
  bool take_pending_frame_(Packet *packet);
  ReportedEvent handle_packet(const Packet *packet);
  ReportedEvent classify_packet(const Packet *packet) const;

  void parse_targets(const uint8_t *data, uint16_t len);
  uint8_t parse_tag_list(const uint8_t *data, uint16_t len, TagConfig *tags, uint8_t max_tags);
  void parse_tag_event(const uint8_t *data, uint16_t len);
  void parse_detection_range_payload(const uint8_t *data, uint16_t len);
  void parse_live_count(const uint8_t *data, uint16_t len);
  bool query_string(uint8_t control, uint8_t cmd, std::string *value);

  uint16_t read_uint16(const uint8_t *data) const;
  int16_t read_int16(const uint8_t *data) const;
  int16_t read_sign_bit_int16(const uint8_t *data) const;
  uint32_t read_uint32(const uint8_t *data) const;
  void write_uint16(uint8_t *data, uint16_t value) const;
  void write_int16(uint8_t *data, int16_t value) const;
  void write_sign_bit_int16(uint8_t *data, int16_t value) const;
  void write_uint32(uint8_t *data, uint32_t value) const;
  std::string encode_hex_payload(const uint8_t *payload, uint16_t payload_len) const;
  std::string encode_target_trajectory_hex() const;
  bool validate_range_points_payload(const uint8_t *payload, uint16_t payload_len, DetectionRangeMode expected_mode,
                                     std::string *normalized, uint16_t *point_count = nullptr) const;
  bool validate_trajectory_range_payload(const uint8_t *payload, uint16_t payload_len, std::string *normalized,
                                         bool *learning_enabled = nullptr, uint16_t *point_count = nullptr) const;
  bool cache_range_points_payload(const uint8_t *payload, uint16_t payload_len, DetectionRangeMode expected_mode,
                                  std::string *cache, bool *learning_enabled = nullptr,
                                  uint16_t *point_count = nullptr);
  bool verify_trajectory_range_mode_state(bool learning);
  bool rebuild_multi_tag_config_hex_from_tags(const TagConfig *tags, uint8_t count);
  void sync_device_state();
  void sync_tag_zone_cache();
  void update_tag_zone_cache(const TagInfo &tag_info);
  void publish_all_states();
  void publish_online(bool online);
  void setup_zone_gpio_pins();
  bool configure_zone_gpio_pin(uint8_t index);
  void poll_zone_gpio_pins();
  void publish_zone_presence_states();
  void publish_presence_state();
  void publish_motion_state(bool force = false);
  void publish_live_count(bool force = false);
  void publish_install_info();
  void publish_boundary_range();
  void publish_switch_states();
  void publish_people_setting_numbers();
  void publish_frame_generate_count();
  void publish_zone_mcu_io_numbers();
  void publish_detection_range_mode();
  void publish_tag_zone_state(uint8_t index, TagType tag_type);
  void publish_tag_zone_states();
  void ensure_mqtt_bridge_subscriptions_();
  void update_mqtt_bridge_connection_();
  bool mqtt_bridge_available_() const;
  std::string mqtt_bridge_topic_base_() const;
  std::string mqtt_bridge_device_topic_prefix_() const;
  std::string mqtt_bridge_topic_(const char *suffix) const;
  void publish_all_mqtt_bridge_states_(bool force = false);
  void publish_multi_tag_config_mqtt_state_();
  void publish_learned_trajectory_range_mqtt_state_();
  void publish_config_file_range_mqtt_state_();
  void publish_target_trajectory_mqtt_state_(bool force = false);
  void publish_tag_event_mqtt_state_();
  void publish_multi_tag_config_mqtt_result_(const char *request_id, bool ok, const char *error = nullptr);
  void publish_config_file_range_mqtt_result_(const char *request_id, bool ok, const char *error = nullptr);
  void publish_learned_trajectory_range_mqtt_result_(const char *result_topic, const char *request_id, bool ok,
                                                     const char *error = nullptr, bool include_hex = false);
#ifdef USE_MQTT
  void handle_multi_tag_config_mqtt_command_(const std::string &topic, JsonObject root);
  void handle_config_file_range_mqtt_command_(const std::string &topic, JsonObject root);
  void handle_learned_trajectory_range_set_mqtt_command_(const std::string &topic, JsonObject root);
  void handle_learned_trajectory_range_query_mqtt_command_(const std::string &topic, JsonObject root);
#endif
  const char *install_mode_to_string(InstallMode mode) const;
  const char *range_mode_to_string(DetectionRangeMode mode) const;
  const char *boundary_state_to_string(uint8_t enter_exit) const;
  const char *approach_away_state_to_string(uint8_t motion_dir) const;

  bool heartbeat_{false};
  bool init_finished_{false};
  bool connected_{false};
  bool presence_enable_{true};
  bool trajectory_track_enable_{true};
  bool trk_led_{true};
  bool occ_led_{true};
  bool trajectory_range_mode_{false};
  uint32_t last_active_query_ms_{0};
  uint32_t last_heartbeat_query_ms_{0};
  uint8_t active_query_step_{0};
  InstallInfo install_info_{};
  FourSidedRange range_info_{};
  DetectionRangeMode range_mode_{RANGE_UNKNOWN};
  PresenceState presence_state_{NO_PRESENCE};
  MotionState motion_state_{MOTION_NONE};
  Packet rx_packet_{};
  uint8_t rx_ring_[C4004_RX_RING_SIZE]{};
  uint16_t rx_head_{0};
  uint16_t rx_tail_{0};
  RxAsmState asm_state_{RX_ASM_SYNC_H1};
  uint8_t asm_checksum_{0};
  uint16_t asm_idx_{0};
  uint8_t asm_recv_checksum_{0};
  Packet pending_packet_{};
  bool pending_valid_{false};
  TargetInfo targets_[C4004_MAX_TARGETS]{};
  uint8_t target_count_{0};
  std::string learned_trajectory_range_hex_{"050000"};
  std::string config_file_range_hex_{"060000"};
  TagInfo tag_info_{};
  bool tag_info_valid_{false};
  TagZoneState tag_zone_states_[C4004_TAG_ZONE_COUNT]{};
  uint8_t live_count_{0};
  MotionState last_published_motion_state_{MOTION_NONE};
  uint8_t last_published_live_count_{0xFF};
  uint8_t last_published_target_count_{0xFF};
  bool motion_state_published_{false};
  bool live_count_published_{false};
  bool target_count_published_{false};
  uint8_t last_emitted_target_count_{0};
  uint32_t real_time_report_interval_{0};
  uint32_t trajectory_generation_distance_{0};
  uint32_t trajectory_lifetime_{0};
  uint32_t unoccupied_time_{0};
  uint8_t frame_generate_count_{1};
  int16_t zone_gpio_pins_[C4004_ZONE_GPIO_COUNT]{-1, -1, -1, -1, -1, -1};
  bool zone_presence_[C4004_ZONE_GPIO_COUNT]{false, false, false, false, false, false};
  uint32_t last_zone_gpio_poll_ms_{0};
  std::string multi_tag_config_hex_{};
  uint16_t multi_tag_config_count_{0};
  uint16_t learned_trajectory_range_point_count_{0};
  uint16_t config_file_range_point_count_{0};
  bool learned_trajectory_learning_enabled_{false};
  bool mqtt_bridge_enabled_{false};
  uint8_t mqtt_bridge_qos_{1};
  uint8_t mqtt_bridge_stream_qos_{0};
  bool mqtt_bridge_retain_state_{true};
  bool mqtt_bridge_connected_{false};
  bool mqtt_bridge_subscriptions_registered_{false};
  std::string mqtt_bridge_key_{};
  std::string mqtt_bridge_topic_prefix_{};

#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *online_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *presence_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *zone_presence_binary_sensors_[C4004_ZONE_GPIO_COUNT]{nullptr, nullptr, nullptr,
                                                                               nullptr, nullptr, nullptr};
#endif
#ifdef USE_SENSOR
  sensor::Sensor *live_count_sensor_{nullptr};
  sensor::Sensor *target_count_sensor_{nullptr};
  sensor::Sensor *motion_state_sensor_{nullptr};
  sensor::Sensor *zone_moving_count_sensors_[C4004_TAG_ZONE_COUNT]{nullptr, nullptr, nullptr, nullptr, nullptr};
  sensor::Sensor *zone_static_count_sensors_[C4004_TAG_ZONE_COUNT]{nullptr, nullptr, nullptr, nullptr, nullptr};
#endif
#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *detection_range_mode_text_sensor_{nullptr};
  text_sensor::TextSensor *zone_boundary_state_text_sensors_[C4004_TAG_ZONE_COUNT]{nullptr, nullptr, nullptr, nullptr,
                                                                              nullptr};
  text_sensor::TextSensor *zone_approach_away_text_sensors_[C4004_TAG_ZONE_COUNT]{nullptr, nullptr, nullptr, nullptr,
                                                                             nullptr};
#endif
#ifdef USE_SWITCH
  switch_::Switch *presence_enable_switch_{nullptr};
  switch_::Switch *trajectory_track_enable_switch_{nullptr};
  switch_::Switch *trk_led_switch_{nullptr};
  switch_::Switch *occ_led_switch_{nullptr};
  switch_::Switch *trajectory_range_mode_switch_{nullptr};
#endif
#ifdef USE_SELECT
  select::Select *install_mode_select_{nullptr};
#endif
#ifdef USE_NUMBER
  number::Number *install_height_number_{nullptr};
  number::Number *install_z_angle_number_{nullptr};
  number::Number *range_x_max_number_{nullptr};
  number::Number *range_x_min_number_{nullptr};
  number::Number *range_y_max_number_{nullptr};
  number::Number *range_y_min_number_{nullptr};
  number::Number *real_time_report_interval_number_{nullptr};
  number::Number *trajectory_generation_distance_number_{nullptr};
  number::Number *trajectory_lifetime_number_{nullptr};
  number::Number *unoccupied_time_number_{nullptr};
  number::Number *frame_generate_count_number_{nullptr};
  number::Number *zone_mcu_io_numbers_[C4004_ZONE_GPIO_COUNT]{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
#endif
};

}  // namespace dfrobot_c4004
}  // namespace esphome

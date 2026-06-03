#pragma once

#include <deque>
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif

#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif

namespace esphome::t6615 {

// Status byte bit flags (CMD_STATUS 0xB6 response)
static const uint8_t T6615_STATUS_ERROR    = 1 << 0;
static const uint8_t T6615_STATUS_WARMUP   = 1 << 1;
static const uint8_t T6615_STATUS_CAL      = 1 << 2;
static const uint8_t T6615_STATUS_IDLE     = 1 << 3;
// bits 4-6 internal
static const uint8_t T6615_STATUS_SELFTEST = 1 << 7;  // bit 7 per datasheet

// Calibration armed timeout: 5 minutes
static const uint32_t T6615_CAL_ARMED_TIMEOUT_MS = 5 * 60 * 1000;

// Self-test poll interval while test is running
static const uint32_t T6615_SELFTEST_POLL_MS = 2000;

enum class T6615Command : uint8_t {
  NONE = 0,
  // Periodic polling
  GET_PPM,
  GET_STATUS,
  // Boot-once reads
  GET_SERIAL,
  GET_FIRMWARE_VERSION,
  GET_FIRMWARE_DATE,
  GET_ELEVATION,
  GET_CAL_PPM_TARGET,
  GET_ABC,
  // Write (ACK response: FF FA 00)
  SET_ELEVATION,
  SET_CAL_PPM_TARGET,
  // ABC logic (1 data byte response: FF FA 01 <state>)
  SET_ABC_ON,
  SET_ABC_OFF,
  RESET_ABC,
  // Action commands
  WARM_RESET,
  TRIGGER_CAL,
  SET_IDLE_ON,
  SET_IDLE_OFF,
  // Self-test (ACK start, 4 data byte result)
  SELF_TEST_START,
  GET_SELF_TEST_RESULT,
};

struct T6615PendingCommand {
  T6615Command command;
  uint16_t value{0};
};

class T6615Component : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  // --- Sensors ---
#ifdef USE_SENSOR
  void set_co2_sensor(sensor::Sensor *s) { this->co2_sensor_ = s; }
  void set_elevation_sensor(sensor::Sensor *s) { this->elevation_sensor_ = s; }
#endif

  // --- Text sensors ---
#ifdef USE_TEXT_SENSOR
  void set_serial_number_text_sensor(text_sensor::TextSensor *s) { this->serial_number_ = s; }
  void set_firmware_version_text_sensor(text_sensor::TextSensor *s) { this->firmware_version_ = s; }
  void set_firmware_date_text_sensor(text_sensor::TextSensor *s) { this->firmware_date_ = s; }
  void set_selftest_result_text_sensor(text_sensor::TextSensor *s) { this->selftest_result_ = s; }
#endif

  // --- Binary sensors ---
#ifdef USE_BINARY_SENSOR
  void set_error_binary_sensor(binary_sensor::BinarySensor *s) { this->error_flag_ = s; }
  void set_warmup_binary_sensor(binary_sensor::BinarySensor *s) { this->warmup_flag_ = s; }
  void set_calibrating_binary_sensor(binary_sensor::BinarySensor *s) { this->calibrating_flag_ = s; }
  void set_selftest_running_binary_sensor(binary_sensor::BinarySensor *s) { this->selftest_running_ = s; }
#endif

  // --- Numbers ---
#ifdef USE_NUMBER
  void set_elevation_number(number::Number *n) { this->elevation_number_ = n; }
  void set_cal_ppm_number(number::Number *n) { this->cal_ppm_number_ = n; }
#endif

  // --- Switches (parent holds pointer for state feedback) ---
#ifdef USE_SWITCH
  void set_cal_armed_switch(switch_::Switch *s) { this->cal_armed_switch_ = s; }
  void set_abc_switch(switch_::Switch *s) { this->abc_switch_ = s; }
#endif

  // --- Called by sub-entity classes ---
  void queue_warm_reset();
  void queue_calibration();
  void queue_set_elevation(uint16_t feet);
  void queue_set_cal_ppm_target(uint16_t ppm);
  void queue_idle_mode(bool enable);
  void queue_self_test();
  void queue_abc_logic(bool enable);
  void set_cal_armed(bool armed);

 protected:
  void send_command_(const T6615PendingCommand &pending);
  void handle_response_(const uint8_t *buf, uint8_t total_len);
  uint8_t response_data_len_() const;
  void queue_boot_sequence_();

  T6615Command command_{T6615Command::NONE};
  uint32_t command_time_{0};
  std::deque<T6615PendingCommand> command_queue_;

  // Startup delay — sensor needs several seconds after power-up before
  // it will respond to any UART commands (per datasheet)
  bool boot_sequence_queued_{false};
  uint32_t setup_time_{0};

  // Calibration armed interlock
  bool cal_armed_{false};
  uint32_t cal_armed_time_{0};

  // Self-test state machine
  bool self_test_pending_result_{false};
  uint32_t last_self_test_poll_{0};

#ifdef USE_SENSOR
  sensor::Sensor *co2_sensor_{nullptr};
  sensor::Sensor *elevation_sensor_{nullptr};
#endif

#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *serial_number_{nullptr};
  text_sensor::TextSensor *firmware_version_{nullptr};
  text_sensor::TextSensor *firmware_date_{nullptr};
  text_sensor::TextSensor *selftest_result_{nullptr};
#endif

#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *error_flag_{nullptr};
  binary_sensor::BinarySensor *warmup_flag_{nullptr};
  binary_sensor::BinarySensor *calibrating_flag_{nullptr};
  binary_sensor::BinarySensor *selftest_running_{nullptr};
#endif

#ifdef USE_NUMBER
  number::Number *elevation_number_{nullptr};
  number::Number *cal_ppm_number_{nullptr};
#endif

#ifdef USE_SWITCH
  switch_::Switch *cal_armed_switch_{nullptr};
  switch_::Switch *abc_switch_{nullptr};
#endif
};

}  // namespace esphome::t6615

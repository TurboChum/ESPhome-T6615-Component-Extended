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

namespace esphome::t6615 {

// Status byte bit flags (0xB6 response)
static const uint8_t T6615_STATUS_ERROR    = 1 << 0;
static const uint8_t T6615_STATUS_WARMUP   = 1 << 1;
static const uint8_t T6615_STATUS_CAL      = 1 << 2;
static const uint8_t T6615_STATUS_IDLE     = 1 << 3;
static const uint8_t T6615_STATUS_SELFTEST = 1 << 4;

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
  // Write (ACK response)
  SET_ELEVATION,
  SET_CAL_PPM_TARGET,
  // Actions (ACK or no response)
  WARM_RESET,
  TRIGGER_CAL,
  SET_IDLE_ON,
  SET_IDLE_OFF,
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
#endif

  // --- Binary sensors ---
#ifdef USE_BINARY_SENSOR
  void set_error_binary_sensor(binary_sensor::BinarySensor *s) { this->error_flag_ = s; }
  void set_warmup_binary_sensor(binary_sensor::BinarySensor *s) { this->warmup_flag_ = s; }
  void set_calibrating_binary_sensor(binary_sensor::BinarySensor *s) { this->calibrating_flag_ = s; }
#endif

  // --- Numbers ---
#ifdef USE_NUMBER
  void set_elevation_number(number::Number *n) { this->elevation_number_ = n; }
  void set_cal_ppm_number(number::Number *n) { this->cal_ppm_number_ = n; }
#endif

  // --- Called by sub-entity classes ---
  void queue_warm_reset();
  void queue_calibration();
  void queue_set_elevation(uint16_t feet);
  void queue_set_cal_ppm_target(uint16_t ppm);
  void queue_idle_mode(bool enable);

 protected:
  void send_command_(const T6615PendingCommand &pending);
  void handle_response_(const uint8_t *buf, uint8_t total_len);
  uint8_t response_data_len_() const;
  void queue_boot_sequence_();

  T6615Command command_{T6615Command::NONE};
  uint32_t command_time_{0};
  std::deque<T6615PendingCommand> command_queue_;

#ifdef USE_SENSOR
  sensor::Sensor *co2_sensor_{nullptr};
  sensor::Sensor *elevation_sensor_{nullptr};
#endif

#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *serial_number_{nullptr};
  text_sensor::TextSensor *firmware_version_{nullptr};
  text_sensor::TextSensor *firmware_date_{nullptr};
#endif

#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *error_flag_{nullptr};
  binary_sensor::BinarySensor *warmup_flag_{nullptr};
  binary_sensor::BinarySensor *calibrating_flag_{nullptr};
#endif

#ifdef USE_NUMBER
  number::Number *elevation_number_{nullptr};
  number::Number *cal_ppm_number_{nullptr};
#endif
};

}  // namespace esphome::t6615

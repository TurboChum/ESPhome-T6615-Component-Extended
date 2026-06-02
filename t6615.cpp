#include "t6615.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::t6615 {

static const char *const TAG = "t6615";

static const uint32_t T6615_TIMEOUT      = 1000;
static const uint8_t  T6615_MAGIC        = 0xFF;
static const uint8_t  T6615_ADDR_HOST    = 0xFA;
static const uint8_t  T6615_ADDR_SENSOR  = 0xFE;

// ---------------------------------------------------------------------------
// Boot / update
// ---------------------------------------------------------------------------

void T6615Component::setup() {
  this->queue_boot_sequence_();
}

void T6615Component::queue_boot_sequence_() {
  this->command_queue_.push_back({T6615Command::GET_SERIAL});
  this->command_queue_.push_back({T6615Command::GET_FIRMWARE_VERSION});
  this->command_queue_.push_back({T6615Command::GET_FIRMWARE_DATE});
  this->command_queue_.push_back({T6615Command::GET_ELEVATION});
  this->command_queue_.push_back({T6615Command::GET_STATUS});
}

void T6615Component::update() {
#ifdef USE_SENSOR
  if (this->co2_sensor_ != nullptr)
    this->command_queue_.push_back({T6615Command::GET_PPM});
#endif
  this->command_queue_.push_back({T6615Command::GET_STATUS});
}

// ---------------------------------------------------------------------------
// Main loop — non-blocking state machine
// ---------------------------------------------------------------------------

void T6615Component::loop() {
  // Dispatch next queued command when idle
  if (this->command_ == T6615Command::NONE) {
    if (!this->command_queue_.empty()) {
      auto next = this->command_queue_.front();
      this->command_queue_.pop_front();
      this->send_command_(next);
    }
    return;
  }

  // Timeout: swallow stale bytes and move on
  if (millis() - this->command_time_ > T6615_TIMEOUT) {
    ESP_LOGW(TAG, "Timeout on command %u, flushing buffer", (uint8_t) this->command_);
    while (this->available())
      this->read();
    this->command_ = T6615Command::NONE;
    this->status_set_warning();
    return;
  }

  // Wait until the full expected response is buffered
  uint8_t expected = 3 + this->response_data_len_();
  if (this->available() < (int) expected)
    return;

  uint8_t buf[20] = {};
  this->read_array(buf, expected);

  if (buf[0] != T6615_MAGIC || buf[1] != T6615_ADDR_HOST) {
    ESP_LOGW(TAG, "Bad response header: %02X %02X", buf[0], buf[1]);
    while (this->available())
      this->read();
    this->command_ = T6615Command::NONE;
    this->status_set_warning();
    return;
  }

  this->status_clear_warning();
  this->handle_response_(buf, expected);
  this->command_ = T6615Command::NONE;
  this->command_time_ = 0;
}

// ---------------------------------------------------------------------------
// Send
// ---------------------------------------------------------------------------

void T6615Component::send_command_(const T6615PendingCommand &pending) {
  this->write_byte(T6615_MAGIC);
  this->write_byte(T6615_ADDR_SENSOR);

  switch (pending.command) {
    // READ commands: payload = {0x02, <cmd_byte>}
    case T6615Command::GET_PPM:
      this->write_byte(2); this->write_byte(0x02); this->write_byte(0x03);
      break;
    case T6615Command::GET_SERIAL:
      this->write_byte(2); this->write_byte(0x02); this->write_byte(0x01);
      break;
    case T6615Command::GET_FIRMWARE_VERSION:
      this->write_byte(2); this->write_byte(0x02); this->write_byte(0x0D);
      break;
    case T6615Command::GET_FIRMWARE_DATE:
      this->write_byte(2); this->write_byte(0x02); this->write_byte(0x0C);
      break;
    case T6615Command::GET_ELEVATION:
      this->write_byte(2); this->write_byte(0x02); this->write_byte(0x0F);
      break;
    case T6615Command::GET_CAL_PPM_TARGET:
      this->write_byte(2); this->write_byte(0x02); this->write_byte(0x11);
      break;

    // STATUS: single-byte command
    case T6615Command::GET_STATUS:
      this->write_byte(1); this->write_byte(0xB6);
      break;

    // UPDATE commands: payload = {0x03, <cmd_byte>, <msb>, <lsb>}
    case T6615Command::SET_ELEVATION: {
      uint8_t msb = (pending.value >> 8) & 0xFF;
      uint8_t lsb = pending.value & 0xFF;
      this->write_byte(4);
      this->write_byte(0x03); this->write_byte(0x0F);
      this->write_byte(msb);  this->write_byte(lsb);
      break;
    }
    case T6615Command::SET_CAL_PPM_TARGET: {
      uint8_t msb = (pending.value >> 8) & 0xFF;
      uint8_t lsb = pending.value & 0xFF;
      this->write_byte(4);
      this->write_byte(0x03); this->write_byte(0x11);
      this->write_byte(msb);  this->write_byte(lsb);
      break;
    }

    // ACTION commands: single or two-byte
    case T6615Command::WARM_RESET:
      this->write_byte(1); this->write_byte(0x84);
      // No response from sensor after reset; don't wait.
      this->queue_boot_sequence_();
      this->command_ = T6615Command::NONE;
      return;

    case T6615Command::TRIGGER_CAL:
      this->write_byte(1); this->write_byte(0x9B);
      break;

    case T6615Command::SET_IDLE_ON:
      this->write_byte(2); this->write_byte(0xB9); this->write_byte(0x01);
      break;

    case T6615Command::SET_IDLE_OFF:
      this->write_byte(2); this->write_byte(0xB9); this->write_byte(0x02);
      break;

    default:
      this->command_ = T6615Command::NONE;
      return;
  }

  this->command_ = pending.command;
  this->command_time_ = millis();
}

// ---------------------------------------------------------------------------
// Expected response payload size (bytes after the 3-byte FF FA <len> header)
// ---------------------------------------------------------------------------

uint8_t T6615Component::response_data_len_() const {
  switch (this->command_) {
    case T6615Command::GET_SERIAL:            return 15;
    case T6615Command::GET_FIRMWARE_VERSION:  return 3;
    case T6615Command::GET_FIRMWARE_DATE:     return 6;
    case T6615Command::GET_STATUS:            return 1;
    default:                                  return 2;  // PPM, elevation, ACKs
  }
}

// ---------------------------------------------------------------------------
// Response parsing
// ---------------------------------------------------------------------------

void T6615Component::handle_response_(const uint8_t *buf, uint8_t /*total_len*/) {
  // Data bytes begin at buf[3]; buf[2] is the sensor-reported length
  switch (this->command_) {

    case T6615Command::GET_PPM: {
#ifdef USE_SENSOR
      uint16_t ppm = encode_uint16(buf[3], buf[4]);
      ESP_LOGD(TAG, "CO₂=%uppm", ppm);
      if (this->co2_sensor_ != nullptr)
        this->co2_sensor_->publish_state(ppm);
#endif
      break;
    }

    case T6615Command::GET_STATUS: {
      uint8_t status = buf[3];
      ESP_LOGD(TAG, "Status=0x%02X", status);
#ifdef USE_BINARY_SENSOR
      if (this->error_flag_ != nullptr)
        this->error_flag_->publish_state(status & T6615_STATUS_ERROR);
      if (this->warmup_flag_ != nullptr)
        this->warmup_flag_->publish_state(status & T6615_STATUS_WARMUP);
      if (this->calibrating_flag_ != nullptr)
        this->calibrating_flag_->publish_state(status & T6615_STATUS_CAL);
#endif
      break;
    }

    case T6615Command::GET_SERIAL: {
#ifdef USE_TEXT_SENSOR
      if (this->serial_number_ != nullptr) {
        std::string s(reinterpret_cast<const char *>(buf + 3), 15);
        ESP_LOGD(TAG, "Serial=%s", s.c_str());
        this->serial_number_->publish_state(s);
      }
#endif
      break;
    }

    case T6615Command::GET_FIRMWARE_VERSION: {
#ifdef USE_TEXT_SENSOR
      if (this->firmware_version_ != nullptr) {
        char ver[12];
        snprintf(ver, sizeof(ver), "%u.%u.%u", buf[3], buf[4], buf[5]);
        ESP_LOGD(TAG, "Firmware version=%s", ver);
        this->firmware_version_->publish_state(ver);
      }
#endif
      break;
    }

    case T6615Command::GET_FIRMWARE_DATE: {
#ifdef USE_TEXT_SENSOR
      if (this->firmware_date_ != nullptr) {
        std::string d(reinterpret_cast<const char *>(buf + 3), 6);
        ESP_LOGD(TAG, "Firmware date=%s", d.c_str());
        this->firmware_date_->publish_state(d);
      }
#endif
      break;
    }

    case T6615Command::GET_ELEVATION: {
      uint16_t feet = encode_uint16(buf[3], buf[4]);
      ESP_LOGD(TAG, "Elevation=%uft", feet);
#ifdef USE_SENSOR
      if (this->elevation_sensor_ != nullptr)
        this->elevation_sensor_->publish_state(feet);
#endif
#ifdef USE_NUMBER
      if (this->elevation_number_ != nullptr)
        this->elevation_number_->publish_state(feet);
#endif
      break;
    }

    case T6615Command::GET_CAL_PPM_TARGET: {
      uint16_t target = encode_uint16(buf[3], buf[4]);
      ESP_LOGD(TAG, "Cal PPM target=%uppm", target);
#ifdef USE_NUMBER
      if (this->cal_ppm_number_ != nullptr)
        this->cal_ppm_number_->publish_state(target);
#endif
      break;
    }

    case T6615Command::SET_ELEVATION:
      ESP_LOGD(TAG, "SET_ELEVATION ACK");
      // Re-read to confirm and refresh the elevation sensor/number
      this->command_queue_.push_front({T6615Command::GET_ELEVATION});
      break;

    case T6615Command::SET_CAL_PPM_TARGET:
      ESP_LOGD(TAG, "SET_CAL_PPM_TARGET ACK");
      this->command_queue_.push_front({T6615Command::GET_CAL_PPM_TARGET});
      break;

    case T6615Command::TRIGGER_CAL:
      ESP_LOGI(TAG, "Single-point calibration triggered");
      break;

    case T6615Command::SET_IDLE_ON:
    case T6615Command::SET_IDLE_OFF:
      ESP_LOGD(TAG, "Idle mode ACK");
      break;

    default:
      break;
  }
}

// ---------------------------------------------------------------------------
// Public queue helpers (called by sub-entity classes)
// ---------------------------------------------------------------------------

void T6615Component::queue_warm_reset() {
  this->command_queue_.push_back({T6615Command::WARM_RESET});
}

void T6615Component::queue_calibration() {
#ifdef USE_BINARY_SENSOR
  if (this->warmup_flag_ != nullptr && this->warmup_flag_->state) {
    ESP_LOGW(TAG, "Ignoring calibration request during sensor warmup");
    return;
  }
#endif
  this->command_queue_.push_back({T6615Command::TRIGGER_CAL});
}

void T6615Component::queue_set_elevation(uint16_t feet) {
  this->command_queue_.push_back({T6615Command::SET_ELEVATION, feet});
}

void T6615Component::queue_set_cal_ppm_target(uint16_t ppm) {
  this->command_queue_.push_back({T6615Command::SET_CAL_PPM_TARGET, ppm});
}

void T6615Component::queue_idle_mode(bool enable) {
  this->command_queue_.push_back({enable ? T6615Command::SET_IDLE_ON : T6615Command::SET_IDLE_OFF});
}

// ---------------------------------------------------------------------------
// Config dump
// ---------------------------------------------------------------------------

void T6615Component::dump_config() {
  ESP_LOGCONFIG(TAG, "T6615:");
  this->check_uart_settings(19200);
#ifdef USE_SENSOR
  LOG_SENSOR("  ", "CO2", this->co2_sensor_);
  LOG_SENSOR("  ", "Elevation", this->elevation_sensor_);
#endif
#ifdef USE_TEXT_SENSOR
  LOG_TEXT_SENSOR("  ", "Serial Number", this->serial_number_);
  LOG_TEXT_SENSOR("  ", "Firmware Version", this->firmware_version_);
  LOG_TEXT_SENSOR("  ", "Firmware Date", this->firmware_date_);
#endif
#ifdef USE_BINARY_SENSOR
  LOG_BINARY_SENSOR("  ", "Error", this->error_flag_);
  LOG_BINARY_SENSOR("  ", "Warmup", this->warmup_flag_);
  LOG_BINARY_SENSOR("  ", "Calibrating", this->calibrating_flag_);
#endif
}

}  // namespace esphome::t6615

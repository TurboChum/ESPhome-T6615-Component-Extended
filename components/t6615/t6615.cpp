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
// Boot / setup
// ---------------------------------------------------------------------------

void T6615Component::setup() {
  this->setup_time_ = millis();
}

void T6615Component::queue_boot_sequence_() {
  this->command_queue_.push_back({T6615Command::GET_SERIAL});
  this->command_queue_.push_back({T6615Command::GET_FIRMWARE_VERSION});
  this->command_queue_.push_back({T6615Command::GET_FIRMWARE_DATE});
  this->command_queue_.push_back({T6615Command::GET_ELEVATION});
  this->command_queue_.push_back({T6615Command::GET_CAL_PPM_TARGET});
  this->command_queue_.push_back({T6615Command::GET_ABC});
  this->command_queue_.push_back({T6615Command::GET_STATUS});
}

// ---------------------------------------------------------------------------
// Periodic update
// ---------------------------------------------------------------------------

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
  // Delay boot sequence — sensor needs several seconds after power-up
  // before it responds to any UART commands (per datasheet)
  if (!this->boot_sequence_queued_ && (millis() - this->setup_time_ >= 8000)) {
    this->queue_boot_sequence_();
    this->boot_sequence_queued_ = true;
  }

  // Auto-disarm calibration armed switch after timeout
#ifdef USE_SWITCH
  if (this->cal_armed_ && (millis() - this->cal_armed_time_ > T6615_CAL_ARMED_TIMEOUT_MS)) {
    ESP_LOGI(TAG, "Calibration armed timeout — disarming");
    this->cal_armed_ = false;
    if (this->cal_armed_switch_ != nullptr)
      this->cal_armed_switch_->publish_state(false);
  }
#endif

  // Inject status polls while self-test is in progress
  if (this->self_test_pending_result_ && this->command_ == T6615Command::NONE &&
      (millis() - this->last_self_test_poll_ >= T6615_SELFTEST_POLL_MS)) {
    this->last_self_test_poll_ = millis();
    this->command_queue_.push_front({T6615Command::GET_STATUS});
  }

  // Dispatch next queued command when idle
  if (this->command_ == T6615Command::NONE) {
    if (!this->command_queue_.empty()) {
      auto next = this->command_queue_.front();
      this->command_queue_.pop_front();
      this->send_command_(next);
    }
    return;
  }

  // Timeout handling — behaviour depends on command
  if (millis() - this->command_time_ > T6615_TIMEOUT) {
    while (this->available())
      this->read();

    if (this->command_ == T6615Command::GET_PPM) {
      // Sensor silently drops commands during its internal DSP cycle (1-2s).
      // Per datasheet: simply re-send. Retry once immediately.
      ESP_LOGD(TAG, "GET_PPM dropped by sensor DSP cycle — retrying");
      this->send_command_({T6615Command::GET_PPM});
      return;
    }

    if (this->command_ == T6615Command::GET_ABC) {
      // T6615 uses a sealed reference channel and does not implement ABC logic.
      // This timeout is expected on T6615 hardware — not a comms error.
      ESP_LOGD(TAG, "GET_ABC not acknowledged (expected on T6615 — no ABC on dual-beam sensor)");
      this->command_ = T6615Command::NONE;
      return;
    }

    ESP_LOGW(TAG, "Timeout on command %u", (uint8_t) this->command_);
    this->command_ = T6615Command::NONE;
    this->status_set_warning();
    return;
  }

  // Wait until full expected response is buffered
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
    // READ commands — payload: {0x02, <cmd_byte>}
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

    // STATUS
    case T6615Command::GET_STATUS:
      this->write_byte(1); this->write_byte(0xB6);
      break;

    // ABC (returns 1 data byte, not ACK)
    case T6615Command::GET_ABC:
      this->write_byte(2); this->write_byte(0xB7); this->write_byte(0x00);
      break;
    case T6615Command::SET_ABC_ON:
      this->write_byte(2); this->write_byte(0xB7); this->write_byte(0x01);
      break;
    case T6615Command::SET_ABC_OFF:
      this->write_byte(2); this->write_byte(0xB7); this->write_byte(0x02);
      break;
    case T6615Command::RESET_ABC:
      this->write_byte(2); this->write_byte(0xB7); this->write_byte(0x03);
      break;

    // UPDATE commands — payload: {0x03, <cmd_byte>, <msb>, <lsb>}
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

    // ACTION commands
    case T6615Command::WARM_RESET:
      this->write_byte(1); this->write_byte(0x84);
      // Sensor may or may not ACK before resetting; treat as fire-and-forget.
      // Re-arm the startup delay so boot sequence waits for sensor to restart.
      this->boot_sequence_queued_ = false;
      this->setup_time_ = millis();
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

    // Self-test
    case T6615Command::SELF_TEST_START:
      this->write_byte(2); this->write_byte(0xC0); this->write_byte(0x00);
      break;
    case T6615Command::GET_SELF_TEST_RESULT:
      this->write_byte(2); this->write_byte(0xC0); this->write_byte(0x01);
      break;

    default:
      this->command_ = T6615Command::NONE;
      return;
  }

  this->command_ = pending.command;
  this->command_time_ = millis();
}

// ---------------------------------------------------------------------------
// Expected response data bytes (after 3-byte FF FA <len> header)
// ---------------------------------------------------------------------------

uint8_t T6615Component::response_data_len_() const {
  switch (this->command_) {
    case T6615Command::GET_SERIAL:            return 15;
    case T6615Command::GET_FIRMWARE_VERSION:  return 3;
    case T6615Command::GET_FIRMWARE_DATE:     return 6;
    case T6615Command::GET_STATUS:            return 1;
    // ABC commands return 1 data byte (0x01 or 0x02), NOT an ACK
    case T6615Command::GET_ABC:
    case T6615Command::SET_ABC_ON:
    case T6615Command::SET_ABC_OFF:
    case T6615Command::RESET_ABC:             return 1;
    // Self-test result: 4 bytes (test_flag, pga, good_dsp, total_dsp)
    case T6615Command::GET_SELF_TEST_RESULT:  return 4;
    // UPDATE ACK / action ACK: FF FA 00 (0 data bytes)
    case T6615Command::SET_ELEVATION:
    case T6615Command::SET_CAL_PPM_TARGET:
    case T6615Command::TRIGGER_CAL:
    case T6615Command::SET_IDLE_ON:
    case T6615Command::SET_IDLE_OFF:
    case T6615Command::SELF_TEST_START:       return 0;
    // PPM, elevation, cal target: 2 data bytes
    default:                                  return 2;
  }
}

// ---------------------------------------------------------------------------
// Response parsing
// ---------------------------------------------------------------------------

void T6615Component::handle_response_(const uint8_t *buf, uint8_t /*total_len*/) {
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
      // Only log when status changes or is non-zero — suppresses 0x00 spam
      if (status != this->last_status_) {
        if (status == 0x00)
          ESP_LOGD(TAG, "Status=0x00 (normal)");
        else
          ESP_LOGW(TAG, "Status=0x%02X (error=%d warmup=%d cal=%d idle=%d selftest=%d)",
                   status,
                   (bool)(status & T6615_STATUS_ERROR),
                   (bool)(status & T6615_STATUS_WARMUP),
                   (bool)(status & T6615_STATUS_CAL),
                   (bool)(status & T6615_STATUS_IDLE),
                   (bool)(status & T6615_STATUS_SELFTEST));
        this->last_status_ = status;
      }
#ifdef USE_BINARY_SENSOR
      if (this->error_flag_ != nullptr)
        this->error_flag_->publish_state(status & T6615_STATUS_ERROR);
      if (this->warmup_flag_ != nullptr)
        this->warmup_flag_->publish_state(status & T6615_STATUS_WARMUP);
      if (this->calibrating_flag_ != nullptr)
        this->calibrating_flag_->publish_state(status & T6615_STATUS_CAL);
      if (this->selftest_running_ != nullptr)
        this->selftest_running_->publish_state(status & T6615_STATUS_SELFTEST);
#endif
      // Drive self-test state machine
      if (this->self_test_pending_result_ && !(status & T6615_STATUS_SELFTEST)) {
        ESP_LOGI(TAG, "Self-test complete — reading results");
        this->self_test_pending_result_ = false;
        this->command_queue_.push_front({T6615Command::GET_SELF_TEST_RESULT});
      }
      break;
    }

    case T6615Command::GET_SERIAL: {
#ifdef USE_TEXT_SENSOR
      if (this->serial_number_ != nullptr) {
        // 15-byte null-padded ASCII — strip trailing nulls
        const char *start = reinterpret_cast<const char *>(buf + 3);
        uint8_t len = 0;
        while (len < 15 && start[len] != '\0')
          len++;
        std::string s(start, len);
        ESP_LOGD(TAG, "Serial=%s", s.c_str());
        this->serial_number_->publish_state(s);
      }
#endif
      break;
    }

    case T6615Command::GET_FIRMWARE_VERSION: {
#ifdef USE_TEXT_SENSOR
      if (this->firmware_version_ != nullptr) {
        // 3-byte ASCII string e.g. "A15"
        std::string ver(reinterpret_cast<const char *>(buf + 3), 3);
        ESP_LOGD(TAG, "Firmware version (COMPILE_SUBVOL)=%s", ver.c_str());
        this->firmware_version_->publish_state(ver);
      }
#endif
      break;
    }

    case T6615Command::GET_FIRMWARE_DATE: {
#ifdef USE_TEXT_SENSOR
      if (this->firmware_date_ != nullptr) {
        // 6-byte ASCII date e.g. "110713" = Nov 7, 2013
        std::string d(reinterpret_cast<const char *>(buf + 3), 6);
        ESP_LOGD(TAG, "Firmware date (COMPILE_DATE)=%s", d.c_str());
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

    case T6615Command::GET_ABC:
    case T6615Command::SET_ABC_ON:
    case T6615Command::SET_ABC_OFF:
    case T6615Command::RESET_ABC: {
      // 1 data byte: 0x01 = ON, 0x02 = OFF
      bool abc_on = (buf[3] == 0x01);
      ESP_LOGD(TAG, "ABC logic=%s", abc_on ? "ON" : "OFF");
#ifdef USE_SWITCH
      if (this->abc_switch_ != nullptr)
        this->abc_switch_->publish_state(abc_on);
#endif
      break;
    }

    case T6615Command::SET_ELEVATION:
      ESP_LOGD(TAG, "SET_ELEVATION ACK");
      this->command_queue_.push_front({T6615Command::GET_ELEVATION});
      break;

    case T6615Command::SET_CAL_PPM_TARGET:
      ESP_LOGD(TAG, "SET_CAL_PPM_TARGET ACK");
      this->command_queue_.push_front({T6615Command::GET_CAL_PPM_TARGET});
      break;

    case T6615Command::TRIGGER_CAL:
      ESP_LOGI(TAG, "Single-point calibration triggered");
      // Auto-disarm after triggering
#ifdef USE_SWITCH
      this->cal_armed_ = false;
      if (this->cal_armed_switch_ != nullptr)
        this->cal_armed_switch_->publish_state(false);
#endif
      break;

    case T6615Command::SELF_TEST_START:
      ESP_LOGI(TAG, "Self-test started — polling status for completion");
      this->self_test_pending_result_ = true;
      this->last_self_test_poll_ = millis();
      break;

    case T6615Command::GET_SELF_TEST_RESULT: {
      // buf[3]=test_flag buf[4]=pga_status buf[5]=good_dsp buf[6]=total_dsp
      uint8_t test_flag  = buf[3];
      uint8_t pga_status = buf[4];
      uint8_t good_dsp   = buf[5];
      uint8_t total_dsp  = buf[6];
      bool complete = (test_flag == 0x0F);
      bool pga_pass = (pga_status == 0x01);
      ESP_LOGI(TAG, "Self-test: flag=0x%02X PGA=%s DSP=%u/%u",
               test_flag, pga_pass ? "PASS" : "FAIL", good_dsp, total_dsp);
#ifdef USE_TEXT_SENSOR
      if (this->selftest_result_ != nullptr) {
        char result[48];
        if (complete)
          snprintf(result, sizeof(result), "PGA:%s DSP:%u/%u",
                   pga_pass ? "PASS" : "FAIL", good_dsp, total_dsp);
        else
          snprintf(result, sizeof(result), "INCOMPLETE (flag=0x%02X)", test_flag);
        this->selftest_result_->publish_state(result);
      }
#endif
      break;
    }

    case T6615Command::SET_IDLE_ON:
    case T6615Command::SET_IDLE_OFF:
      ESP_LOGD(TAG, "Idle mode ACK");
      break;

    default:
      break;
  }
}

// ---------------------------------------------------------------------------
// Public queue helpers
// ---------------------------------------------------------------------------

void T6615Component::queue_warm_reset() {
  this->command_queue_.push_back({T6615Command::WARM_RESET});
}

void T6615Component::queue_calibration() {
  if (!this->cal_armed_) {
    ESP_LOGW(TAG, "Calibration blocked — arm the calibration switch first");
    return;
  }
#ifdef USE_BINARY_SENSOR
  if (this->warmup_flag_ != nullptr && this->warmup_flag_->state) {
    ESP_LOGW(TAG, "Calibration blocked — sensor is in warmup");
    return;
  }
  if (this->error_flag_ != nullptr && this->error_flag_->state) {
    ESP_LOGW(TAG, "Calibration blocked — sensor is in error state");
    return;
  }
#endif
  // Verify the target PPM first, then trigger
  this->command_queue_.push_back({T6615Command::GET_CAL_PPM_TARGET});
  this->command_queue_.push_back({T6615Command::TRIGGER_CAL});
}

void T6615Component::set_cal_armed(bool armed) {
  this->cal_armed_ = armed;
  if (armed) {
    this->cal_armed_time_ = millis();
    ESP_LOGI(TAG, "Calibration armed — will auto-disarm in 5 minutes");
  } else {
    ESP_LOGI(TAG, "Calibration disarmed");
  }
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

void T6615Component::queue_self_test() {
  if (this->self_test_pending_result_) {
    ESP_LOGW(TAG, "Self-test already in progress");
    return;
  }
  this->command_queue_.push_back({T6615Command::SELF_TEST_START});
}

void T6615Component::queue_abc_logic(bool enable) {
  this->command_queue_.push_back({enable ? T6615Command::SET_ABC_ON : T6615Command::SET_ABC_OFF});
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
  LOG_TEXT_SENSOR("  ", "Self-test Result", this->selftest_result_);
#endif
#ifdef USE_BINARY_SENSOR
  LOG_BINARY_SENSOR("  ", "Error", this->error_flag_);
  LOG_BINARY_SENSOR("  ", "Warmup", this->warmup_flag_);
  LOG_BINARY_SENSOR("  ", "Calibrating", this->calibrating_flag_);
  LOG_BINARY_SENSOR("  ", "Self-test Running", this->selftest_running_);
#endif
}

}  // namespace esphome::t6615

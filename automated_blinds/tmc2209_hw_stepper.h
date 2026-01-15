#pragma once

#include <Arduino.h>
#include <TMCStepper.h>
#include <functional>

// Hardware timer-based stepper with StallGuard support
// Uses ESP32 timer interrupt for precise step timing

#define DRIVER_ADDRESS 0b00
#define R_SENSE 0.11f

class TMC2209HWStepper {
 public:
  // Current position (public for easy ESPHome access)
  int32_t current_position = 0;

  TMC2209HWStepper(uint8_t step_pin, uint8_t dir_pin, uint8_t en_pin, uint8_t diag_pin,
                   uint8_t uart_rx, uint8_t uart_tx)
      : step_pin_(step_pin), dir_pin_(dir_pin), en_pin_(en_pin), diag_pin_(diag_pin),
        uart_rx_(uart_rx), uart_tx_(uart_tx),
        driver_(&Serial2, R_SENSE, DRIVER_ADDRESS) {}

  void setup() {
    // Setup pins
    pinMode(step_pin_, OUTPUT);
    pinMode(dir_pin_, OUTPUT);
    pinMode(en_pin_, OUTPUT);
    pinMode(diag_pin_, INPUT_PULLDOWN);

    digitalWrite(step_pin_, LOW);
    digitalWrite(en_pin_, LOW);  // Enable temporarily for configuration

    // Setup UART for TMC2209
    Serial2.begin(115200, SERIAL_8N1, uart_rx_, uart_tx_);
    delay(100);

    // Check driver communication
    uint8_t version = driver_.version();
    if (version != 0x21) {
      ESP_LOGE("tmc_hw", "TMC2209 not found! Version: 0x%02X", version);
      return;
    }
    ESP_LOGI("tmc_hw", "TMC2209 found, version: 0x%02X", version);

    // Configure driver - match working Arduino settings exactly
    driver_.begin();
    driver_.toff(4);
    driver_.blank_time(24);
    driver_.rms_current(600);  // Start lower for stall detection
    driver_.microsteps(16);

    // StealthChop mode (required for StallGuard on TMC2209)
    driver_.en_spreadCycle(false);
    driver_.pwm_autoscale(true);

    // StallGuard configuration
    driver_.SGTHRS(100);
    driver_.TCOOLTHRS(0xFFFFF);  // Enable StallGuard at all velocities
    driver_.TPWMTHRS(0);

    ESP_LOGI("tmc_hw", "StallGuard configured: SGTHRS=100, TCOOLTHRS=max");

    // Disable driver after configuration - will enable when movement starts
    digitalWrite(en_pin_, HIGH);
    ESP_LOGI("tmc_hw", "Driver disabled for power saving - will enable on movement");

    // Setup hardware timer
    // New ESP32 Arduino 3.x API: timerBegin(frequency_hz)
    // 1MHz = 1µs resolution
    timer_ = timerBegin(1000000);
    timerAttachInterrupt(timer_, &TMC2209HWStepper::onTimerStatic);
    // Don't start timer yet - will start when movement requested
    timerStop(timer_);

    instance_ = this;
    driver_ok_ = true;
  }

  void loop() {
    // Check for stall flag set by ISR
    if (stall_detected_) {
      stall_detected_ = false;
      // Read actual SG_RESULT now (not from ISR)
      uint16_t sg_now = driver_.SG_RESULT();
      bool diag_now = digitalRead(diag_pin_);
      ESP_LOGW("tmc_hw", "STALL DETECTED after %d steps! DIAG=%d SG_RESULT=%d", steps_since_start_, diag_now, sg_now);
      stopTimer();
      is_running_ = false;
      target_position_ = current_position_;
      disable();  // Cut power after stall
      has_stalled_ = true;
      if (on_stall_callback_) {
        on_stall_callback_();
      }
    }

    // Periodic SG_RESULT logging during movement
    static uint32_t last_sg_log = 0;
    if (is_running_ && (millis() - last_sg_log > 200)) {
      last_sg_log = millis();
      uint16_t sg = driver_.SG_RESULT();
      bool diag = digitalRead(diag_pin_);
      ESP_LOGI("tmc_hw", "Running: step=%d DIAG=%d SG=%d", steps_since_start_, diag, sg);
    }

    // Update current position from ISR (for external access)
    current_position = current_position_;

    // Check if we've reached target
    if (is_running_ && current_position_ == target_position_) {
      stopTimer();
      is_running_ = false;
      disable();  // Cut power to motor when idle
      ESP_LOGD("tmc_hw", "Reached target position: %d", current_position_);
    }
  }

  void set_target(int32_t target) {
    target_position_ = target;

    if (target_position_ == current_position_) {
      return;
    }

    // Enable driver before movement
    enable();

    // Set direction (inverted: positive steps = DIR LOW)
    direction_ = (target_position_ > current_position_) ? 1 : -1;
    digitalWrite(dir_pin_, direction_ > 0 ? LOW : HIGH);

    // Debug: Check DIAG state BEFORE movement
    bool diag_before = digitalRead(diag_pin_);
    uint16_t sg_before = driver_.SG_RESULT();
    ESP_LOGW("tmc_hw", "Before move: DIAG=%d, SG_RESULT=%d, SGTHRS=%d", diag_before, sg_before, sgthrs_);

    // Reset stall state and step counter for warmup
    has_stalled_ = false;
    stall_detected_ = false;
    steps_since_start_ = 0;

    // Start stepping
    is_running_ = true;
    startTimer();

    ESP_LOGD("tmc_hw", "Moving from %d to %d (dir=%d)", current_position_, target_position_, direction_);
  }

  void stop() {
    stopTimer();
    is_running_ = false;
    target_position_ = current_position_;
    disable();  // Cut power to motor
  }

  void report_position(int32_t pos) {
    current_position_ = pos;
    current_position = pos;
  }

  bool is_running() { return is_running_; }
  bool has_stalled() { return has_stalled_; }

  // Power management - disable driver when not moving to save power
  void enable() {
    digitalWrite(en_pin_, LOW);  // EN is active LOW
    delay(1);  // Brief delay for driver to wake up
    ESP_LOGD("tmc_hw", "Driver ENABLED");
  }

  void disable() {
    digitalWrite(en_pin_, HIGH);  // EN is active LOW, so HIGH = disabled
    ESP_LOGD("tmc_hw", "Driver DISABLED (power saving)");
  }

  void enable_stall_detection(bool enable) {
    stall_detection_enabled_ = enable;
    ESP_LOGI("tmc_hw", "Stall detection %s", enable ? "ENABLED" : "DISABLED");
  }

  // Configuration methods
  void set_speed(uint32_t steps_per_second) {
    if (steps_per_second == 0) steps_per_second = 1;
    // Timer interval in microseconds for each half-step
    // Full step = 2 half-steps, so divide by 2
    step_interval_us_ = 1000000 / (steps_per_second * 2);
    if (step_interval_us_ < 100) step_interval_us_ = 100;  // Min 100µs
    ESP_LOGD("tmc_hw", "Speed set to %d steps/s (interval=%dµs)", steps_per_second, step_interval_us_);
  }

  void set_current(uint16_t run_ma, uint16_t hold_ma) {
    driver_.rms_current(run_ma);
    driver_.ihold(hold_ma * 31 / run_ma);  // Scale hold relative to run
    ESP_LOGD("tmc_hw", "Current set to run=%dmA, hold=%dmA", run_ma, hold_ma);
  }

  void set_stall_threshold(uint8_t threshold) {
    driver_.SGTHRS(threshold);
    sgthrs_ = threshold;
    ESP_LOGD("tmc_hw", "SGTHRS set to %d", threshold);
  }

  void set_stall_callback(std::function<void()> callback) {
    on_stall_callback_ = callback;
  }

  // Debug methods
  uint16_t get_sg_result() {
    return driver_.SG_RESULT();
  }

  void dump_registers() {
    uint32_t gconf = driver_.GCONF();
    uint32_t tcoolthrs = driver_.TCOOLTHRS();
    uint8_t sgthrs = driver_.SGTHRS();
    uint16_t sg_result = driver_.SG_RESULT();

    ESP_LOGW("tmc_hw", "=== Register Dump ===");
    ESP_LOGW("tmc_hw", "GCONF: 0x%08X", gconf);
    ESP_LOGW("tmc_hw", "TCOOLTHRS: 0x%05X", tcoolthrs);
    ESP_LOGW("tmc_hw", "SGTHRS: %d", sgthrs);
    ESP_LOGW("tmc_hw", "SG_RESULT: %d", sg_result);
  }

 private:
  // Pin configuration
  uint8_t step_pin_;
  uint8_t dir_pin_;
  uint8_t en_pin_;
  uint8_t diag_pin_;
  uint8_t uart_rx_;
  uint8_t uart_tx_;

  // Driver
  TMC2209Stepper driver_;
  bool driver_ok_ = false;

  // Timer
  hw_timer_t* timer_ = nullptr;
  uint32_t step_interval_us_ = 600;  // Default ~833 steps/s (matches Arduino)

  // Position tracking (volatile for ISR access)
  volatile int32_t current_position_ = 0;
  volatile int32_t target_position_ = 0;
  volatile int8_t direction_ = 1;
  volatile bool is_running_ = false;
  volatile bool step_state_ = false;

  // Stall detection
  volatile bool stall_detected_ = false;
  volatile bool has_stalled_ = false;
  volatile uint16_t last_sg_result_ = 0;
  volatile uint32_t steps_since_start_ = 0;
  static const uint32_t WARMUP_STEPS = 500;  // Skip DIAG check for first 500 steps (~600ms)
  uint8_t sgthrs_ = 100;
  bool stall_detection_enabled_ = false;  // Only enable during homing
  std::function<void()> on_stall_callback_ = nullptr;

  // Static instance for ISR
  static TMC2209HWStepper* instance_;

  void startTimer() {
    // New ESP32 Arduino 3.x API
    timerAlarm(timer_, step_interval_us_, true, 0);  // value, autoreload, reload_count
    timerStart(timer_);
  }

  void stopTimer() {
    timerStop(timer_);
    digitalWrite(step_pin_, LOW);
    step_state_ = false;
  }

  // Timer ISR - called every step_interval_us_ microseconds
  static void onTimerStatic() {
    if (instance_ && instance_->is_running_) {
      // Toggle step pin
      instance_->step_state_ = !instance_->step_state_;
      digitalWrite(instance_->step_pin_, instance_->step_state_ ? HIGH : LOW);

      // On falling edge, count step and check DIAG
      if (!instance_->step_state_) {
        instance_->current_position_ += instance_->direction_;
        instance_->steps_since_start_++;

        // Only check DIAG after warmup period (let StallGuard stabilize)
        if (instance_->stall_detection_enabled_ && instance_->steps_since_start_ > WARMUP_STEPS) {
          if (digitalRead(instance_->diag_pin_)) {
            instance_->stall_detected_ = true;
            instance_->is_running_ = false;
          }
        }

        // Check if reached target
        if (instance_->current_position_ == instance_->target_position_) {
          instance_->is_running_ = false;
        }
      }
    }
  }
};

// Static instance pointer
TMC2209HWStepper* TMC2209HWStepper::instance_ = nullptr;

// Global pointer for external access (use this in ESPHome lambdas)
TMC2209HWStepper* g_stepper = nullptr;

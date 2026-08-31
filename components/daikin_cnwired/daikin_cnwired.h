#pragma once

#include <cmath>
#include <string>
#include "esphome/components/climate/climate.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "cn_wired.h"
#include "cn_wired_driver.h"
#include "daikin_state.h"
#include "daikin_converters.h"
#include "daikin_switch.h"
#include <cstring>
#include <optional>

namespace esphome {
namespace daikin_cnwired {

class DaikinCNWired : public climate::Climate, public Component {
 public:
  void set_pins(GPIOPin *rx, GPIOPin *tx) { this->rx_pin_ = rx; this->tx_pin_ = tx; }
  void set_rx_invert(bool value) { this->rx_invert_ = value; }
  void set_tx_invert(bool value) { this->tx_invert_ = value; }
  void set_auto_mode(bool value) { this->auto_mode_ = value; }
  void set_turbo_as_preset(bool value) { this->turbo_as_preset_ = value; }
  //void set_powerful_preset(const std::string &value) { this->powerful_preset_ = value; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_switch_led(switch_::Switch *sw) { this->switch_led_ = sw; }
  void set_switch_listen_only(switch_::Switch *sw) { this->switch_listen_only_ = sw; }
  void set_switch_power(switch_::Switch *sw) { this->switch_power_ = sw; }  
  void set_room_temperature_sensor(sensor::Sensor *sensor) { this->room_sensor_ = sensor; }
  void set_room_humidity_sensor(sensor::Sensor *sensor) { this->room_humidity_sensor_ = sensor; }
  void set_ac_temperature_sensor(sensor::Sensor *sensor) { this->ac_temperature_sensor_ = sensor; }

  void switch_callback(DaikinSwitch *sw, bool value, DaikinSwitchType type);

  DaikinState current_state;
  DaikinState desired_state;
  //void user_command_changed_();

 protected:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  void process_packet_(const uint8_t *packet);
  void send_command_();
  void publish_state_();

  
  bool validate_packet_(const uint8_t *packet) const;

  GPIOPin *rx_pin_{nullptr};
  GPIOPin *tx_pin_{nullptr};
  bool rx_invert_{false};
  bool tx_invert_{false};
  bool auto_mode_{false};
  bool turbo_as_preset_{false};
  CNWiredDriver driver_;

  std::string fan_mode_turbo_{"Turbo"};

  bool online_{false};

  
  uint32_t packet_count_{0};
  uint32_t bad_checksum_count_{0};
  uint32_t last_rx_ms_{0};
  uint32_t pending_tx_ms_{0};
  uint32_t last_rx_mode_time_{0}; 
  uint32_t last_rx_packet_type_{0}; 

  uint32_t last_remote_control_ms_{0}; 
  uint32_t last_remote_control_checksum_{0};

  bool pending_tx_{true};
  // Snapshot of a user command. CN_WIRED packets can arrive while the 20 ms
  // response delay is active; the incoming packet must not overwrite the
  // command that the user just requested (especially OFF).
  
  bool pending_user_command_{false};
  bool pending_user_command_sended_{false};
  bool pending_user_command_sended_after_temp_report_{false};
  uint32_t pending_user_command_time_{0};
  uint32_t pending_user_command_to_{0};
  uint32_t pending_user_command_count_{0};
  bool pending_user_command_to_confirm_{false};
  void user_command_changed_();
  void pending_changes_apply();

  std::optional<climate::ClimateMode> pending_mode_;
  std::optional<climate::ClimateFanMode> pending_fan_;
  std::optional<float> pending_target_temp_;
  std::optional<climate::ClimatePreset> pending_preset_;
  std::optional<climate::ClimateSwingMode> pending_swing_;
  std::optional<bool> pending_led_;

  void set_climate_from_states_(DaikinState &state);  
  climate::ClimateAction calculate_action_(DaikinState &state) const;

  switch_::Switch *switch_led_{nullptr};
  switch_::Switch *switch_listen_only_{nullptr};
  switch_::Switch *switch_power_{nullptr};
  
  sensor::Sensor *room_sensor_{nullptr};
  sensor::Sensor *room_humidity_sensor_{nullptr};
  sensor::Sensor *ac_temperature_sensor_{nullptr};

  
  bool use_room_sensor();
  bool room_sensor_unit_is_valid();
  float room_sensor_degc();
  float get_room_temp_offset();
  float get_effective_current_temperature();

};

}  // namespace daikin_cnwired
}  // namespace esphome

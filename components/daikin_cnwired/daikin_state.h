#pragma once

#include <cmath>
#include <string>
#include "esphome/components/climate/climate.h"
//#include "cn_wired.h"
//#include "cn_wired_driver.h"
#include "daikin_converters.h"
#include <cstring>
#include <optional>

namespace esphome {
namespace daikin_cnwired {

// https://github.com/Sonic-Amiga/ESP8266-Faikin/tree/main/Tools/Simulators/CN_WIRED

struct DaikinState {
  
  bool powerful_{false};
  bool led_{false};
  bool sleep_{false};
  bool swing_v_{false};
  bool power_{false};
  Mode mode_{Mode::INVALID};
  Fan fan_{Fan::INVALID};
  float target_temp_{NAN};
  float current_temp_{NAN};


  void set_states_from_command_packet_(const uint8_t *packet);
  bool packet_changed(const uint8_t *packet) const;
  uint8_t last_package_[CNW_PKT_LEN]{};
  bool has_last_package_{false};

  uint8_t tx_package_[CNW_PKT_LEN]{};
  bool has_tx_package_{false};

  //climate::ClimatePreset preset{climate::CLIMATE_PRESET_NONE};
  //std::string custom_fan_mode;

    bool operator==(const DaikinState &other) const {
        return powerful_ == other.powerful_ &&
                led_ == other.led_ &&
                swing_v_ == other.swing_v_ &&
                power_ == other.power_ &&
                sleep_ == other.sleep_ &&
                mode_ == other.mode_ &&
                fan_ == other.fan_ &&
                float_equal(target_temp_, other.target_temp_)
                ;
    }

  bool operator!=(const DaikinState &other) const {
    return !(*this == other);
  }

  bool is_equal(const DaikinState &other) const {
    return *this == other;
  }

  bool is_different(const DaikinState &other) const {
    return *this != other;
  }

  void copy_from(const DaikinState &other) {
    powerful_ = other.powerful_;
    led_ = other.led_;
    swing_v_ = other.swing_v_;
    power_ = other.power_;
    sleep_ = other.sleep_;
    mode_ = other.mode_;
    fan_ = other.fan_;
    target_temp_ = other.target_temp_;
    current_temp_ = other.current_temp_;
    
    has_tx_package_ = false;
  }

};

}  // namespace daikin_cnwired
}  // namespace esphome
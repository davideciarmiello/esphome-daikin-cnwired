#pragma once

#include <cmath>
#include <string>
#include "esphome/components/climate/climate.h"
#include "esphome/components/switch/switch.h"
#include "cn_wired.h"
//#include "cn_wired_driver.h"
#include <cstring>
#include <optional>

namespace esphome {
namespace daikin_cnwired {

constexpr Mode from_esphome_mode(
    climate::ClimateMode mode,
    Mode fallback
) {
  switch (mode) {
    case climate::CLIMATE_MODE_OFF:
      return fallback;

    case climate::CLIMATE_MODE_FAN_ONLY:
      return Mode::FAN;

    case climate::CLIMATE_MODE_HEAT:
      return Mode::HEAT;

    case climate::CLIMATE_MODE_COOL:
      return Mode::COOL;

    case climate::CLIMATE_MODE_DRY:
      return Mode::DRY;

    case climate::CLIMATE_MODE_AUTO:
    default:
      return Mode::AUTO;
  }
}

constexpr climate::ClimateMode to_esphome_mode(
    Mode mode,
    bool power
) {
  if (!power) {
    return climate::CLIMATE_MODE_OFF;
  }

  switch (mode) {
    case Mode::AUTO:
      return climate::CLIMATE_MODE_AUTO;

    case Mode::FAN:
      return climate::CLIMATE_MODE_FAN_ONLY;

    case Mode::HEAT:
      return climate::CLIMATE_MODE_HEAT;

    case Mode::COOL:
      return climate::CLIMATE_MODE_COOL;

    case Mode::DRY:
      return climate::CLIMATE_MODE_DRY;

    default:
      return climate::CLIMATE_MODE_AUTO;
  }
}


constexpr climate::ClimateFanMode to_esphome_fan(Fan fan) {
  switch (fan) {
    case Fan::AUTO:
      return climate::CLIMATE_FAN_AUTO;

    case Fan::LEVEL1:
      return climate::CLIMATE_FAN_LOW;

    case Fan::LEVEL3:
      return climate::CLIMATE_FAN_MEDIUM;

    case Fan::LEVEL5:
      return climate::CLIMATE_FAN_HIGH;

    case Fan::QUIET:
      return climate::CLIMATE_FAN_QUIET;

    default:
      return climate::CLIMATE_FAN_AUTO;
  }
}

constexpr Fan from_esphome_fan(climate::ClimateFanMode fan) {
  switch (fan) {
    case climate::CLIMATE_FAN_LOW:
      return Fan::LEVEL1;

    case climate::CLIMATE_FAN_MEDIUM:
      return Fan::LEVEL3;

    case climate::CLIMATE_FAN_HIGH:
      return Fan::LEVEL5;

    case climate::CLIMATE_FAN_QUIET:
      return Fan::QUIET;

    case climate::CLIMATE_FAN_AUTO:
    default:
      return Fan::AUTO;
  }
}


constexpr bool float_equal(float a, float b) {
  if (std::isnan(a) || std::isnan(b))
    return std::isnan(a) && std::isnan(b);

  return std::fabs(a - b) < 0.01f;
}



}  // namespace daikin_cnwired
}  // namespace esphome


          //const char *mode_name = "INVALID";
          //switch (this->mode_) {
          //  case Mode::FAN: mode_name = "FAN"; break;
          //  case Mode::HEAT: mode_name = "HEAT"; break;
          //  case Mode::COOL: mode_name = "COOL"; break;
          //  case Mode::AUTO: mode_name = "AUTO"; break;
          //  case Mode::DRY: mode_name = "DRY"; break;
          //  default: break;
          //}
//
          //const char *fan_name = "INVALID";
          //switch (this->fan_) {
          //  case Fan::AUTO: fan_name = "AUTO"; break;
          //  case Fan::LEVEL1: fan_name = "LEVEL1"; break;
          //  case Fan::LEVEL2: fan_name = "LEVEL2"; break;
          //  case Fan::LEVEL3: fan_name = "LEVEL3"; break;
          //  case Fan::LEVEL4: fan_name = "LEVEL4"; break;
          //  case Fan::LEVEL5: fan_name = "LEVEL5"; break;
          //  case Fan::QUIET: fan_name = "QUIET"; break;
          //  default: break;
          //}
//
          //ESP_LOGD(TAG,
          //        "RX CN_WIRED MODE: power=%s temp=%u C mode=%s(0x%02X) fan=%s(0x%02X) "
          //        "specials=0x%02X swing=%s led=%s powerful=%s",
          //        power ? "ON" : "OFF",
          //        static_cast<unsigned>(this->target_temp_),
          //        mode_name, packet[CNW_MODE_OFFSET],
          //        fan_name, packet[CNW_FAN_OFFSET],
          //        packet[CNW_SPECIALS_OFFSET],
          //        this->swing_ ? "ON" : "OFF",
          //        this->led_ ? "ON" : "OFF",
          //        this->powerful_ ? "ON" : "OFF");
      //}
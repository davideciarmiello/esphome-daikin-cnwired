#pragma once

#include <cmath>
#include <string>
//#include "esphome/components/climate/climate.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

//#include "daikin_cnwired.h"
//#include "cn_wired.h"
//#include "cn_wired_driver.h"
//#include "daikin_state.h"
//#include "daikin_converters.h"
#include <cstring>
#include <optional>

namespace esphome {
namespace daikin_cnwired {

enum DaikinSwitchType {
  SWITCH_LED = 0,
  SWITCH_LISTEN_ONLY = 1,
  SWITCH_POWER = 2,
  SWITCH_SLEEP = 3,
};

class DaikinCNWired;

class DaikinSwitch : public switch_::Switch {
 public:

  DaikinSwitch(DaikinCNWired *parent, int type)
    : parent_(parent),
      type_(static_cast<DaikinSwitchType>(type)) {}
  void write_state(bool state) override;
 protected:
  DaikinCNWired *parent_;
  DaikinSwitchType type_;
};


}  // namespace daikin_cnwired
}  // namespace esphome

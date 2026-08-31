#include <algorithm>
#include "daikin_switch.h"
#include "daikin_cnwired.h"
#include "esphome/core/log.h"
#include <vector>
#include <string>

namespace esphome {
namespace daikin_cnwired {

static const char *const TAG = "daikin_cnwired.switch";

void DaikinSwitch::write_state(bool state) {
  ESP_LOGD("daikin_switch", "write_state: %s", state ? "ON" : "OFF");
  this->parent_->switch_callback(this, state, this->type_);
  this->publish_state(state);
}

}  // namespace daikin_cnwired
}  // namespace esphome

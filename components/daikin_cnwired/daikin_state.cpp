#include "daikin_state.h"

namespace esphome {
namespace daikin_cnwired {

bool DaikinState::packet_changed(const uint8_t *packet) const {
    if (this->has_last_package_ == false)
        return true;
    bool equal = std::memcmp(this->last_package_, packet, CNW_PKT_LEN) == 0;
    return equal == false;
}

void DaikinState::set_states_from_command_packet_(const uint8_t *packet) {
    const Mode new_mode = decode_mode(packet);
    if (new_mode != Mode::INVALID)
    {
        this->mode_ = new_mode;
        this->target_temp_ = decode_bcd(packet[CNW_TEMP_OFFSET]);
        this->power_ = (packet[CNW_MODE_OFFSET] & CNW_MODE_POWEROFF) == 0;
        this->powerful_ = packet[CNW_FAN_OFFSET] == CNW_FAN_POWERFUL;
        this->fan_ = decode_fan(packet);
        this->swing_v_ = (packet[CNW_SPECIALS_OFFSET] & CNW_SPECIALS_V_SWING) != 0;
        this->sleep_ = (packet[CNW_SPECIALS_OFFSET] & CNW_SPECIALS_SLEEP) != 0;
        this->led_ = (packet[CNW_SPECIALS_OFFSET] & CNW_SPECIALS_LED_ON) != 0;
        this->has_last_package_ = true;
        std::memcpy(this->last_package_, packet, CNW_PKT_LEN);
        this->has_tx_package_ = false;
    }
}


}  // namespace daikin_cnwired
}  // namespace esphome



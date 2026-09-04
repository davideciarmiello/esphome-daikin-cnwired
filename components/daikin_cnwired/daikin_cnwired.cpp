#include <algorithm>
#include "daikin_cnwired.h"
#include "esphome/core/log.h"
#include <vector>
#include <string>


namespace esphome {
namespace daikin_cnwired {

#define SETPOINT_STEP 1.0f
static const char *const TAG = "daikin_cnwired";

void DaikinCNWired::setup() {
  if (this->rx_pin_ == nullptr || this->tx_pin_ == nullptr) {
    ESP_LOGE(TAG, "RX/TX pins are required");
    this->mark_failed();
    return;
  }
  this->rx_pin_->setup();
  this->tx_pin_->setup();

  // The configuration schema restricts RX/TX to native ESP32 GPIO pins.
  // ESPHome is built with RTTI disabled (-fno-rtti), so do not use
  // dynamic_cast here. InternalGPIOPin exposes the hardware GPIO number.
  auto *rx_internal = static_cast<InternalGPIOPin *>(this->rx_pin_);
  auto *tx_internal = static_cast<InternalGPIOPin *>(this->tx_pin_);

  if (!this->driver_.install(
      static_cast<gpio_num_t>(rx_internal->get_pin()),
      static_cast<gpio_num_t>(tx_internal->get_pin()),
      this->rx_invert_,
      this->tx_invert_)) {
    ESP_LOGE(TAG, "Failed to initialize CN_WIRED RMT driver");
    this->mark_failed();
    return;
  }

  std::vector<const char *> fan_modes;
  fan_modes.reserve(1);
  if (this->turbo_as_preset_ == false) {
    fan_modes.push_back(this->fan_mode_turbo_.c_str());
  }
  if (!fan_modes.empty()) {
    // contiene almeno un elemento
    this->set_supported_custom_fan_modes(fan_modes);
  }
  //this->set_supported_custom_fan_modes({});
  //if (!this->powerful_preset_.empty())
  //  this->set_supported_custom_presets({this->fan_mode_turbo_.c_str()});

  // Start with the same sane defaults used by Faikout until the first mode
  // packet is received from the Daikin controller.
  //this->mode = climate::CLIMATE_MODE_AUTO;
  //this->fan_mode = climate::CLIMATE_FAN_AUTO;
  //this->target_temperature = 20.0f;
  //this->current_temperature = NAN;
  //this->action = climate::CLIMATE_ACTION_OFF;
  //this->publish_state();
  //this->request_rx_mode_ = true;
}

void DaikinCNWired::loop() {
  uint8_t packet[CNW_PKT_LEN];
  if (this->driver_.has_packet() && this->driver_.read(packet)) {
    this->process_packet_(packet);
    // CN_WIRED devices expect the controller command as the response to the
    // received packet. Faikout waits 20 ms to let the trailer pulse pass.
    // CN_WIRED expects our command as the response to the AC packet.
    // Always schedule the response here. If the user has changed the state,
    // send_command_() will use the saved user-command snapshot instead of
    // the state decoded from this packet.
    this->pending_tx_ms_ = millis() + 20;
  }

  //this->set_available(false);
  //return;

  if (this->pending_tx_ && millis() >= this->pending_tx_ms_) {
    if (this->last_rx_mode_time_ > 0)
    {
      this->pending_tx_ = false;
      this->send_command_();
    } else {
      ESP_LOGW(TAG, "cannot send, not received last_rx_mode_time_");
    }
  }
  
  bool changed = false;
  float temp = this->get_effective_current_temperature();
  if (!float_equal(temp, this->current_temperature, 1) && temp > 0)
  {
    this->current_temperature = temp;
    changed = true;
  }
  if (this->room_humidity_sensor_ != nullptr && !float_equal(this->room_humidity_sensor_->state, this->current_humidity, 0))
  {
      this->current_humidity=this->room_humidity_sensor_->state;
      changed = true;
  }
  if (changed)
      this->publish_state();

  // A missing bus packet for 10 seconds is treated as offline.
  if (this->online_ && this->last_rx_ms_ != 0 && millis() - this->last_rx_ms_ > 10000) {
    this->online_ = false;
    this->action = climate::CLIMATE_ACTION_OFF;
    ESP_LOGW(TAG, "CN_WIRED bus timeout - AC not responding");
    this->publish_state();
  }

  if (this->online_sensor_ != nullptr && this->online_sensor_->state != this->online_)
    this->online_sensor_->publish_state(this->online_);

}
void DaikinCNWired::user_command_changed_() {
  this->pending_tx_ = false; // viene poi impostato a true quando riceve il prossimo pacchetto, ed avrà un timeout di almeno 20ms
  //almeno dopo 20ms, cosi posso inviare piu cose insieme e do il tempo di settarle tutte
  if (this->pending_user_command_ == false) {
    this->pending_user_command_ = true;
    
    DaikinState &state = this->desired_state;
    state.has_tx_package_ = false;
    state.copy_from(this->current_state);
  }
  this->pending_user_command_sended_ = false;
  this->pending_user_command_sended_after_temp_report_ = false;
  this->pending_user_command_time_ = millis();
  this->pending_user_command_ = true;
  this->pending_user_command_count_ = 3;
  //lo reinvio per massimo 5 secondi
  this->pending_user_command_to_ = std::max(this->pending_user_command_to_, millis() + 10000);
  this->pending_user_command_to_confirm_ = true; // per forzare il caricamento dell'interfaccia qando arriva il pacchetto
}
void DaikinCNWired::pending_changes_apply() {
  //
}

bool DaikinCNWired::validate_packet_(const uint8_t *packet) const {
  return cnw_checksum(packet) == packet[CNW_CRC_TYPE_OFFSET];
}

void DaikinCNWired::process_packet_(const uint8_t *packet) {
  ++this->packet_count_;
  this->last_rx_ms_ = millis();

  if (!this->validate_packet_(packet)) {
    ++this->bad_checksum_count_;
    ESP_LOGW(TAG, "Invalid CN_WIRED checksum. Packet=%s", packet_to_string(packet).c_str());
    return;
  }

  //bool changed = this->frame_changed_(packet, this->last_rx_, this->has_last_rx_);
  //if (changed){
    // Log the complete raw 8-byte frame before any decoding. This is useful
    // when comparing ESPHome behaviour with the original Faikout firmware.
  //  ESP_LOGD(TAG, "RX CN_WIRED RAW: Packet=%s", packet_to_string(packet).c_str());
  //}

  //if (this->pending_user_command_ > 0)
  //{
  //  ESP_LOGD(TAG, "Ignoring RX Changes during send user command 1.");
  //  return;
  //}

  bool was_online = this->online_;
  this->online_ = true;
  if (!was_online)
    ESP_LOGI(TAG, "CN_WIRED AC online");

  const uint8_t type = packet[CNW_CRC_TYPE_OFFSET] & CNW_TYPE_MASK;
  this->last_rx_packet_type_ = type;

  switch (type) {
    case CNW_SENSOR_REPORT: {
      //non invio la risposta, altrimenti non arriva mai il pacchetto del MODE_CHANGED
      if (this->pending_user_command_)
      {        
          //ESP_LOGD(TAG, "Received Current temperature signal during send user command.");
        //  return;
        if (this->pending_user_command_ && this->pending_user_command_count_ > 0 && this->pending_user_command_sended_after_temp_report_ == false)
            this->pending_tx_ = true;
      }
      float temp = decode_bcd(packet[CNW_TEMP_OFFSET]);
      if (temp != this->current_state.current_temp_)
      {
          this->current_state.current_temp_ = temp;
          this->desired_state.current_temp_ = temp;
          ESP_LOGD(TAG, "RX Current temperature report:  %.1f °C - Packet=%s", temp, packet_to_string(packet).c_str());
          if (this->ac_temperature_sensor_ != nullptr)
              this->ac_temperature_sensor_->publish_state(temp);
      }
      break;
    }
    case CNW_MODE_CHANGED: {

      //il comando arriva ogni 6 secondi
      const Mode new_mode = decode_mode(packet);
      if (new_mode != Mode::INVALID)
      {
        this->last_rx_mode_time_ = millis();
        this->pending_tx_ = true;
        bool is_remote_control = (packet[CNW_SPECIALS_OFFSET] & CNW_SPECIALS_REMOTE_CONTROL) != 0;
        if (is_remote_control)
        {
          bool is_new = this->last_remote_control_checksum_ != packet[CNW_CRC_TYPE_OFFSET]
            || this->last_remote_control_ms_ == 0
            || this->last_remote_control_ms_ + 4000 < millis();
          if (is_new)
          {
            this->last_remote_control_checksum_ = packet[CNW_CRC_TYPE_OFFSET];
            this->last_remote_control_ms_ = millis();
            ESP_LOGD(TAG, "RX STATUS: Detected remote control. Packet=%s", packet_to_string(packet).c_str());
          } else {
            is_remote_control = false;
          }
        } else {
          this->last_remote_control_checksum_ = 0;
        }
        if (this->pending_user_command_ && is_remote_control)
        {
          this->pending_user_command_ = false;
          this->pending_user_command_to_confirm_ = false;
          ESP_LOGW(TAG, "RX STATUS: Sending pending command cancelled, received a remote change command. Packet=%s", packet_to_string(packet).c_str());
        }
        //se mentre invio arriva un comando, lo ignoro e forzo l'invio ancora di nuovo
        if (this->pending_user_command_ && this->pending_user_command_sended_)
        {
          DaikinState temp_state;
          temp_state.set_states_from_command_packet_(packet);
          if (temp_state == this->desired_state && this->pending_user_command_sended_)
          {
            this->pending_user_command_ = false;
            ESP_LOGI(TAG, "RX STATUS: Mode confirmed as sended: Packet=%s - Sended Packet: %s", packet_to_string(packet).c_str(), packet_to_string(desired_state.tx_package_).c_str());
          } else {
            ESP_LOGD(TAG, "RX STATUS: Mode waiting for confirm as sended: Packet=%s - Sended Packet: %s", packet_to_string(packet).c_str(), packet_to_string(desired_state.tx_package_).c_str());
          }
        }
        if (this->pending_user_command_)
        {
          this->pending_user_command_count_ += 3;
          if (this->pending_user_command_sended_ == false)
            ESP_LOGD(TAG, "RX STATUS: Received MODE_CHANGED signal during send user command without pending_user_command_sended_.");
          break;
        }
        bool changed = this->current_state.packet_changed(packet);
        if (changed || this->pending_user_command_to_confirm_ || is_remote_control)
        {
          ESP_LOGD(TAG, "RX STATUS: Mode changed. Packet=%s <---", packet_to_string(packet).c_str());
          this->current_state.set_states_from_command_packet_(packet);
          if (this->pending_user_command_to_confirm_){
            this->pending_user_command_to_confirm_ = false;
            if (this->current_state != this->desired_state)
            {
              ESP_LOGE(TAG, "TX COMMAND FAILED. Received response not match. Packet=%s - Sended Packet: %s", packet_to_string(packet).c_str(), packet_to_string(desired_state.tx_package_).c_str());
            }
          }
          if (this->pending_user_command_)
            break;
          this->set_climate_from_states_(this->current_state);
          if (this->pending_user_command_)
            break;
          this->publish_state();
        } else {
          ESP_LOGD(TAG, "RX STATUS: Packet=%s <--- (UNCHANGED)", packet_to_string(packet).c_str());
        }
      } else {        
        ESP_LOGE(TAG, "RX STATUS: Mode not valid. Packet=%s", packet_to_string(packet).c_str());
      }

      break;
    }

    default:
      ESP_LOGD(TAG, "Unknown CN_WIRED packet type: 0x%02X", type);
      ESP_LOGD(TAG, "RX CN_WIRED RAW: Packet=%s", packet_to_string(packet).c_str());
      return;
  }
}

climate::ClimateAction DaikinCNWired::calculate_action_(DaikinState &state) const {
  if (state.power_ == false) 
    return climate::CLIMATE_ACTION_OFF;
  switch (this->mode) {
    case climate::CLIMATE_MODE_OFF: return climate::CLIMATE_ACTION_OFF;
    case climate::CLIMATE_MODE_HEAT: return climate::CLIMATE_ACTION_HEATING;
    case climate::CLIMATE_MODE_COOL: return climate::CLIMATE_ACTION_COOLING;
    case climate::CLIMATE_MODE_DRY: return climate::CLIMATE_ACTION_DRYING;
    case climate::CLIMATE_MODE_FAN_ONLY: return climate::CLIMATE_ACTION_FAN;
    case climate::CLIMATE_MODE_AUTO: return climate::CLIMATE_ACTION_IDLE;
    default: return climate::CLIMATE_ACTION_IDLE;
  }
}


void DaikinCNWired::set_climate_from_states_(DaikinState &state) {
    this->mode = to_esphome_mode(state.mode_, state.power_);
    this->fan_mode = to_esphome_fan(state.fan_);
    if (state.powerful_ && this->turbo_as_preset_)
      this->preset = climate::CLIMATE_PRESET_BOOST;
    else if (state.sleep_ && this->sleep_as_preset_)
      this->preset = climate::CLIMATE_PRESET_SLEEP;
    else if (this->preset == climate::CLIMATE_PRESET_BOOST 
        || this->preset == climate::CLIMATE_PRESET_SLEEP)
      this->preset = climate::CLIMATE_PRESET_NONE;
    this->clear_custom_fan_mode_();
    if (state.powerful_ && this->turbo_as_preset_ == false)
      this->set_custom_fan_mode_(this->fan_mode_turbo_.c_str());
    if (state.swing_v_)
      this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
    else
      this->swing_mode = climate::CLIMATE_SWING_OFF;
    this->target_temperature = state.target_temp_;
    //this->current_temperature = this->current_temp_;
    this->action = this->calculate_action_(state);

    if (this->switch_led_ != nullptr) {
      this->switch_led_->publish_state(state.led_);
    }
    if (this->switch_sleep_ != nullptr) {
      this->switch_sleep_->publish_state(state.sleep_);
    }
    if (this->switch_power_ != nullptr) {
      this->switch_power_->publish_state(state.power_);
    }
}


void DaikinCNWired::switch_callback(DaikinSwitch *sw, bool value, DaikinSwitchType type) {
  if (type == SWITCH_LISTEN_ONLY)
    return;
  this->user_command_changed_();  
  DaikinState &state = this->desired_state;  
  switch (type) {
    case SWITCH_LED:
      ESP_LOGD(TAG, "Changed LED: %s", value ? "ON" : "OFF");
      state.led_ = value;
      break;
    case SWITCH_SLEEP:
      ESP_LOGD(TAG, "Changed SLEEP: %s", value ? "ON" : "OFF");
      state.sleep_ = value;
      break;
    case SWITCH_POWER:
      ESP_LOGD(TAG, "Changed POWER: %s", value ? "ON" : "OFF");
      state.power_ = value;
      break;
    case SWITCH_LISTEN_ONLY:
      break;
  }
}


void DaikinCNWired::control(const climate::ClimateCall &call) {
  //bool changed = false;
  ESP_LOGD(TAG, "=== CLIMATE CALL ===");
  

  DaikinState &state = this->desired_state;

  if (call.get_mode().has_value()) {
    ESP_LOGD(TAG, "mode: %d", call.get_mode().has_value() ? *call.get_mode() : -1);
    this->user_command_changed_();
    auto m = *call.get_mode();
    if ((m == climate::CLIMATE_MODE_AUTO && !this->auto_mode_) 
      || (m == climate::CLIMATE_MODE_HEAT_COOL && this->heat_cool_as_power_on_))
    {
      m = to_esphome_mode(state.mode_, true);
      ESP_LOGD(TAG, "Accendo con nuova modalità: %s", LOG_STR_ARG(climate_mode_to_string(m)));
      state.power_ = true;
    } else if (m == climate::CLIMATE_MODE_OFF)
    {      
      state.power_ = false;
    } else {
      state.power_ = true;
      ESP_LOGD(TAG, "Setto nuova modalità: %s", LOG_STR_ARG(climate_mode_to_string(m)));
      state.mode_ = from_esphome_mode(m, state.mode_);
    }
  }

  if (call.get_target_temperature().has_value()) {
    ESP_LOGD(TAG, "target temp: %s", call.get_target_temperature().has_value() ? "YES" : "NO");
    this->user_command_changed_();
    const auto m = *call.get_target_temperature();
    if (this->target_temperature != m || state.target_temp_ != m)
    {
      //this->user_command_changed_();
      //this->target_temp_ = std::round(*call.get_target_temperature());
      //this->target_temp_ = std::clamp(this->target_temp_, 16.0f, 30.0f);
      //this->target_temperature = m;
      state.target_temp_ = m;
    }
  }

  if (call.get_preset().has_value()) {
    ESP_LOGD(TAG, "preset: %s", call.get_preset().has_value() ? "YES" : "NO");
    this->user_command_changed_();
    const auto m = *call.get_preset();
    if (this->preset != m)
    {      
      if (this->turbo_as_preset_)
        state.powerful_ = m == climate::CLIMATE_PRESET_BOOST;
      if (this->sleep_as_preset_)
        state.sleep_ = m == climate::CLIMATE_PRESET_SLEEP;
      this->preset = m;
    }
  }

  if (call.get_fan_mode().has_value()) {
    ESP_LOGD(TAG, "fan mode: %s", call.get_fan_mode().has_value() ? "YES" : "NO");
    this->user_command_changed_();
    const auto m = *call.get_fan_mode();
    if (this->fan_mode != m)
    {
      if (this->turbo_as_preset_ == false)
        state.powerful_ = false;
      state.fan_ = from_esphome_fan(m);
      this->fan_mode = m;
    }
  }

  if (call.has_custom_fan_mode()) {
    this->user_command_changed_();
    if (this->turbo_as_preset_ == false)
      state.powerful_ = false;
    const auto mode = call.get_custom_fan_mode();
    ESP_LOGD(TAG, "custom fan mode: %s", mode.c_str());
    if (!mode.empty())
    {
      if (this->fan_mode_turbo_ == mode.c_str())
        state.powerful_ = true;
    }
  }

  //if (call.has_custom_preset()) {
  //  const auto preset = call.get_custom_preset();
  //  this->powerful_ = !preset.empty() && this->powerful_preset_ == preset.c_str();
  //  changed = true;
  //}

  if (call.get_swing_mode().has_value()) {
    this->user_command_changed_();
    const auto m = *call.get_swing_mode();
    ESP_LOGD(TAG, "swing: %s", m == climate::CLIMATE_SWING_VERTICAL ? "Vertical" : "-");
    state.swing_v_ = m == climate::CLIMATE_SWING_VERTICAL;
  }

 //// Do not transmit immediately. CN_WIRED is controller-driven: commands
 //// must be sent as the response to an incoming AC packet. The next received
 //// packet will schedule the 20 ms response in loop().
    //this->publish_state();
}

void DaikinCNWired::send_raw(const std::string &command) {
  std::string input = command;

  // Supporta:
  // 21 00 23 08 08 F0 11 90
  // 21:00:23:08:08:F0:11:90

  for (char &c : input) {
    if (c == ':')
      c = ' ';
  }

  uint8_t data[8];
  int count = 0;
  size_t pos = 0;

  while (pos < input.length() && count < 8) {
    while (pos < input.length() && input[pos] == ' ')
      pos++;

    if (pos >= input.length())
      break;

    size_t end = input.find(' ', pos);

    if (end == std::string::npos)
      end = input.length();

    std::string token = input.substr(pos, end - pos);

    if (token.empty() || token.length() > 2) {
      ESP_LOGE(TAG, "Invalid byte: %s", token.c_str());
      return;
    }

    char *endptr = nullptr;
    long value = strtol(token.c_str(), &endptr, 16);

    if (*endptr != '\0' || value < 0 || value > 0xFF) {
      ESP_LOGE(TAG, "Invalid HEX byte: %s", token.c_str());
      return;
    }

    data[count++] = static_cast<uint8_t>(value);
    pos = end;
  }

  if (count != 8) {
    ESP_LOGE(TAG, "Invalid raw command: expected 8 bytes, got %d", count);
    return;
  }

  DaikinState &state = this->current_state; 
  if (data[CNW_TEMP_OFFSET] == 0)
    data[CNW_TEMP_OFFSET] = encode_bcd(static_cast<uint8_t>(std::lround(state.target_temp_)));
  if (data[1] == 0)
  {
    data[1] = 0x04;
    data[2] = 0x50;
  }
  if (data[CNW_MODE_OFFSET] == 0)
    data[CNW_MODE_OFFSET] = encode_mode(state.mode_, state.power_);
  if (data[CNW_FAN_OFFSET] == 0)
    data[CNW_FAN_OFFSET] = state.powerful_ ? CNW_FAN_POWERFUL : encode_fan(state.fan_);

  data[CNW_CRC_TYPE_OFFSET] = CNW_COMMAND;
  data[CNW_CRC_TYPE_OFFSET] = cnw_checksum(data);

  ESP_LOGD(TAG, "SENDING RAW TX: Packet=%s", packet_to_string(data).c_str());

  if (this->switch_listen_only_ != nullptr && this->switch_listen_only_->state == false)
    this->switch_listen_only_->publish_state(true);
  
  if (!this->driver_.write(data)) {
    ESP_LOGW(TAG, "Failed to transmit CN_WIRED packet=%s", packet_to_string(data).c_str());
    //this->has_last_tx_ = false;
    return;
  }
}


void DaikinCNWired::send_command_() {

  if (this->switch_listen_only_ != nullptr && this->switch_listen_only_->state == true) {
      //ESP_LOGW(TAG, "Cannot send data: listen only enabled.");
    if (this->pending_user_command_)
    {
      this->pending_user_command_ = false;
      ESP_LOGW(TAG, "Cannot send user command: listen only enabled.");
    }
    return;
  }

  // Use the user's queued snapshot when one exists. This is essential for OFF:
  // the Daikin may transmit an old ON status between the control request and
  // our 20 ms response.
  DaikinState &state = this->current_state;  
  uint32_t pending_user_command_time = this->pending_user_command_time_;
  if (this->pending_user_command_)
  {
    this->pending_changes_apply();
    state = this->desired_state;
  }
  uint8_t *buf = state.tx_package_;
  if (state.has_tx_package_ == false)
  {
    const uint8_t fan = state.powerful_ ? CNW_FAN_POWERFUL : encode_fan(state.fan_);


    uint8_t specials = 0;
    if (state.swing_v_)
      specials |= CNW_SPECIALS_V_SWING_SEND; // il flag originale è CNW_V_SWING 0x10 ma non basta
    if (state.led_)
      specials |= CNW_SPECIALS_LED_ON; // 0x80
    
    uint8_t specials_send = CNW_WRITESPECIALS_BASE; //0001 0000
    if (state.swing_v_)
      specials_send |= CNW_WRITESPECIALS_V_SWING; //0000 0001
    if (state.sleep_)
      specials_send |= CNW_WRITESPECIALS_SLEEP; //0000 0010
    if (state.led_)
      specials_send |= CNW_WRITESPECIALS_LED_ON; //0000 1000

    buf[CNW_TEMP_OFFSET] = encode_bcd(static_cast<uint8_t>(std::lround(state.target_temp_)));
    buf[1] = 0x04;
    buf[2] = 0x50;
    buf[CNW_MODE_OFFSET] = encode_mode(state.mode_, state.power_);
    buf[CNW_FAN_OFFSET] = fan;
    buf[CNW_SPECIALS_OFFSET] = specials;
    buf[CNW_WRITESPECIALS_OFFSET] = specials_send;
    buf[CNW_CRC_TYPE_OFFSET] = CNW_COMMAND;
    buf[CNW_CRC_TYPE_OFFSET] = cnw_checksum(buf);
    state.has_tx_package_ = true;
    if (this->pending_user_command_ == false)
      ESP_LOGD(TAG, "TX STATUS CONFIRM: Packet=%s --->", packet_to_string(buf).c_str());
  }

  //if (this->frame_changed_(buf, this->last_tx_, this->has_last_tx_)) {
   // ESP_LOGD(TAG, "TX CN_WIRED RAW: Packet=%s", packet_to_string(packet).c_str());
  //}
  if (!this->driver_.write(buf)) {
    ESP_LOGE(TAG, "Failed to transmit CN_WIRED Packet=%s", packet_to_string(buf).c_str());
    //this->has_last_tx_ = false;
    return;
  }

  if (this->pending_user_command_)
  {
    //se nel frattempo ho ricevuto un nuovo user command
    if (this->pending_user_command_time_ != pending_user_command_time)
    {
      //devo reinviare il dato
      return;
    }
    
    this->pending_user_command_sended_ = true;
    this->pending_user_command_to_confirm_ = true;
    this->pending_user_command_count_ = this->pending_user_command_count_ - 1;
    if (this->last_rx_packet_type_ == CNW_SENSOR_REPORT)
      this->pending_user_command_sended_after_temp_report_ = true;
    
    ESP_LOGD(TAG, "TX STATUS: Packet=%s --->", packet_to_string(buf).c_str());

    this->set_climate_from_states_(this->desired_state);
    this->publish_state();

    if (static_cast<int32_t>(millis() - this->pending_user_command_to_) >= 0 || this->pending_user_command_count_ <= 0)
    {
      //this->pending_tx_ms_ = std::max(this->pending_tx_ms_, millis() + 200);
      this->pending_user_command_ = false;
    }
  } else {
  }
}

climate::ClimateTraits DaikinCNWired::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_COOL,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_DRY,
      climate::CLIMATE_MODE_FAN_ONLY,
  });
  if (this->auto_mode_) {
    traits.add_supported_mode(climate::CLIMATE_MODE_AUTO);
  }
  if (this->heat_cool_as_power_on_) {
    traits.add_supported_mode(climate::CLIMATE_MODE_AUTO);
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT_COOL);
  }

  traits.set_supported_fan_modes({climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW,
                                  climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH,
                                  climate::CLIMATE_FAN_QUIET});
  traits.set_supported_swing_modes({climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL});
  
  traits.set_supported_presets({});
  if (this->turbo_as_preset_) {
    traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);
    traits.add_supported_preset(climate::CLIMATE_PRESET_BOOST);
  }
  if (this->sleep_as_preset_) {
    traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);
    traits.add_supported_preset(climate::CLIMATE_PRESET_SLEEP);
  }
  
    traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);
    traits.add_supported_preset(climate::CLIMATE_PRESET_SLEEP);

  //traits.set_supported_presets({
  //    climate::CLIMATE_PRESET_NONE,
  //    //climate::CLIMATE_PRESET_HOME,
  //    //climate::CLIMATE_PRESET_AWAY,
  //    climate::CLIMATE_PRESET_BOOST,
  //    //climate::CLIMATE_PRESET_COMFORT,
  //    //climate::CLIMATE_PRESET_ECO,
  //    //climate::CLIMATE_PRESET_SLEEP,
  //    //climate::CLIMATE_PRESET_ACTIVITY,
  //});
  
  traits.set_visual_min_temperature(16.0f);
  traits.set_visual_max_temperature(30.0f);
  traits.set_visual_temperature_step(SETPOINT_STEP);
  traits.set_visual_current_temperature_step(0.1f);

  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_ACTION);
  if (this->room_humidity_sensor_ != nullptr) {
    traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_HUMIDITY);
  }
  return traits;
}

void DaikinCNWired::publish_state_() { this->publish_state(); }

void DaikinCNWired::dump_config() {
  ESP_LOGCONFIG(TAG, "Daikin CN_WIRED:");
  LOG_PIN("  RX pin: ", this->rx_pin_);
  LOG_PIN("  TX pin: ", this->tx_pin_);
  ESP_LOGCONFIG(TAG, "  RX invert: %s", this->rx_invert_ ? "yes" : "no");
  ESP_LOGCONFIG(TAG, "  TX invert: %s", this->tx_invert_ ? "yes" : "no");
  if (this->room_sensor_ != nullptr) {
    if (!this->room_sensor_unit_is_valid()) {
      ESP_LOGCONFIG(TAG, "  ROOM SENSOR: INVALID UNIT '%s' (must be °C or °F)",
                    this->room_sensor_->get_unit_of_measurement_ref().c_str());
    } else {
      ESP_LOGCONFIG(TAG, "  Room sensor: %s",
                    this->room_sensor_->get_name().c_str());
      //ESP_LOGCONFIG(TAG, "  Setpoint interval: %d", this->setpoint_interval);
    }
  }
}



bool DaikinCNWired::use_room_sensor() {
  return this->room_sensor_unit_is_valid() && this->room_sensor_->has_state() &&
         !isnanf(this->room_sensor_->get_state());
}

bool DaikinCNWired::room_sensor_unit_is_valid() {
  if (this->room_sensor_ != nullptr) {
    auto u = this->room_sensor_->get_unit_of_measurement_ref();
    return u == "°C" || u == "°F";
  }
  return false;
}

float DaikinCNWired::room_sensor_degc() {
  float temp = this->room_sensor_->get_state();
  if (this->room_sensor_->get_unit_of_measurement_ref() == "°F") {
    temp = fahrenheit_to_celsius(temp);
  }
  return temp;
}

float DaikinCNWired::get_effective_current_temperature() {
  if (this->use_room_sensor()) {
    return this->room_sensor_degc();
  }
  return this->current_state.current_temp_;
}

float DaikinCNWired::get_room_temp_offset() {
  if (!this->use_room_sensor()) {
    return 0.0;
  }
  float room_val = this->room_sensor_degc();
  float s21_val = this->current_state.current_temp_;
  return s21_val - room_val;
}

float nearest_step(float temp) {
  return std::round(temp / SETPOINT_STEP) * SETPOINT_STEP;
}

}  // namespace daikin_cnwired
}  // namespace esphome

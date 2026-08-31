#pragma once

#include <cstddef>
#include <cstdint>
#include <driver/gpio.h>
#include <driver/rmt_rx.h>
#include <driver/rmt_tx.h>
#include "esphome/core/log.h"
#include "cn_wired.h"

namespace esphome {
namespace daikin_cnwired {

class CNWiredDriver {
 public:
  bool install(gpio_num_t rx_pin, gpio_num_t tx_pin, bool rx_invert, bool tx_invert);
  void uninstall();
  bool read(uint8_t *data);
  bool write(const uint8_t *data);
  bool has_packet() const { return this->rx_len_ != 0; }
  uint32_t invalid_packets() const { return this->invalid_packets_; }

 protected:
  static bool rx_callback_(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data);
  void restart_receive_();
  bool decode_(uint8_t *data);

  static constexpr uint32_t SYNC_US = 2600;
  static constexpr uint32_t START_US = 1000;
  static constexpr uint32_t SPACE_US = 300;
  static constexpr uint32_t BIT0_US = 400;
  static constexpr uint32_t BIT1_US = 1000;
  static constexpr uint32_t IDLE_US = 16000;
  static constexpr uint32_t TERM_US = 2000;
  static constexpr uint32_t MARGIN_US = 200;

  rmt_channel_handle_t tx_{nullptr};
  rmt_channel_handle_t rx_{nullptr};
  rmt_encoder_handle_t encoder_{nullptr};
  rmt_symbol_word_t rx_raw_[70]{};
  volatile size_t rx_len_{0};
  uint32_t invalid_packets_{0};
  bool tx_invert_{false};
  bool installed_{false};
};

}  // namespace daikin_cnwired
}  // namespace esphome

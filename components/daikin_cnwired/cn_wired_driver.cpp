#include "cn_wired_driver.h"
#include <algorithm>
#include <cstring>
#include "esphome/core/log.h"

namespace esphome {
namespace daikin_cnwired {

static const char *const TAG = "daikin_cnwired.driver";

bool CNWiredDriver::rx_callback_(rmt_channel_handle_t, const rmt_rx_done_event_data_t *edata, void *user_data) {
  auto *self = static_cast<CNWiredDriver *>(user_data);
  if (edata->num_symbols < 64) {
    self->rx_len_ = 0;
    self->restart_receive_();
    return false;
  }
  self->rx_len_ = edata->num_symbols;
  return false;
}

void CNWiredDriver::restart_receive_() {
  if (this->rx_ == nullptr)
    return;
  rmt_receive_config_t config{};
  config.signal_range_min_ns = 1000;
  config.signal_range_max_ns = (SYNC_US + MARGIN_US) * 1000;
  esp_err_t err = rmt_receive(this->rx_, this->rx_raw_, sizeof(this->rx_raw_), &config);
  if (err != ESP_OK)
    ESP_LOGW(TAG, "rmt_receive failed: %s", esp_err_to_name(err));
}

bool CNWiredDriver::install(gpio_num_t rx_pin, gpio_num_t tx_pin, bool rx_invert, bool tx_invert) {
  if (this->installed_)
    return false;
  this->tx_invert_ = tx_invert;

  // Declare all local objects before any goto. GCC rejects jumping over
  // the initialization of C++ objects (the ESPHome build uses -fpermissive=off).
  rmt_copy_encoder_config_t encoder_config{};
  rmt_tx_channel_config_t tx_config{};
  rmt_rx_channel_config_t rx_config{};
  rmt_rx_event_callbacks_t callbacks{};

  if (rmt_new_copy_encoder(&encoder_config, &this->encoder_) != ESP_OK)
    return false;


  tx_config.clk_src = RMT_CLK_SRC_DEFAULT;
  tx_config.gpio_num = tx_pin;
  tx_config.mem_block_symbols = 72;
  tx_config.resolution_hz = 1000000;
  tx_config.trans_queue_depth = 1;
  // The new RMT TX driver idles low. Faikout therefore inverts the output and
  // expresses the waveform using TX_HIGH=0/TX_LOW=1.
  tx_config.flags.invert_out = tx_invert ^ true;

  esp_err_t err = rmt_new_tx_channel(&tx_config, &this->tx_);
  if (err != ESP_OK)
    goto fail;
  err = rmt_enable(this->tx_);
  if (err != ESP_OK)
    goto fail;

  rx_config.clk_src = RMT_CLK_SRC_DEFAULT;
  rx_config.resolution_hz = 1000000;
  rx_config.mem_block_symbols = 72;
  rx_config.gpio_num = rx_pin;
  rx_config.flags.invert_in = rx_invert;

  err = rmt_new_rx_channel(&rx_config, &this->rx_);
  if (err != ESP_OK)
    goto fail;

  callbacks.on_recv_done = &CNWiredDriver::rx_callback_;
  err = rmt_rx_register_event_callbacks(this->rx_, &callbacks, this);
  if (err != ESP_OK)
    goto fail;
  err = rmt_enable(this->rx_);
  if (err != ESP_OK)
    goto fail;

  this->rx_len_ = 0;
  this->invalid_packets_ = 0;
  this->restart_receive_();
  this->installed_ = true;
  ESP_LOGI(TAG, "CN_WIRED RMT installed RX=%d TX=%d RX invert=%s TX invert=%s",
           static_cast<int>(rx_pin), static_cast<int>(tx_pin), rx_invert ? "yes" : "no", tx_invert ? "yes" : "no");
  return true;

fail:
  this->uninstall();
  return false;
}

void CNWiredDriver::uninstall() {
  this->rx_len_ = 0;
  if (this->rx_ != nullptr) {
    rmt_disable(this->rx_);
    rmt_del_channel(this->rx_);
    this->rx_ = nullptr;
  }
  if (this->tx_ != nullptr) {
    rmt_disable(this->tx_);
    rmt_del_channel(this->tx_);
    this->tx_ = nullptr;
  }
  if (this->encoder_ != nullptr) {
    rmt_del_encoder(this->encoder_);
    this->encoder_ = nullptr;
  }
  this->installed_ = false;
}

bool CNWiredDriver::decode_(uint8_t *rx) {
  if (this->rx_len_ != CNW_PKT_LEN * 8 + 2) {
    this->invalid_packets_++;
    return false;
  }

  size_t p = 0;
  auto in_range = [](uint32_t value, uint32_t target) {
    return value >= target - MARGIN_US && value <= target + MARGIN_US;
  };

  if (this->rx_raw_[p].level0 != 0 || !in_range(this->rx_raw_[p].duration0, SYNC_US) ||
      !in_range(this->rx_raw_[p].duration1, START_US)) {
    this->invalid_packets_++;
    return false;
  }
  ++p;

  for (uint8_t i = 0; i < CNW_PKT_LEN; ++i) {
    rx[i] = 0;
    for (uint8_t bit = 0x01; bit != 0; bit <<= 1) {
      if (!in_range(this->rx_raw_[p].duration0, SPACE_US)) {
        this->invalid_packets_++;
        return false;
      }
      const uint32_t duration = this->rx_raw_[p].duration1;
      if (duration >= BIT1_US - MARGIN_US && duration <= BIT1_US + MARGIN_US) {
        rx[i] |= bit;
      } else if (!(duration >= BIT0_US - MARGIN_US && duration <= BIT0_US + MARGIN_US)) {
        this->invalid_packets_++;
        return false;
      }
      ++p;
    }
  }

  return true;
}

bool CNWiredDriver::read(uint8_t *data) {
  if (!this->installed_ || this->rx_len_ == 0)
    return false;

  bool ok = this->decode_(data);
  this->rx_len_ = 0;
  this->restart_receive_();
  return ok;
}

bool CNWiredDriver::write(const uint8_t *buf) {
  if (!this->installed_ || this->tx_ == nullptr || this->encoder_ == nullptr)
    return false;

  // One RMT symbol contains two states. Keep the complete sync LOW, then
  // encode HIGH/LOW pairs for start/data bits, followed by the idle trailer.
  rmt_symbol_word_t seq[3 + CNW_PKT_LEN * 8 + 1]{};
  size_t p = 0;
  constexpr uint16_t TX_HIGH = 0;
  constexpr uint16_t TX_LOW = 1;

  seq[p].duration0 = SYNC_US - 1000;
  seq[p].level0 = TX_LOW;
  seq[p].duration1 = 1000;
  seq[p].level1 = TX_LOW;
  ++p;

  auto add = [&](uint16_t duration) {
    seq[p].duration0 = duration;
    seq[p].level0 = TX_HIGH;
    seq[p].duration1 = SPACE_US;
    seq[p].level1 = TX_LOW;
    ++p;
  };

  add(START_US);
  for (uint8_t i = 0; i < CNW_PKT_LEN; ++i) {
    for (uint8_t bit = 0x01; bit != 0; bit <<= 1)
      add((buf[i] & bit) ? BIT1_US : BIT0_US);
  }

  seq[p].duration0 = IDLE_US;
  seq[p].level0 = TX_HIGH;
  seq[p].duration1 = TERM_US;
  seq[p].level1 = TX_LOW;
  ++p;

  rmt_transmit_config_t tx_config{};
  tx_config.flags.eot_level = TX_HIGH;
  esp_err_t err = rmt_transmit(this->tx_, this->encoder_, seq, p * sizeof(rmt_symbol_word_t), &tx_config);
  if (err != ESP_OK)
    return false;
  err = rmt_tx_wait_all_done(this->tx_, 1000);
  return err == ESP_OK;
}

}  // namespace daikin_cnwired
}  // namespace esphome

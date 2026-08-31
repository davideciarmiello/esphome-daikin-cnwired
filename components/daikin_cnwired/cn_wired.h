#pragma once

#include <cstdint>

namespace esphome {
namespace daikin_cnwired {

static constexpr uint8_t CNW_PKT_LEN = 8;
static constexpr uint8_t CNW_TEMP_OFFSET = 0;
static constexpr uint8_t CNW_MODE_OFFSET = 3;
static constexpr uint8_t CNW_FAN_OFFSET = 4;
static constexpr uint8_t CNW_SPECIALS_OFFSET = 5;
static constexpr uint8_t CNW_CRC_TYPE_OFFSET = 7;

static constexpr uint8_t CNW_MODE_POWEROFF = 0x10;
static constexpr uint8_t CNW_MODE_MASK = 0x0F;
static constexpr uint8_t CNW_DRY = 0;
static constexpr uint8_t CNW_FAN = 1;
static constexpr uint8_t CNW_COOL = 2;
static constexpr uint8_t CNW_HEAT = 4;
static constexpr uint8_t CNW_AUTO = 8;

static constexpr uint8_t CNW_FAN_1 = 8;
static constexpr uint8_t CNW_FAN_2 = 4;
static constexpr uint8_t CNW_FAN_3 = 2;
static constexpr uint8_t CNW_FAN_AUTO = 1;
static constexpr uint8_t CNW_FAN_POWERFUL = 3;
static constexpr uint8_t CNW_FAN_QUIET = 9;

static constexpr uint8_t CNW_LED_ON = 0x80;
static constexpr uint8_t CNW_V_SWING = 0x10;
static constexpr uint8_t CNW_SPECIAL_REMOTE_CONTROL = 0x40;
static constexpr uint8_t CNW_TYPE_MASK = 0x0F;
static constexpr uint8_t CNW_SENSOR_REPORT = 0;
static constexpr uint8_t CNW_MODE_CHANGED = 1;
static constexpr uint8_t CNW_COMMAND = 0;


inline uint8_t cnw_checksum(const uint8_t *data) {
  const uint8_t last_nibble = data[CNW_CRC_TYPE_OFFSET] & CNW_TYPE_MASK;
  uint8_t crc = last_nibble;
  for (int i = 0; i < CNW_CRC_TYPE_OFFSET; i++) {
    crc += (data[i] >> 4) + (data[i] & 0x0F);
  }
  if (last_nibble > CNW_MODE_CHANGED)
    crc = static_cast<uint8_t>(0x0F - crc);
  return static_cast<uint8_t>((crc << 4) | last_nibble);
}

inline uint8_t decode_bcd(uint8_t data) {
  return static_cast<uint8_t>((data >> 4) * 10 + (data & 0x0F));
}

inline uint8_t encode_bcd(uint8_t value) {
  return static_cast<uint8_t>(((value / 10) << 4) | (value % 10));
}

enum class Mode : uint8_t {
  FAN = 0,
  HEAT = 1,
  COOL = 2,
  AUTO = 3,
  DRY = 7,
  INVALID = 0xFF,
};

enum class Fan : uint8_t {
  AUTO = 0,
  LEVEL1 = 1,
  //LEVEL2 = 2,
  LEVEL3 = 3,
  //LEVEL4 = 4,
  LEVEL5 = 5,
  QUIET = 6,
  INVALID = 0xFF,
};

inline Mode decode_mode(const uint8_t *data) {
  switch (data[CNW_MODE_OFFSET] & CNW_MODE_MASK) {
    case CNW_DRY: return Mode::DRY;
    case CNW_FAN: return Mode::FAN;
    case CNW_COOL: return Mode::COOL;
    case CNW_HEAT: return Mode::HEAT;
    case CNW_AUTO: return Mode::AUTO;
    default: return Mode::INVALID;
  }
}

inline Fan decode_fan(const uint8_t *data) {
  switch (data[CNW_FAN_OFFSET]) {
    case CNW_FAN_1: return Fan::LEVEL1;
    case CNW_FAN_2: return Fan::LEVEL3;
    case CNW_FAN_3: return Fan::LEVEL5;
    case CNW_FAN_AUTO: return Fan::AUTO;
    case CNW_FAN_QUIET: return Fan::QUIET;
    default: return Fan::INVALID;
  }
}

inline uint8_t encode_mode(Mode mode, bool power) {
  //fix per dry,  i valori bits sono nel primo gruppo di bits. ON 0x20: 0 0 1 0 0 0 0 0 - OFF: 0 0 1 1 0 0 0 0
  if (mode == Mode::DRY)
    return power ? 0x20 : 0x30;
  uint8_t value = CNW_AUTO;
  switch (mode) {
    case Mode::FAN: value = CNW_FAN; break;
    case Mode::HEAT: value = CNW_HEAT; break;
    case Mode::COOL: value = CNW_COOL; break;
    case Mode::AUTO: value = CNW_AUTO; break;
    case Mode::DRY: value = CNW_DRY; break;
    default: value = CNW_AUTO; break;
  }
  return power ? value : static_cast<uint8_t>(value | CNW_MODE_POWEROFF);
}

inline uint8_t encode_fan(Fan fan) {
  switch (fan) {
    case Fan::AUTO: return CNW_FAN_AUTO;
    case Fan::QUIET: return CNW_FAN_QUIET;
    case Fan::LEVEL1: return CNW_FAN_1;
    case Fan::LEVEL3: return CNW_FAN_2;
    case Fan::LEVEL5: return CNW_FAN_3;
    default: return CNW_FAN_AUTO;
  }
}

}  // namespace daikin_cnwired
}  // namespace esphome

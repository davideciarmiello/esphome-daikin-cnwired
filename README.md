# Daikin CN-WIRED for ESPHome

An **ESPHome custom component for Daikin CN-WIRED** wired remote controllers.

This project allows an ESP32 to communicate with Daikin air conditioning systems through the **CN-WIRED interface**, exposing the unit to ESPHome and, consequently, to Home Assistant.

The component is designed to provide a native ESPHome `climate` entity, allowing you to monitor and control the main HVAC functions directly from Home Assistant.

## ✨ Features

* 🌡️ Temperature monitoring
* 🎯 Target temperature control
* 🔥 Heating mode
* ❄️ Cooling mode
* 💧 Dry / dehumidification mode
* 🌀 Fan-only mode
* 🤖 Auto mode
* 🌬️ Fan speed control
* ⚡ Powerful / Boost mode
* ↕️ Vertical swing
* 💡 Wired controller LED control
* 🔄 Bidirectional communication with the Daikin unit
* 📡 CN-WIRED packet decoding and transmission
* 🏠 Native ESPHome climate integration
* Home Assistant compatible

> Features may depend on the specific Daikin indoor unit and wired controller.

---

## 🙏 Credits

This project would not have been possible without the excellent reverse-engineering work by **[faikout](https://codeberg.org/faikout)**.

A huge thank you to **faikout** for researching and documenting the Daikin CN-WIRED protocol and for the original implementation that made this project possible.

This ESPHome component is based on that work and adapts the CN-WIRED communication to the ESPHome ecosystem.

Please consider checking out the original project and giving credit to the original author when using or extending this implementation.

---

## 🔌 Hardware

You need:

* An ESP32 board
* A connection to the Daikin CN-WIRED bus
* A Daikin indoor unit supporting the CN-WIRED interface

The ESP32 communicates directly with the CN-WIRED bus using separate RX and TX connections.

> ⚠️ **Warning:** CN-WIRED is an electrical interface. Make sure you understand the electrical characteristics of your specific Daikin system before connecting an ESP32. An incorrect connection may damage the HVAC controller or the ESP32.

---

## 📦 ESPHome Configuration

Example configuration:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/davideciarmiello/esphome-daikin-cnwired
    components: [daikin_cnwired]

climate:
  - platform: daikin_cnwired
    name: "Daikin"
    id: daikin

    rx_pin:
      number: GPIO16
      mode:
        input: true

    tx_pin:
      number: GPIO17

    rx_invert: false
    tx_invert: false

    auto_mode: true # disable if you not have auto_mode
    heat_cool_as_power_on: true # useful for power on restoring old mode, called from turn_on of Home Assistant    
    turbo_as_preset: false # if true, show turbo as preset and not as fan speed

    room_temperature_sensor: external_temperature
    room_humidity_sensor: external_humidity

    led_switch:
      name: "Daikin LED"
    sleep_switch:
      name: "Daikin SLEEP"
    power_switch:
      name: "Daikin Power"
    
    ac_temperature_sensor:
      name: "Daikin Temperature"
    online_sensor:
      name: "Daikin OnLine"

    # for debug, only receive commands and not send response
    listen_only_switch:
      name: "Daikin Listen Only"

sensor:
  - platform: template
    id: external_humidity
    name: "Template Sensor humidity"
    lambda: |-
      return 42.0;
    update_interval: 60s

  - platform: template
    id: external_temperature
    name: "Template Sensor temperature"
    lambda: |-
      return 25.0;
    update_interval: 60s
    unit_of_measurement: °C
    

# for debug, you can send a raw command
text:
  - platform: template
    name: "Raw Command"
    id: raw_command
    mode: text
    optimistic: true
    restore_value: false
    max_length: 23
    disabled_by_default: true
    entity_category: diagnostic

button:
  - platform: template
    name: "Send Raw Command"
    id: send_raw_command
    icon: "mdi:send"
    disabled_by_default: true
    entity_category: diagnostic
    on_press:
      - lambda: |-
          id(daikin).send_raw(id(raw_command).state);
          

```

Replace the GPIOs with the pins used by your hardware.

---

## 🏠 Home Assistant

Once connected to ESPHome, the Daikin unit appears as a standard Home Assistant climate entity.

For example:

```text
Daikin
├── Power
├── Mode
├── Target temperature
├── Fan mode
├── Swing
├── Boost
└── LED
```

This allows the Daikin unit to be controlled using the normal Home Assistant climate services and automations.

---

## 🧩 Architecture

The component is structured around three main layers:

```text
                 ┌─────────────────────┐
                 │    Home Assistant   │
                 └──────────┬──────────┘
                            │
                     ESPHome Climate
                            │
                 ┌──────────▼──────────┐
                 │   DaikinCNWired     │
                 │                     │
                 │  Current State      │
                 │  Desired State      │
                 │  Command Handling   │
                 └──────────┬──────────┘
                            │
                     CN-WIRED Protocol
                            │
                 ┌──────────▼──────────┐
                 │     Daikin HVAC     │
                 └─────────────────────┘
```

The component maintains both the **current state** received from the Daikin unit and the **desired state** requested by the user.

This makes it possible to queue changes and transmit only the necessary commands to the CN-WIRED bus.

---

## 🧪 Development

This project is primarily developed and tested with:

* ESPHome
* ESP32
* ESP-IDF
* Daikin CN-WIRED

The protocol implementation is still under active development.

Some CN-WIRED commands and features may not yet be completely understood or supported.

If you discover new packet formats or undocumented features, contributions and protocol observations are very welcome.

---

## 📡 Protocol Reverse Engineering

One of the goals of this project is to improve the understanding of the Daikin CN-WIRED protocol.

For example, some packets contain bit fields that represent multiple independent states:

```text
24 00 00 02 08 00 00 11
             │
             └── mode/state byte
```

Different bits can represent different pieces of information.

The protocol is being progressively documented as new commands and states are identified.

If you are experimenting with CN-WIRED packets, please consider sharing:

* The original packet
* The action performed on the controller
* The resulting packet
* The indoor unit model
* The wired controller model

This information can be extremely useful for further reverse engineering.

---

## ⚠️ Disclaimer

This is an **unofficial, community-developed project**.

It is not affiliated with, endorsed by, or supported by Daikin.

Use the hardware and software at your own risk.

Always verify the electrical characteristics of the CN-WIRED interface before connecting custom hardware.

---

## 📄 License

See the `LICENSE` file for the license applicable to this project.

---

## ⭐ Support the Project

If this project is useful to you:

* ⭐ Star the repository
* 🐛 Report bugs
* 💡 Share protocol discoveries
* 🔧 Submit pull requests
* 📖 Improve the documentation

And again, special thanks to **faikout** for the original CN-WIRED reverse-engineering work that made this project possible.

**Thank you, faikout! ❤️**

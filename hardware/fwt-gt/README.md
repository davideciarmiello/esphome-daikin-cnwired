## Hardware

The **Daikin CN-Wired ESPHome component** requires an ESP32-based board connected to the CN-Wired bus of the Daikin indoor unit.

### Recommended hardware

For this project, I use an **ESP32 D1 Mini**.

Tested on climate FWT04GATNMV1 FWT-GT.

The communication with the Daikin CN-Wired bus is handled using software RX/TX pins:

* **RX:** GPIO25
* **TX:** GPIO27

No level shifter is required for this setup.

### Wiring

The following diagrams show the recommended wiring between the ESP32 D1 Mini and the Daikin CN-Wired interface.

![ESP32 connection](esp32-connection.jpeg)

The CN-Wired connector and its position on the FWT-GT control board are shown below:

![FWT-GT connectors](fwt-gt-connectors.jpeg)

> **Note:** The RX and TX pins are software-configurable, so different GPIOs can be used if required by your hardware setup. The examples in this project use GPIO25 and GPIO27.

### Hardware overview

| Component     | Description         |
| ------------- | ------------------- |
| ESP32 board   | ESP32 D1 Mini       |
| RX            | GPIO25              |
| TX            | GPIO27              |
| Level shifter | **Not required**    |
| Communication | Daikin CN-Wired bus |

The ESP32 communicates directly with the CN-Wired interface, without an external level-shifting circuit.

### CN_WIR Connector

The Daikin `CN_WIR` connector uses a **JST PH series connector**, with a **2.0 mm pitch** and **5 pins**.

For the cable side, use a:

* **JST PH 2.0 mm, 5-pin female housing**
* Housing: **JST PHR-5**

The corresponding connector on the Daikin PCB is the **male JST PH header**. The official JST documentation confirms the PHR-5 housing and SPH-002T-P0.5S contact for the PH series.

#### Pinout

| CN_WIR Pin | Function      | Faikin Pin |
| ---------: | ------------- | ---------: |
|          1 | 5V reference  |          1 |
|          2 | Data → Daikin |          3 |
|          3 | GND           |          5 |
|          4 | Data → Faikin |          2 |
|          5 | ~12V power    |          4 |

> **Note:** The pin numbering above refers to the `CN_WIR` connector on the Daikin indoor unit PCB. Do not assume that the pin order is the same as the Daikin S21 connector.

The `CN_WIR` interface uses separate TX/RX data lines and provides both a 5V logic reference and a higher-voltage power supply.

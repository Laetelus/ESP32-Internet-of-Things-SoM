# ESP32-Internet-of-Things-SoM

![alt text](https://github.com/MateoSegura/ESP32-Internet-of-Things-SoM/blob/main/images/65820403583__40BDBBA1-D261-430F-ABA7-4362D48FF893.jpg)

A versatile IoT development platform, designed to accelerate development time, and create performant microcontroller embedded applications.

This board combines hardware & sensors that are widely used in industrial, and internet of things applications, where a right balance of power & performance are required.

Some of the feature of the SoM are:

- Dual core MCU ESP32
- MCP2518FD CAN 2.0B/FD Controller
    - Up to 5Mb/s
    - Software selectable 120 Ohms termination
- AD7689 16-bit ASR Analog to Digital (ADC) converter
    - 8-Inputs
    - Up to 150K samples/second
- MPU9250 Internal Motion Unit (IMU)
    - Accelerometer, Gyroscope & Magnemometer
- BME688 Environmental sensor
    - Temperature, Humidity & Barometric Pressure
- RV-3028 Real Time Clock (RTC)
- Up to 128Gb eMMC memory

This hardware is combined in a System on Module (SoM) with a small footprint, in a 4-layer board, and all signals are routed out through high density connectors.



The reason for this board was to create the bases of a eco-system on electronic control units for a variety of products in the automotive/industrial sector. This allows me to re-use a big percentage of the software across all of these projects.


## Hardware support

The schematics for the module itself are not given. However, the documentation below has diagrams & connections for each hardware system, along with explanations, examples & notes  to help you integrate any of these system in your own design.

[ESP32 SoM Hardware Documentation](https://www.notion.so/ESP32-SoM-Hardware-Documentation-87b4bec93dd54ebaa275c7d341c4cf4d)

## Software support

The ESP32 Utilities Libraries were created to abstract & ease a lot of the over head code that is needed when using any of the hardware in this board, such as:

- Debugging
- Hardware Initialization
- Error handling

These libraries shorten development time, and enable the developer to spend more time creating the logic of their application.

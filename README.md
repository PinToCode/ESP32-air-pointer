# ESP32 Air Pointer

A high-precision Bluetooth Low Energy (BLE) Air Pointer built using the ESP32-C3 SuperMini and MPU9250 IMU. The project enables natural hand-controlled mouse movement using gyroscope data and gesture-based touch controls.

---

## Overview

Traditional computer mice rely on optical sensors and require a surface. This project replaces the optical sensor with an Inertial Measurement Unit (IMU), allowing the cursor to be controlled entirely through hand movements.

The ESP32-C3 acts as a Bluetooth HID mouse, while the MPU9250 provides real-time gyroscope data for smooth cursor movement. A TTP223 capacitive touch sensor is used for intuitive gesture-based mouse actions such as clicking, dragging, scrolling, and right-clicking.

The software includes filtering, dead-zone removal, precision scaling, and motion smoothing to provide a stable and responsive user experience.

---

## Features

- Bluetooth Low Energy (BLE) HID Mouse
- High-precision gyroscope cursor movement
- Automatic gyroscope calibration during startup
- Exponential low-pass filtering for smoother movement
- Dead-zone filtering to remove sensor noise
- Non-linear precision response curve
- Sub-pixel movement accumulation
- Gesture-controlled mouse actions
- Scroll mode using hand movement
- Click-and-drag support
- 125 Hz update rate for low latency
- Lightweight and battery-powered design

---

## Hardware Used

| Component | Description |
|------------|------------|
| ESP32-C3 SuperMini | Main microcontroller |
| MPU9250 | 9-Axis IMU |
| TTP223 | Capacitive Touch Sensor |
| 3.7V 400mAh Li-Po Battery | Portable power source |
| TP4056 Charging Module | Battery charging and protection |

---

## Power Supply

The ESP32 Air Pointer is designed to be fully portable and is powered by a **3.7V 400mAh rechargeable Lithium Polymer (Li-Po) battery**.

Battery charging is handled by a **TP4056 USB charging module** with battery protection.

To match the charging current with the 400mAh battery, the **R3 (PROG) resistor** on the TP4056 module was replaced with a **6.8 kΩ resistor** (measured approximately **6.7 kΩ**). This reduces the charging current to approximately **180 mA**, providing safer charging and helping extend battery life.

---

### Power Components

| Component | Specification |
|----------|---------------|
| Battery | 3.7V 400mAh Rechargeable Li-Po Battery |
| Charging Module | TP4056 USB Li-Po Charger |
| Modified PROG (R3) Resistor | 6.8 kΩ (Measured ≈ 6.7 kΩ) |
| Approximate Charging Current | ~180 mA |
| Power Output | 3.7V Nominal (4.2V Fully Charged) |

## Gesture Controls

| Gesture | Action |
|---------|--------|
| Single Tap | Left Click |
| Double Tap | Double Left Click |
| Triple Tap | Right Click |
| Tap & Hold | Left Click & Drag |
| Double Tap & Hold | Scroll Mode |

---

## Motion Processing Pipeline

```
MPU9250 Gyroscope
        │
        ▼
Bias Calibration
        │
        ▼
Dead-zone Removal
        │
        ▼
Precision Curve
        │
        ▼
Low-pass Filter
        │
        ▼
Sub-pixel Accumulation
        │
        ▼
BLE HID Mouse Report
        │
        ▼
Computer Cursor
```

---

## Wiring

| ESP32-C3 SuperMini | MPU9250 |
|--------------------|---------|
| GPIO4 | SDA |
| GPIO5 | SCL |
| 3.3V | VCC |
| GND | GND |

### Touch Sensor

| ESP32-C3 | TTP223 |
|-----------|---------|
| GPIO3 | SIG |
| 3.3V | VCC |
| GND | GND |

---

## Software Requirements

- Arduino IDE
- ESP32 Arduino Core
- Wire Library
- HijelHID BLE Mouse Library

---

## Installation

1. Clone the repository.

```bash
git clone https://github.com/yourusername/esp32-air-pointer.git
```

2. Open the project in Arduino IDE.

3. Install the required ESP32 board package.

4. Install the HijelHID BLE Mouse library.

5. Connect the ESP32-C3.

6. Select the correct COM port.

7. Upload the sketch.

8. Pair the device with your computer using Bluetooth.

---

## How It Works

The MPU9250 continuously measures angular velocity around the X and Z axes.

The firmware:

- Reads raw gyroscope data
- Removes calibration offsets
- Applies a dead-zone filter
- Passes the signal through a non-linear precision curve
- Smooths movement using an exponential filter
- Accumulates sub-pixel motion
- Sends cursor updates over Bluetooth HID

Touch gestures are processed independently to trigger mouse events including clicking, dragging, and scrolling.

---

## Project Structure

```
ESP32-Air-Pointer/
│
├── ESP32_Air_Pointer.ino
├── README.md
├── LICENSE
├── images/
│   ├── hardware.jpg
│   ├── wiring.png
│   ├── demo.gif
│   └── airpointer.jpg
│
└── docs/
    └── schematic.pdf
```

---

## Future Improvements

- Battery level indication
- BLE configuration app
- Adjustable sensitivity
- OTA firmware updates
- Sleep mode
- Sensor fusion using Madgwick filter
- Multi-device Bluetooth pairing
- Rechargeable battery management

---

## Applications

- Air Mouse
- Smart TV Control
- Presentation Controller
- Accessibility Device
- VR Interaction
- Robotics Interface
- Media Control
- DIY Embedded Projects

---

## License

This project is licensed under the MIT License.

---

## Author

**Jerit Jose**

Embedded Systems | BLE & IoT Engineer

GitHub: https://github.com/yourusername

LinkedIn: https://linkedin.com/in/yourprofile

---

## Contributing

Contributions, suggestions, and improvements are welcome.

If you find a bug or have an idea for a new feature, feel free to open an issue or submit a pull request.

---

## Acknowledgements

- Espressif Systems
- Arduino
- MPU9250 Community
- Bluetooth SIG

# Thrust-Measurement-Test-Stand

# 🚀 BLDC Motor Thrust Measurement Test Stand

A low-cost and accurate **BLDC Motor Thrust Measurement Test Stand** built using an **Arduino Mega 2560**, **HX711 Load Cell Amplifier**, **PCA9685 PWM Driver**, and an **Infrared RPM Sensor**.

The system measures:

- ✅ Thrust (g and N)
- ✅ Motor RPM
- ✅ ESC Throttle (%)
- ✅ Automatic performance testing
- ✅ Serial command interface

This project is intended for UAV, drone propulsion, and BLDC motor performance testing.

---

# Features

- Real-time thrust measurement
- Real-time RPM calculation using interrupt-based IR sensing
- ESC control through PCA9685
- Automatic throttle sweep (0–100%)
- CSV output for Excel or MATLAB
- Watchdog safety system
- Load cell taring
- Serial command interface

---

# Hardware Requirements

| Component | Description |
|-----------|-------------|
| Arduino Mega 2560 | Main controller |
| HX711 | Load cell amplifier |
| Load Cell | Thrust measurement |
| PCA9685 | 16-channel PWM driver |
| ESC | Electronic Speed Controller |
| BLDC Motor | Test motor |
| IR Sensor | RPM measurement |
| LiPo Battery | Motor power supply |

---

# Wiring

## HX711

| HX711 | Arduino Mega |
|--------|--------------|
| DT | D3 |
| SCK | D2 |

---

## IR Sensor

| IR Sensor | Arduino Mega |
|-----------|--------------|
| OUT | D19 |
| VCC | 5V |
| GND | GND |

---

## PCA9685

| PCA9685 | Arduino Mega |
|----------|--------------|
| SDA | SDA (20) |
| SCL | SCL (21) |
| VCC | 5V |
| GND | GND |

ESC Signal → PCA9685 Channel 0

---

# Libraries

Install the following Arduino libraries before compiling:

- HX711
- Adafruit PWM Servo Driver Library
- Wire (built-in)

---

# Serial Commands

| Command | Description |
|---------|-------------|
| arm | Arm the ESC |
| disarm | Stop the motor |
| tare | Zero the load cell |
| status | Display current status |
| help | Show command list |
| test | Automatic throttle sweep |
| 0–100 | Set throttle percentage |

Example:

```
arm
50
status
test
```

---

# Automatic Test Mode

Running:

```
test
```

performs:

- 0–100% throttle sweep
- 5% throttle increments
- Stabilization delay
- Sample averaging
- Automatic CSV output

Example output:

```
Throttle(%),RPM,Thrust(g)
0,0,0
5,1200,15.2
10,2450,42.6
15,3900,81.4
...
100,21500,965.3
```

The CSV output can be imported directly into:

- Microsoft Excel
- MATLAB
- Python
- Origin
- GNUplot

---

# RPM Calculation

The infrared sensor generates pulses that are captured using hardware interrupts.

The motor speed is calculated as:

\[
RPM=\frac{60\times10^6}{\Delta t \times N_p}
\]

where

- Δt = pulse interval (µs)
- Nₚ = pulses per revolution

---

# Thrust Calculation

The HX711 converts the load cell signal into force.

Measured thrust is displayed as:

- grams (g)
- Newtons (N)

Conversion:

\[
T(N)=T(g)\times0.00981
\]

---

# Safety Features

- ESC starts at 1000 µs
- Manual arming required
- Automatic watchdog timeout
- Motor shutdown on communication loss
- Load cell tare before testing

---

# Repository Structure

```
.
├── README.md
├── BLDC_Test_Stand.ino
├── LICENSE
└── images
    ├── wiring.png
    ├── test_stand.jpg
    └── setup.png
```

---

# Future Improvements

- OLED display support
- SD card data logging
- OLED RPM graph
- PID-based thrust control
- Bluetooth control
- Wi-Fi dashboard
- Temperature sensing
- Torque measurement

---

# Applications

- UAV propulsion testing
- Drone motor characterization
- ESC calibration
- Propeller comparison
- Research laboratories
- Educational demonstrations

---

# License

This project is released under the MIT License.

---

# Authors

Developed by:

**Harshit Kumar Sahu**  
PhD Scholar  
Indian Institute of Technology Bhilai

---

# Acknowledgements

Special thanks to the open-source Arduino community and the developers of the HX711 and Adafruit PCA9685 libraries.

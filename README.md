# 🤖 PETG Robotic Arm Controller (ESP8266 + PCA9685)

A high-performance, web-based controller for a 4-axis robotic arm. Built for **ESP-12E (NodeMCU)**, it utilizes the **PCA9685** PWM driver to manage high-torque **MG996R** servos and a precision **SG90** micro-servo for the gripper.

## 🚀 Features

* **Smooth Motion Engine:** Custom logic to prevent jerky movements; servos move in small increments with adjustable speed.
* **Web Dashboard:** Mobile-responsive UI with real-time sliders and feedback.
* **Safety First:** One-touch **Emergency Stop** that kills the PWM signal instantly to protect the PETG structure and motors.
* **Precision Calibration:** Individual pulse-width mapping for different servo models (MG996R vs SG90).
* **Speed Control:** Dynamic adjustment of movement delay (1ms to 100ms) via the web interface.

## 🛠 Hardware Required

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| **ESP-12E (NodeMCU)** | 1 | WiFi Microcontroller |
| **PCA9685** | 1 | 16-Channel 12-bit PWM Driver |
| **MG996R** | 3 | High-Torque Servos (Base, Elbow, Shoulder) |
| **SG90** | 1 | Micro Servo (Gripper) |
| **WANPTEK GA3010H** | 1 | DC Power Supply (Set to 6.0V) |

## 🔌 Wiring Diagram

### ESP8266 to PCA9685 (I2C)
* **3.3V** -> VCC
* **GND** -> GND
* **D1 (GPIO 5)** -> SCL
* **D2 (GPIO 4)** -> SDA

### Power Supply (WANPTEK)
* **(+) 6.0V** -> PCA9685 V+ Terminal Block
* **(-) GND** -> PCA9685 GND Terminal Block

### Servo Mapping
* **Port 0:** MG996R (Base)
* **Port 1:** MG996R (Elbow)
* **Port 2:** MG996R (Shoulder)
* **Port 3:** SG90 (Gripper)

## 💻 Installation

1.  Install the **Adafruit PWM Servo Driver Library** in your Arduino IDE.
2.  Update the `ssid` and `password` variables in the code with your WiFi credentials.
3.  Flash the code to your ESP8266.
4.  Open the Serial Monitor (115200 baud) to find the **IP Address**.
5.  Enter the IP in any web browser on the same network.

## ⚠️ Safety Notes
* **Voltage:** Do not exceed 7.2V on the PCA9685 power terminal. 6.0V is the "sweet spot" for performance and longevity.
* **Calibration:** If your servos "hum" at the ends of their travel, adjust `MG996_MIN/MAX` or `SG90_MIN/MAX` in the code to avoid mechanical stall.
* **Current:** MG996R servos can draw up to 2.5A each under load. Ensure your WANPTEK supply is set to at least 5A-10A limit.

---
*Created for the PETG Robotic Arm project.*

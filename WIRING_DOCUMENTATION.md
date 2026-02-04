# 📋 DOKUMENTASI WIRING LENGKAP - ESP-NOW RC Robot System

## 📖 Daftar Isi
1. [Ringkasan Proyek](#ringkasan-proyek)
2. [Arsitektur Sistem](#arsitektur-sistem)
3. [Master Device (Robot) Wiring](#master-device-robot-wiring)
4. [Transmitter (Remote) Wiring](#transmitter-remote-wiring)
5. [ESP-NOW Receiver Wiring](#esp-now-receiver-wiring)
6. [Sensor Tambahan](#sensor-tambahan)
7. [⚠️ KONFLIK & PERBEDAAN WIRING](#️-konflik--perbedaan-wiring)
8. [Diagram Koneksi Visual](#diagram-koneksi-visual)
9. [Referensi File per Komponen](#referensi-file-per-komponen)

---

## Ringkasan Proyek

Proyek ini adalah sistem robot RC berbasis ESP32-S3 menggunakan protokol ESP-NOW untuk komunikasi wireless peer-to-peer. Sistem terdiri dari:

| Komponen | Deskripsi | MCU |
|----------|-----------|-----|
| **Master (RC)** | Robot 4WD dengan Mecanum wheel, OLED, buzzer, buttons | ESP32-S3 |
| **Slave (Remote)** | Remote dengan 1 joystick analog & Mic INMP441 | ESP32-S3 |

---

## Arsitektur Sistem

```
┌─────────────────────┐                    ┌─────────────────────┐
│     SLAVE           │     ESP-NOW        │      MASTER         │
│    (Remote)         │ ◄────────────────► │      (Robot)        │
│                     │                    │                     │
│  • 1 Joystick       │                    │  • 4 Motor (2 L298N)│
│  • INMP441 (mic)    │                    │  • OLED Display     │
│  • 3x Button        │                    │  • Buzzer           │
│  • LCD Display      │                    │  • 3x Button        │
│  • 2x Battery 18650 │                    │  • 3x Battery 18650 │
└─────────────────────┘                    └─────────────────────┘
```

---

## Master Device (Robot) Wiring

### 🔌 Pin Summary (ESP32-S3)

| GPIO | Fungsi | Komponen | Catatan |
|------|--------|----------|---------|
| **4** | PWM (ENA) | Motor Front Left | L298N Depan |
| **14** | Direction (IN1) | Motor Front Left | L298N Depan |
| **21** | Direction (IN2) | Motor Front Left | L298N Depan |
| **5** | PWM (ENB) | Motor Front Right | L298N Depan |
| **16** | Direction (IN3) | Motor Front Right | L298N Depan |
| **17** | Direction (IN4) | Motor Front Right | L298N Depan |
| **18** | PWM (ENA) | Motor Back Left | L298N Belakang |
| **19** | Direction (IN1) | Motor Back Left | L298N Belakang |
| **20** | Direction (IN2) | Motor Back Left | L298N Belakang |
| **7** | PWM (ENB) | Motor Back Right | L298N Belakang |
| **15** | Direction (IN3) | Motor Back Right | L298N Belakang |
| **8** | Direction (IN4) | Motor Back Right | L298N Belakang |
| **9** | I2C SDA | OLED Display | SSD1306 |
| **10** | I2C SCK | OLED Display | SSD1306 |
| **11** | PWM | Buzzer | Active/Passive |
| **12** | Input (Pull-up) | Button UP | Navigation |
| **13** | Input (Pull-up) | Button DOWN | Navigation |
| **38** | Input (Pull-up) | Button OK | Select/Back |

### 🎮 Motor Driver L298N - DEPAN

```
        L298N DEPAN
    ┌─────────────────┐
    │                 │
    │ +12V ───── Baterai 12V
    │ GND ────── GND (shared)
    │ 5V ─────── ESP32 VIN
    │                 │
    │ ENA ────── GPIO4  (PWM FL)
    │ IN1 ────── GPIO14 (DIR FL)
    │ IN2 ────── GPIO21 (DIR FL)
    │ OUT1 ──┬── Motor FL (+)
    │ OUT2 ──┴── Motor FL (-)
    │                 │
    │ ENB ────── GPIO5  (PWM FR)
    │ IN3 ────── GPIO16 (DIR FR)
    │ IN4 ────── GPIO17 (DIR FR)
    │ OUT3 ──┬── Motor FR (+)
    │ OUT4 ──┴── Motor FR (-)
    │                 │
    └─────────────────┘
```

### 🎮 Motor Driver L298N - BELAKANG

```
        L298N BELAKANG
    ┌─────────────────┐
    │                 │
    │ +12V ───── Baterai 12V
    │ GND ────── GND (shared)
    │                 │
    │ ENA ────── GPIO18 (PWM BL)
    │ IN1 ────── GPIO19 (DIR BL)
    │ IN2 ────── GPIO20 (DIR BL)
    │ OUT1 ──┬── Motor BL (+)
    │ OUT2 ──┴── Motor BL (-)
    │                 │
    │ ENB ────── GPIO7  (PWM BR)
    │ IN3 ────── GPIO15 (DIR BR)
    │ IN4 ────── GPIO8  (DIR BR)
    │ OUT3 ──┬── Motor BR (+)
    │ OUT4 ──┴── Motor BR (-)
    │                 │
    └─────────────────┘
```

### 📺 OLED Display SSD1306 (I2C)

```
    OLED SSD1306
    ┌────────────┐
    │ VCC ─────── 3.3V
    │ GND ─────── GND
    │ SDA ─────── GPIO9
    │ SCL ─────── GPIO10
    └────────────┘
    
    I2C Address: 0x3C
    Resolution: 128x64
```

### 🔊 Buzzer

```
    BUZZER
    ┌────────────┐
    │ (+) ─────── GPIO11
    │ (-) ─────── GND
    └────────────┘
    
    PWM Channel: 4
    Frequency: Variable (tone)
```

### 🔘 Buttons (Active LOW dengan Pull-up)

```
    ┌────────────┐
    │ UP ──────── GPIO12 ──┬── GND
    │ DOWN ────── GPIO13 ──┤
    │ OK ──────── GPIO38 ──┘
    └────────────┘
    
    Mode: INPUT_PULLUP
    Active: LOW (pressed)
```

---

## Slave (Remote) Wiring

### 🔌 Pin Summary (ESP32-S3)

| GPIO | Fungsi | Komponen | Catatan |
|------|--------|----------|---------|
| **1** | ADC | Joystick 1 X | Steering |
| **2** | ADC | Joystick 1 Y | Throttle |
| **42** | Input (Pull-up) | Joystick 1 Button | SW |
| **4** | ADC | Joystick 2 X | Auxiliary |
| **5** | ADC | Joystick 2 Y | Auxiliary |
| **6** | Input (Pull-up) | Joystick 2 Button | SW |
| **9** | I2C SDA | MPU6050 | Optional |
| **10** | I2C SCL | MPU6050 | Optional |
| **3** | I2S SCK | INMP441 Mic | Optional |
| **40** | I2S WS | INMP441 Mic | Optional |
| **41** | I2S SD | INMP441 Mic | Optional |
| **48** | Output | LED Status | Built-in |

### 🕹️ Joystick 1 (Kiri - Throttle/Steering)

```
    JOYSTICK 1 (KY-023)
    ┌────────────────┐
    │ VCC ────────── 3.3V
    │ GND ────────── GND
    │ VRx ────────── GPIO1 (Steering)
    │ VRy ────────── GPIO2 (Throttle)
    │ SW ─────────── GPIO42 (Button)
    └────────────────┘
    
    ADC Resolution: 12-bit (0-4095)
    Invert: X=false, Y=true
```

### 🎤 INMP441 Microphone (Optional - Voice Mode)

```
    INMP441
    ┌────────────────┐
    │ VCC ────────── 3.3V
    │ GND ────────── GND
    │ SCK ────────── GPIO3
    │ WS ─────────── GPIO40
    │ SD ─────────── GPIO41
    │ L/R ────────── GND (Left channel)
    └────────────────┘
    
    Interface: I2S
```

---


## Diagram Koneksi Visual

### Master Robot - Wiring Overview

```
                     ┌───────────────────────────────────────┐
                     │            ESP32-S3 MASTER            │
                     │                                       │
    ┌─────────────┐  │                                       │  ┌─────────────┐
    │  L298N      │  │                                       │  │  L298N      │
    │  DEPAN      │  │                                       │  │  BELAKANG   │
    │             │  │                                       │  │             │
    │ ENA ────────┼──┤ GPIO4                          GPIO18 ├──┼── ENA       │
    │ IN1 ────────┼──┤ GPIO14                         GPIO19 ├──┼── IN1       │
    │ IN2 ────────┼──┤ GPIO21                         GPIO20 ├──┼── IN2       │
    │ ENB ────────┼──┤ GPIO5                           GPIO7 ├──┼── ENB       │
    │ IN3 ────────┼──┤ GPIO16                         GPIO15 ├──┼── IN3       │
    │ IN4 ────────┼──┤ GPIO17                          GPIO8 ├──┼── IN4       │
    │             │  │                                       │  │             │
    │ 5V ─────────┼──┤ 5V (VIN)                              │  │             │
    │ GND ────────┼──┤ GND ──────────────────────────────────┼──┼── GND       │
    │             │  │                                       │  │             │
    │ +12V ───────┼──┴──────────BATERAI 12V──────────────────┼──┼── +12V      │
    │             │  │                                       │  │             │
    └─────────────┘  │                                       │  └─────────────┘
                     │                                       │
    ┌─────────────┐  │                                       │  ┌─────────────┐
    │ OLED SSD1306│  │                                       │  │   BUZZER    │
    │             │  │                                       │  │             │
    │ VCC ────────┼──┤ 3.3V                                  │  │ (+) ────────┤ GPIO11
    │ GND ────────┼──┤ GND                                   │  │ (-) ────────┤ GND
    │ SDA ────────┼──┤ GPIO9                                 │  └─────────────┘
    │ SCL ────────┼──┤ GPIO10                                │
    └─────────────┘  │                                       │  ┌─────────────┐
                     │                                       │  │   BUTTONS   │
    ┌─────────────┐  │                                       │  │             │
    │  LED STATUS │  │                                       │  │ UP ─────────┤ GPIO12
    │             │  │                                       │  │ DOWN ───────┤ GPIO13
    │ (+) ────────┼──┤ GPIO48                                │  │ OK ─────────┤ GPIO38
    │ (-) ────────┼──┤ GND                                   │  └─────────────┘
    └─────────────┘  │                                       │
                     └───────────────────────────────────────┘
```

### Slave Remote - Wiring Overview

```
                     ┌───────────────────────────────────────┐
                     │          ESP32-S3 TRANSMITTER         │
                     │                                       │
    ┌─────────────┐  │                                       │  ┌─────────────┐
    │ JOYSTICK 1  │  │                                       │  │ JOYSTICK 2  │
    │ (KIRI)      │  │                                       │  │ (KANAN)     │
    │             │  │                                       │  │             │
    │ VCC ────────┼──┤ 3.3V                            3.3V ├──┼── VCC       │
    │ GND ────────┼──┤ GND                              GND ├──┼── GND       │
    │ VRx ────────┼──┤ GPIO1 (Steering)          (Aux X) GPIO4 ├──┼── VRx       │
    │ VRy ────────┼──┤ GPIO2 (Throttle)          (Aux Y) GPIO5 ├──┼── VRy       │
    │ SW ─────────┼──┤ GPIO42                         GPIO6 ├──┼── SW        │
    │             │  │                                       │  │             │
    └─────────────┘  │                                       │  └─────────────┘
                     │                                       │
    ┌─────────────┐  │                                       │  ┌─────────────┐
    │ MPU6050     │  │                                       │  │ INMP441     │
    │ (Optional)  │  │                                       │  │ (Optional)  │
    │             │  │                                       │  │             │
    │ VCC ────────┼──┤ 3.3V                            3.3V ├──┼── VCC       │
    │ GND ────────┼──┤ GND                              GND ├──┼── GND       │
    │ SDA ────────┼──┤ GPIO9                          GPIO3 ├──┼── SCK       │
    │ SCL ────────┼──┤ GPIO10                        GPIO40 ├──┼── WS        │
    │             │  │                               GPIO41 ├──┼── SD        │
    └─────────────┘  │                                       │  └─────────────┘
                     │                                       │
    ┌─────────────┐  │                                       │
    │  LED STATUS │  │                                       │
    │             │  │                                       │
    │ (+) ────────┼──┤ GPIO48                                │
    │ (-) ────────┼──┤ GND                                   │
    └─────────────┘  │                                       │
                     └───────────────────────────────────────┘
```

---



## 📋 Quick Reference Card

### ESP32-S3 Master (Production)
```
MOTOR FL: ENA=4, IN1=14, IN2=21
MOTOR FR: ENB=5, IN3=16, IN4=17
MOTOR BL: ENA=18, IN1=19, IN2=20
MOTOR BR: ENB=7, IN3=15, IN4=8
OLED: SDA=9, SCL=10 (0x3C)
BUZZER: GPIO11
BTN: UP=12, DOWN=13, OK=38
LED: GPIO48
```

### ESP32-S3 Transmitter
```
JOY1: X=1, Y=2, SW=42
JOY2: X=4, Y=5, SW=6
MPU6050: SDA=9, SCL=10 (0x68)
MIC: SCK=3, WS=40, SD=41
LED: GPIO48
```

---

*Terakhir diperbarui: 2026-02-04*

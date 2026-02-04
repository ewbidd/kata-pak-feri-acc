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
| **Master (Robot)** | Robot 4WD dengan Mecanum wheel, OLED, buzzer, buttons | ESP32-S3 |
| **Transmitter (Remote)** | Remote dengan 2 joystick analog | ESP32-S3 |

---

## Arsitektur Sistem

```
┌─────────────────────┐                    ┌─────────────────────┐
│    TRANSMITTER      │     ESP-NOW        │      MASTER         │
│    (Remote)         │ ◄────────────────► │      (Robot)        │
│                     │                    │                     │
│  • 2x Joystick      │                    │  • 4x Motor (L298N) │
│  • MPU6050 (opt)    │                    │  • OLED Display     │
│  • INMP441 (opt)    │                    │  • Buzzer           │
│  • LED Status       │                    │  • 3x Button        │
│                     │                    │  • LED Status       │
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
| **10** | I2C SCL | OLED Display | SSD1306 |
| **11** | PWM | Buzzer | Active/Passive |
| **12** | Input (Pull-up) | Button UP | Navigation |
| **13** | Input (Pull-up) | Button DOWN | Navigation |
| **38** | Input (Pull-up) | Button OK | Select/Back |
| **48** | Output | LED Status | Built-in LED |

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

### 💡 LED Status

```
    LED STATUS
    ┌────────────┐
    │ (+) ─────── GPIO48
    │ (-) ─────── GND (via resistor 220Ω)
    └────────────┘
```

---

## Transmitter (Remote) Wiring

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

### 🕹️ Joystick 2 (Kanan - Auxiliary)

```
    JOYSTICK 2 (KY-023)
    ┌────────────────┐
    │ VCC ────────── 3.3V
    │ GND ────────── GND
    │ VRx ────────── GPIO4 (Aux X)
    │ VRy ────────── GPIO5 (Aux Y)
    │ SW ─────────── GPIO6 (Button)
    └────────────────┘
    
    ADC Resolution: 12-bit (0-4095)
    Invert: X=true, Y=true
```

### 📐 MPU6050 (Optional - Hand Gesture Mode)

```
    MPU6050
    ┌────────────────┐
    │ VCC ────────── 3.3V
    │ GND ────────── GND
    │ SDA ────────── GPIO9
    │ SCL ────────── GPIO10
    │ INT ────────── (not connected)
    └────────────────┘
    
    I2C Address: 0x68
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

## ESP-NOW Receiver Wiring

> ⚠️ **PERHATIAN**: Wiring ini khusus untuk `espnow_receiver` (ESP32 standar, bukan S3)

### 🔌 Pin Summary (ESP32 Standard)

| GPIO | Fungsi | Komponen | Catatan |
|------|--------|----------|---------|
| **25** | PWM (ENA) | Motor Front Left | L298N #1 |
| **26** | Direction (IN1) | Motor Front Left | - |
| **27** | Direction (IN2) | Motor Front Left | - |
| **14** | PWM (ENB) | Motor Front Right | L298N #1 |
| **12** | Direction (IN3) | Motor Front Right | - |
| **13** | Direction (IN4) | Motor Front Right | - |
| **32** | PWM (ENA) | Motor Rear Left | L298N #2 |
| **33** | Direction (IN1) | Motor Rear Left | - |
| **15** | Direction (IN2) | Motor Rear Left | - |
| **4** | PWM (ENB) | Motor Rear Right | L298N #2 |
| **16** | Direction (IN3) | Motor Rear Right | - |
| **17** | Direction (IN4) | Motor Rear Right | - |

---

## Sensor Tambahan

### 🔊 Ultrasonic Sensor HC-SR04 (Mode Autonomous)

```
    HC-SR04
    ┌────────────────┐
    │ VCC ────────── 5V
    │ GND ────────── GND
    │ TRIG ───────── GPIO2 (autnomus.ino) / GPIO11 (object_avoid.ino)
    │ ECHO ───────── GPIO9 (autnomus.ino) / GPIO38 (object_avoid.ino)
    └────────────────┘
```

### 🔄 Servo (Mode Autonomous)

```
    SERVO
    ┌────────────────┐
    │ VCC ────────── 5V
    │ GND ────────── GND
    │ Signal ─────── GPIO6 (autnomus.ino) / GPIO13 (object_avoid.ino)
    └────────────────┘
```

### 🔋 Battery Monitor (Mode Autonomous)

```
    VOLTAGE DIVIDER
    ┌────────────────────────────────┐
    │                                │
    │ Battery 12V ──┬── R1 (100K)    │
    │               │                │
    │               ├──────── GPIO10 │
    │               │                │
    │ GND ─────────┴── R2 (10K)     │
    │                                │
    └────────────────────────────────┘
```

---

## ⚠️ KONFLIK & PERBEDAAN WIRING

### 🔴 KONFLIK KRITIS

#### 1. **OLED I2C Pin - Konflik Antara File Test dan File Utama**

| File | SDA | SCL |
|------|-----|-----|
| `mini_os_v1/main/config.h` | GPIO9 | GPIO10 |
| `mini_os/common/config.h` | GPIO9 | GPIO10 |
| `master_wiring_test.ino` | GPIO9 | GPIO10 |
| `brightness_test.ino` | GPIO9 | GPIO10 |
| `led_test.ino` | GPIO9 | GPIO10 |
| **`joystick_test.ino`** ⚠️ | **GPIO8** | **GPIO9** |

> **❌ KONFLIK**: `joystick_test.ino` menggunakan GPIO8 untuk SDA dan GPIO9 untuk SCL, berbeda dengan semua file lainnya!

#### 2. **ESP-NOW Receiver vs Master - Pin Motor BERBEDA TOTAL**

| Motor | Master (ESP32-S3) | ESP-NOW Receiver (ESP32) |
|-------|-------------------|--------------------------|
| FL ENA | GPIO4 | **GPIO25** |
| FL IN1 | GPIO14 | **GPIO26** |
| FL IN2 | GPIO21 | **GPIO27** |
| FR ENB | GPIO5 | **GPIO14** |
| FR IN3 | GPIO16 | **GPIO12** |
| FR IN4 | GPIO17 | **GPIO13** |
| BL ENA | GPIO18 | **GPIO32** |
| BL IN1 | GPIO19 | **GPIO33** |
| BL IN2 | GPIO20 | **GPIO15** |
| BR ENB | GPIO7 | **GPIO4** |
| BR IN3 | GPIO15 | **GPIO16** |
| BR IN4 | GPIO8 | **GPIO17** |

> **⚠️ PERHATIAN**: `espnow_receiver` menggunakan pin yang berbeda karena ditujukan untuk **ESP32 standar** (bukan ESP32-S3). Ini BUKAN konflik, tapi perbedaan platform yang disengaja.

#### 3. **Ultrasonic Sensor TRIG/ECHO - Perbedaan Antar Mode**

| File | TRIG | ECHO | Catatan |
|------|------|------|---------|
| `autnomus.ino` | GPIO2 | GPIO9 | ⚠️ ECHO bentrok dengan OLED SDA! |
| `object_avoid.ino` | GPIO11 | GPIO38 | ⚠️ TRIG bentrok dengan Buzzer! |

> **❌ KONFLIK SERIUS**:
> - `autnomus.ino`: ECHO menggunakan GPIO9 yang juga digunakan untuk OLED SDA
> - `object_avoid.ino`: TRIG menggunakan GPIO11 yang juga digunakan untuk Buzzer

#### 4. **Servo Pin - Perbedaan Antar Mode**

| File | Servo Pin | Catatan |
|------|-----------|---------|
| `autnomus.ino` | GPIO6 | OK |
| `object_avoid.ino` | GPIO13 | ⚠️ Bentrok dengan Button DOWN! |

> **❌ KONFLIK**: `object_avoid.ino` menggunakan GPIO13 untuk servo, yang juga digunakan untuk Button DOWN pada Master.

#### 5. **Battery Monitor Pin**

| File | Battery ADC Pin | Catatan |
|------|-----------------|---------|
| `autnomus.ino` | GPIO10 | ⚠️ Bentrok dengan OLED SCL! |

> **❌ KONFLIK SERIUS**: GPIO10 digunakan untuk battery monitoring padahal juga digunakan untuk OLED I2C SCL.

---

### 🟡 PERBEDAAN (Non-Konflik)

#### 1. **Button Configuration - Perbedaan Test vs Production**

| File | BTN_UP | BTN_DOWN | BTN_OK | BTN_BACK |
|------|--------|----------|--------|----------|
| `mini_os_v1/main/config.h` | GPIO12 | GPIO13 | GPIO38 | - |
| `master_ui_test.ino` | GPIO12 | GPIO13 | GPIO38 | - |
| `joystick_test.ino` | **GPIO4** | **GPIO5** | **GPIO6** | - |
| `WIRING_GUIDE.md` (lama) | GPIO12 | GPIO13 | GPIO38 | GPIO39 |

> **Catatan**: `joystick_test.ino` menggunakan pin berbeda (GPIO4, 5, 6) - ini mungkin untuk hardware test yang berbeda.

#### 2. **PWM Frequency**

| File | Motor PWM Freq | Catatan |
|------|----------------|---------|
| `mini_os_v1/main/config.h` | 5000 Hz | |
| `autnomus.ino` | 20000 Hz | |
| `rc.ino` | 20000 Hz | |
| `rc_mecanum.ino` | 20000 Hz | |
| `master_wiring_test.ino` | 20000 Hz | |

> **Catatan**: mini_os_v1 menggunakan 5kHz, file lainnya 20kHz. Tidak konflik fisik, tapi perlu konsistensi.

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

### Transmitter Remote - Wiring Overview

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

## Referensi File per Komponen

### Motor Control Files

| File | Path | Target |
|------|------|--------|
| `config.h` | `mini_os_v1/main/config.h` | ESP32-S3 Master |
| `motor.c` | `mini_os_v1/main/drivers/motor.c` | ESP32-S3 Master |
| `rc.ino` | `rc.ino` | ESP32-S3 Master |
| `rc_mecanum.ino` | `rc_mecanum/rc_mecanum.ino` | ESP32-S3 Master |
| `autnomus.ino` | `autnomus.ino` | ESP32-S3 Master |
| `object_avoid.ino` | `object_avoid/object_avoid.ino` | ESP32-S3 Master |
| `config.h` | `espnow_receiver/main/config.h` | ESP32 Standard |
| `motor_control.c` | `espnow_receiver/main/motor_control.c` | ESP32 Standard |

### Display & UI Files

| File | Path | Target |
|------|------|--------|
| `display.c` | `mini_os_v1/main/drivers/display.c` | ESP32-S3 Master |
| `master_ui_test.ino` | `master_ui_test/master_ui_test.ino` | ESP32-S3 Master |
| `brightness_test.ino` | `brightness_test/brightness_test.ino` | ESP32-S3 Master |
| `joystick_test.ino` | `joystick_test/joystick_test.ino` | ESP32-S3 (Different pins!) |

### Transmitter Files

| File | Path | Target |
|------|------|--------|
| `remote_transmitter.ino` | `remote_transmitter/remote_transmitter.ino` | ESP32-S3 Transmitter |
| `config.h` | `mini_os/common/config.h` | Shared definitions |

---

## 📝 Rekomendasi Perbaikan

### Prioritas Tinggi (Harus Diperbaiki)

1. **Fix `joystick_test.ino`** - Ubah pin I2C agar konsisten:
   ```cpp
   // Ubah dari:
   #define SDA_PIN 8
   #define SCL_PIN 9
   // Menjadi:
   #define SDA_PIN 9
   #define SCL_PIN 10
   ```

2. **Fix `autnomus.ino`** - Ubah pin ECHO agar tidak bentrok dengan OLED:
   ```cpp
   // Ubah dari:
   #define ECHO_PIN 9
   // Menjadi (gunakan pin yang tidak terpakai, misal):
   #define ECHO_PIN 3  // atau GPIO yang tersedia
   ```

3. **Fix `autnomus.ino`** - Ubah pin Battery Monitor:
   ```cpp
   // Ubah dari:
   #define BATTERY_PIN 10
   // Menjadi:
   #define BATTERY_PIN 1  // atau ADC pin yang tersedia
   ```

4. **Fix `object_avoid.ino`** - Ubah pin Servo dan TRIG:
   ```cpp
   // Servo bentrok dengan Button DOWN
   // TRIG bentrok dengan Buzzer
   // Ubah ke pin yang tersedia
   #define SERVO_PIN 3  // atau GPIO yang tersedia
   #define TRIG_PIN 2   // atau GPIO yang tersedia
   ```

### Prioritas Rendah (Konsistensi)

1. **Standarisasi PWM Frequency** - Pilih 5kHz atau 20kHz untuk semua file
2. **Update `WIRING_GUIDE.md`** - Hapus referensi ke GPIO39 (BTN_BACK) yang tidak lagi digunakan

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

*Dokumentasi ini dibuat secara otomatis dengan menganalisis semua file sumber dalam proyek.*
*Terakhir diperbarui: 2026-02-04*

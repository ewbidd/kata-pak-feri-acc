# ESP-NOW Remote Control System - Wiring Guide

## Overview
Sistem remote control wireless menggunakan protokol ESP-NOW untuk komunikasi antara dua ESP32-S3. Tidak memerlukan WiFi router karena komunikasi langsung peer-to-peer.

---

## 1️⃣ REMOTE TRANSMITTER (dengan 2 Joystick)

### Komponen yang Dibutuhkan:
- 1x ESP32-S3 DevKit
- 2x Analog Joystick Module (KY-023 atau sejenisnya)
- Kabel jumper
- Power bank / baterai 5V (opsional untuk portable)

### Wiring Diagram:

```
                    ┌─────────────────────┐
                    │      ESP32-S3       │
                    │    (Transmitter)    │
                    │                     │
   JOYSTICK 1       │                     │       JOYSTICK 2
   (Kiri)           │                     │       (Kanan)
                    │                     │
   VCC ─────────────┤ 3.3V          3.3V ├───────────── VCC
   GND ─────────────┤ GND            GND ├───────────── GND
   VRx ─────────────┤ GPIO1        GPIO4 ├───────────── VRx
   VRy ─────────────┤ GPIO2        GPIO5 ├───────────── VRy
   SW  ─────────────┤ GPIO42       GPIO6 ├───────────── SW
                    │                     │
                    │              GPIO48 ├─── LED Status (built-in)
                    │                     │
                    │         USB ────────┼─── Power / Program
                    └─────────────────────┘
```

### Tabel Pin Joystick 1 (Kiri - Throttle/Steering):
| Joystick Pin | ESP32-S3 Pin | Fungsi |
|--------------|--------------|--------|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| VRx | GPIO1 | Steering (Kiri/Kanan) |
| VRy | GPIO2 | Throttle (Maju/Mundur) |
| SW | GPIO42 | Tombol Tekan |

### Tabel Pin Joystick 2 (Kanan - Auxiliary):
| Joystick Pin | ESP32-S3 Pin | Fungsi |
|--------------|--------------|--------|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| VRx | GPIO4 | Auxiliary X |
| VRy | GPIO5 | Auxiliary Y (untuk servo dll) |
| SW | GPIO6 | Tombol Mode |

---

## 2️⃣ RC RECEIVER (Robot 4WD)

### Komponen yang Dibutuhkan:
- 1x ESP32-S3 DevKit
- 2x L298N Motor Driver
- 4x Motor DC (dengan roda)
- Baterai 12V (3S LiPo atau 3x 18650)
- Kabel jumper

### Wiring Diagram:

```
                                   ┌───────────────────────────────────────┐
                                   │            ESP32-S3                   │
                                   │           (Receiver)                  │
                                   │                                       │
    ┌─────────────┐                │                                       │                ┌─────────────┐
    │  L298N      │                │                                       │                │  L298N      │
    │  DEPAN      │                │                                       │                │  BELAKANG   │
    │             │                │                                       │                │             │
    │ ENA ────────┼────────────────┤ GPIO4                          GPIO18 ├────────────────┼── ENA       │
    │ IN1 ────────┼────────────────┤ GPIO14                         GPIO19 ├────────────────┼── IN1       │
    │ IN2 ────────┼────────────────┤ GPIO21                         GPIO20 ├────────────────┼── IN2       │
    │ ENB ────────┼────────────────┤ GPIO5                           GPIO7 ├────────────────┼── ENB       │
    │ IN3 ────────┼────────────────┤ GPIO16                         GPIO15 ├────────────────┼── IN3       │
    │ IN4 ────────┼────────────────┤ GPIO17                          GPIO8 ├────────────────┼── IN4       │
    │             │                │                                       │                │             │
    │ 5V ─────────┼────────────────┤ 5V (VIN)                              │                │             │
    │ GND ────────┼────────────────┤ GND ──────────────────────────────────┼────────────────┼── GND       │
    │             │                │                                       │                │             │
    │ +12V ───────┼───┬────────────┼───────────BATERAI 12V─────────────────┼────────────────┼── +12V      │
    │             │   │            │                                       │                │             │
    └─────────────┘   │            │                              GPIO48 ──┼─── LED Status  └─────────────┘
                      │            │                                       │
                      │            │                                 USB ──┼─── Program Only
                      │            └───────────────────────────────────────┘
                      │
                      └── Shared 12V dari Baterai
```

### Tabel Pin L298N Depan:
| L298N Pin | ESP32-S3 Pin | Fungsi |
|-----------|--------------|--------|
| ENA | GPIO4 | PWM Motor Kiri Depan |
| IN1 | GPIO14 | Direction Motor Kiri |
| IN2 | GPIO21 | Direction Motor Kiri |
| ENB | GPIO5 | PWM Motor Kanan Depan |
| IN3 | GPIO16 | Direction Motor Kanan |
| IN4 | GPIO17 | Direction Motor Kanan |
| +12V | Baterai 12V | Power Motor |
| 5V | ESP32 5V/VIN | Power ESP32 |
| GND | GND (shared) | Ground |

### Tabel Pin L298N Belakang:
| L298N Pin | ESP32-S3 Pin | Fungsi |
|-----------|--------------|--------|
| ENA | GPIO18 | PWM Motor Kiri Belakang |
| IN1 | GPIO19 | Direction Motor Kiri |
| IN2 | GPIO20 | Direction Motor Kiri |
| ENB | GPIO7 | PWM Motor Kanan Belakang |
| IN3 | GPIO15 | Direction Motor Kanan |
| IN4 | GPIO8 | Direction Motor Kanan |
| +12V | Baterai 12V | Power Motor |
| GND | GND (shared) | Ground |

### Koneksi Motor:
```
    DEPAN ROBOT
    ┌─────────────────────┐
    │  [M1]        [M2]   │   M1 = Motor Kiri Depan  -> L298N Depan OUT1/OUT2
    │   │            │    │   M2 = Motor Kanan Depan -> L298N Depan OUT3/OUT4
    │   │            │    │
    │   │  ESP32-S3  │    │
    │   │  L298N x2  │    │
    │   │            │    │
    │  [M3]        [M4]   │   M3 = Motor Kiri Blkg  -> L298N Belakang OUT1/OUT2
    │                     │   M4 = Motor Kanan Blkg -> L298N Belakang OUT3/OUT4
    └─────────────────────┘
    BELAKANG ROBOT
```

---

## 3️⃣ LANGKAH SETUP

### Step 1: Upload Kode ke Receiver
1. Hubungkan ESP32-S3 **receiver** ke komputer via USB
2. Buka `rc.ino` di Arduino IDE
3. Upload kode
4. Buka Serial Monitor (115200 baud)
5. **CATAT MAC ADDRESS** yang ditampilkan!

### Step 2: Update MAC Address di Transmitter
1. Buka `remote_transmitter.ino`
2. Temukan baris:
   ```cpp
   uint8_t receiverMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
   ```
3. Ganti dengan MAC address receiver, contoh:
   ```cpp
   uint8_t receiverMAC[] = {0x34, 0x85, 0x18, 0xAB, 0xCD, 0xEF};
   ```

### Step 3: Upload Kode ke Transmitter
1. Hubungkan ESP32-S3 **transmitter** ke komputer
2. Upload `remote_transmitter.ino`
3. Buka Serial Monitor untuk monitoring

### Step 4: Testing
1. Power on kedua ESP32-S3
2. Gerakkan joystick kiri - robot harus bergerak
3. LED akan berkedip saat ada komunikasi

---

## 4️⃣ KONTROL

### Joystick Kiri (Throttle/Steering):
| Gerakan | Aksi Robot |
|---------|------------|
| Dorong ke atas | Maju |
| Tarik ke bawah | Mundur |
| Dorong ke kiri | Belok kiri |
| Dorong ke kanan | Belok kanan |
| Diagonal | Kombinasi maju/mundur + belok |
| Tekan tombol | Emergency stop (btn1) |

### Joystick Kanan (Auxiliary):
| Gerakan | Aksi |
|---------|------|
| Sumbu Y (atas/bawah) | Kontrol servo (jika ada) |
| Sumbu X (kiri/kanan) | Cadangan |
| Tekan tombol | Ganti mode (Manual/Semi-Auto/Auto) |

---

## 5️⃣ TROUBLESHOOTING

| Masalah | Solusi |
|---------|--------|
| LED tidak menyala | Cek power supply |
| Tidak ada koneksi | Pastikan MAC address benar |
| Motor tidak bergerak | Cek wiring L298N dan jumper enable |
| Respon lambat | Kurangi SEND_INTERVAL di transmitter |
| Joystick tidak responsif | Cek koneksi ADC dan adjust DEADZONE |
| Robot bergerak sendiri | Kalibrasi joystick (adjust ADC_CENTER) |

---

## 6️⃣ CATATAN PENTING

> ⚠️ **PERINGATAN POWER**
> - Jangan hubungkan baterai 12V langsung ke ESP32-S3!
> - Gunakan output 5V dari L298N untuk power ESP32
> - Pastikan GND terhubung bersama (common ground)

> 💡 **TIPS**
> - Gunakan kabel yang cukup tebal untuk motor (min 18 AWG)
> - Tambahkan kapasitor 100µF di power L298N untuk stabilitas
> - Pastikan baterai memiliki kapasitas cukup (min 2000mAh)

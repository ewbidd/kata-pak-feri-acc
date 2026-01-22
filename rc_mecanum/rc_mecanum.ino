/*
 * ===================================================
 * ESP-NOW RC RECEIVER - ESP32-S3
 * MECANUM WHEEL OMNIDIRECTIONAL ROBOT
 * ===================================================
 *
 * KONFIGURASI RODA MECANUM:
 *
 *   DEPAN
 *   +-------------------+
 *   | FL (\\)     FR (/) |   <-- Arah roller mecanum
 *   |                   |
 *   | BL (/)     BR (\\) |
 *   +-------------------+
 *   BELAKANG
 *
 * WIRING L298N DEPAN:
 *   ENA  -> GPIO4  (PWM Motor Front Left)
 *   IN1  -> GPIO14 (Direction FL)
 *   IN2  -> GPIO21 (Direction FL)
 *   ENB  -> GPIO5  (PWM Motor Front Right)
 *   IN3  -> GPIO16 (Direction FR)
 *   IN4  -> GPIO17 (Direction FR)
 *   +12V -> Baterai 12V
 *   GND  -> GND (shared dengan ESP32)
 *
 * WIRING L298N BELAKANG:
 *   ENA  -> GPIO18 (PWM Motor Back Left)
 *   IN1  -> GPIO19 (Direction BL)
 *   IN2  -> GPIO20 (Direction BL)
 *   ENB  -> GPIO7  (PWM Motor Back Right)
 *   IN3  -> GPIO15 (Direction BR)
 *   IN4  -> GPIO8  (Direction BR)
 *   +12V -> Baterai 12V
 *   GND  -> GND (shared dengan ESP32)
 *
 * KONTROL (1 JOYSTICK - Mode Diagonal = Rotasi):
 *   - Y murni (atas/bawah): Maju/Mundur
 *   - X murni (kiri/kanan): Strafe Kiri/Kanan
 *   - Diagonal: Rotasi (arah X menentukan rotasi)
 *   - Tombol: Emergency Stop
 *
 * GERAKAN MECANUM:
 *   - Maju: Semua roda maju
 *   - Mundur: Semua roda mundur
 *   - Strafe Kiri: FL mundur, FR maju, BL maju, BR mundur
 *   - Strafe Kanan: FL maju, FR mundur, BL mundur, BR maju
 *   - Rotasi Kiri: FL mundur, FR maju, BL mundur, BR maju
 *   - Rotasi Kanan: FL maju, FR mundur, BL maju, BR mundur
 *
 * ===================================================
 */

#include <WiFi.h>
#include <esp_now.h>

// ===== PILIH SERIAL PORT =====
#define USE_SERIAL0 true

#if USE_SERIAL0
#define DEBUG_SERIAL Serial0
#else
#define DEBUG_SERIAL Serial
#endif

// ===== MOTOR PINS =====
// Front Left Motor (FL)
#define FL_ENA 4
#define FL_IN1 14
#define FL_IN2 21

// Front Right Motor (FR)
#define FR_ENA 5
#define FR_IN1 16
#define FR_IN2 17

// Back Left Motor (BL)
#define BL_ENA 18
#define BL_IN1 19
#define BL_IN2 20

// Back Right Motor (BR)
#define BR_ENA 7
#define BR_IN1 15
#define BR_IN2 8

// ===== LED STATUS =====
#define LED_PIN 48

// ===== PWM CONFIG =====
#define PWM_FREQ 20000
#define PWM_RES 8

#define CH_FL 0 // Channel PWM untuk Front Left
#define CH_FR 1 // Channel PWM untuk Front Right
#define CH_BL 2 // Channel PWM untuk Back Left
#define CH_BR 3 // Channel PWM untuk Back Right

// ===== TIMEOUT =====
#define CONNECTION_TIMEOUT 500 // ms tanpa data = stop motor

// ===== DEADZONE & THRESHOLD =====
#define DEADZONE 25        // Nilai joystick di bawah ini dianggap 0
#define DIAGONAL_RATIO 0.6 // Rasio min untuk deteksi diagonal
#define MAX_SPEED 200      // Kecepatan maksimum motor (0-255)

// ===== STRUKTUR DATA YANG DITERIMA =====
// Hanya menggunakan 1 joystick (throttle = Y, steering = X)
typedef struct {
  int16_t throttle; // -255 to 255 (Y axis)
  int16_t steering; // -255 to 255 (X axis)
  int16_t aux_x;    // tidak dipakai (1 joystick)
  int16_t aux_y;    // tidak dipakai
  bool btn1;        // Tombol joystick (emergency stop)
  bool btn2;        // Tombol 2
  uint8_t mode;     // Mode operasi
} RemoteData;

RemoteData receivedData;
unsigned long lastReceiveTime = 0;
bool dataReceived = false;

// ===== VARIABEL MOTOR =====
int speedFL = 0; // Front Left
int speedFR = 0; // Front Right
int speedBL = 0; // Back Left
int speedBR = 0; // Back Right

// ===== CALLBACK SAAT DATA DITERIMA =====
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len) {
  if (len == sizeof(RemoteData)) {
    memcpy(&receivedData, data, sizeof(receivedData));
    lastReceiveTime = millis();
    dataReceived = true;

    // LED berkedip saat menerima data
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}

// ===== APPLY DEADZONE =====
int applyDeadzone(int value) {
  if (abs(value) < DEADZONE) {
    return 0;
  }
  return value;
}

// ===== SETUP =====
void setup() {
  DEBUG_SERIAL.begin(115200);
  delay(1000);

  DEBUG_SERIAL.println("=============================================");
  DEBUG_SERIAL.println("  ESP-NOW RC RECEIVER - MECANUM WHEEL");
  DEBUG_SERIAL.println("  Omnidirectional Robot Controller");
  DEBUG_SERIAL.println("=============================================");

  // Setup motor pins
  pinMode(FL_IN1, OUTPUT);
  pinMode(FL_IN2, OUTPUT);
  pinMode(FR_IN1, OUTPUT);
  pinMode(FR_IN2, OUTPUT);
  pinMode(BL_IN1, OUTPUT);
  pinMode(BL_IN2, OUTPUT);
  pinMode(BR_IN1, OUTPUT);
  pinMode(BR_IN2, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Setup PWM channels
  ledcSetup(CH_FL, PWM_FREQ, PWM_RES);
  ledcSetup(CH_FR, PWM_FREQ, PWM_RES);
  ledcSetup(CH_BL, PWM_FREQ, PWM_RES);
  ledcSetup(CH_BR, PWM_FREQ, PWM_RES);

  ledcAttachPin(FL_ENA, CH_FL);
  ledcAttachPin(FR_ENA, CH_FR);
  ledcAttachPin(BL_ENA, CH_BL);
  ledcAttachPin(BR_ENA, CH_BR);

  // Stop motor saat startup
  stopAll();

  // Inisialisasi WiFi dalam mode station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Tampilkan MAC Address
  DEBUG_SERIAL.println("=============================================");
  DEBUG_SERIAL.print("MAC ADDRESS RECEIVER: ");
  DEBUG_SERIAL.println(WiFi.macAddress());
  DEBUG_SERIAL.println("=============================================");
  DEBUG_SERIAL.println("SALIN MAC ADDRESS DI ATAS KE KODE TRANSMITTER!");
  DEBUG_SERIAL.println("=============================================");

  // Inisialisasi ESP-NOW
  if (esp_now_init() != ESP_OK) {
    DEBUG_SERIAL.println("ERROR: Gagal inisialisasi ESP-NOW!");
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(100);
    }
  }
  DEBUG_SERIAL.println("ESP-NOW berhasil diinisialisasi");

  // Daftarkan callback
  esp_now_register_recv_cb(OnDataRecv);

  // Inisialisasi data
  receivedData.throttle = 0;
  receivedData.steering = 0;
  receivedData.aux_x = 0;
  receivedData.aux_y = 0;
  receivedData.btn1 = false;
  receivedData.btn2 = false;
  receivedData.mode = 0;

  // LED indikator ready
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }

  DEBUG_SERIAL.println("");
  DEBUG_SERIAL.println("KONTROL MECANUM (1 JOYSTICK):");
  DEBUG_SERIAL.println("  Y murni    : Maju/Mundur");
  DEBUG_SERIAL.println("  X murni    : Strafe Kiri/Kanan");
  DEBUG_SERIAL.println("  Diagonal   : Rotasi");
  DEBUG_SERIAL.println("  Tombol     : Emergency Stop");
  DEBUG_SERIAL.println("");
  DEBUG_SERIAL.println("Receiver siap! Menunggu data dari remote...");
  DEBUG_SERIAL.println("---------------------------------------------");
}

// ===== PARSING INPUT 1 JOYSTICK =====
// Mendeteksi mode berdasarkan posisi joystick
void parseJoystickInput(int rawY, int rawX, int &throttle, int &strafe,
                        int &rotation) {
  // Apply deadzone
  int y = applyDeadzone(rawY);
  int x = applyDeadzone(rawX);

  // Reset output
  throttle = 0;
  strafe = 0;
  rotation = 0;

  // Jika dalam deadzone, tidak ada gerakan
  if (x == 0 && y == 0) {
    return;
  }

  int absX = abs(x);
  int absY = abs(y);

  // Cek apakah diagonal (kedua axis aktif dengan rasio tertentu)
  bool isDiagonal = false;
  if (absX > DEADZONE && absY > DEADZONE) {
    float ratio = (float)min(absX, absY) / (float)max(absX, absY);
    isDiagonal = (ratio >= DIAGONAL_RATIO);
  }

  if (isDiagonal) {
    // MODE DIAGONAL = ROTASI
    // Arah X menentukan arah rotasi, magnitude dari yang terbesar
    int magnitude = max(absX, absY);
    rotation = (x > 0) ? magnitude : -magnitude;
  } else if (absY > absX) {
    // MODE VERTICAL = MAJU/MUNDUR
    throttle = y;
  } else {
    // MODE HORIZONTAL = STRAFE
    strafe = x;
  }
}

// ===== MECANUM KINEMATIK =====
// Menghitung kecepatan setiap roda berdasarkan input
void calculateMecanum(int throttle, int strafe, int rotation) {
  /*
   * Formula Mecanum:
   * FL = throttle + strafe + rotation
   * FR = throttle - strafe - rotation
   * BL = throttle - strafe + rotation
   * BR = throttle + strafe - rotation
   */

  // Hitung kecepatan setiap motor
  speedFL = throttle + strafe + rotation;
  speedFR = throttle - strafe - rotation;
  speedBL = throttle - strafe + rotation;
  speedBR = throttle + strafe - rotation;

  // Normalisasi jika ada yang melebihi MAX_SPEED
  int maxSpeed =
      max(max(abs(speedFL), abs(speedFR)), max(abs(speedBL), abs(speedBR)));

  if (maxSpeed > MAX_SPEED) {
    float scale = (float)MAX_SPEED / maxSpeed;
    speedFL = (int)(speedFL * scale);
    speedFR = (int)(speedFR * scale);
    speedBL = (int)(speedBL * scale);
    speedBR = (int)(speedBR * scale);
  }
}

// ===== KONTROL MOTOR INDIVIDUAL =====
void setMotor(int in1Pin, int in2Pin, int pwmChannel, int speed) {
  if (speed > 0) {
    // Maju
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, LOW);
    ledcWrite(pwmChannel, speed);
  } else if (speed < 0) {
    // Mundur
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
    ledcWrite(pwmChannel, -speed);
  } else {
    // Stop
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, LOW);
    ledcWrite(pwmChannel, 0);
  }
}

// ===== SET SEMUA MOTOR =====
void setMotors() {
  setMotor(FL_IN1, FL_IN2, CH_FL, speedFL);
  setMotor(FR_IN1, FR_IN2, CH_FR, speedFR);
  setMotor(BL_IN1, BL_IN2, CH_BL, speedBL);
  setMotor(BR_IN1, BR_IN2, CH_BR, speedBR);
}

// ===== STOP SEMUA MOTOR =====
void stopAll() {
  ledcWrite(CH_FL, 0);
  ledcWrite(CH_FR, 0);
  ledcWrite(CH_BL, 0);
  ledcWrite(CH_BR, 0);

  digitalWrite(FL_IN1, LOW);
  digitalWrite(FL_IN2, LOW);
  digitalWrite(FR_IN1, LOW);
  digitalWrite(FR_IN2, LOW);
  digitalWrite(BL_IN1, LOW);
  digitalWrite(BL_IN2, LOW);
  digitalWrite(BR_IN1, LOW);
  digitalWrite(BR_IN2, LOW);

  speedFL = 0;
  speedFR = 0;
  speedBL = 0;
  speedBR = 0;
}

// ===== DEBUG GERAKAN =====
String currentMode = "STOP";

void printMovement(int throttle, int strafe, int rotation) {
  if (throttle == 0 && strafe == 0 && rotation == 0) {
    currentMode = "STOP";
  } else if (rotation != 0) {
    currentMode = (rotation > 0) ? "ROTASI-KANAN" : "ROTASI-KIRI";
  } else if (throttle != 0) {
    currentMode = (throttle > 0) ? "MAJU" : "MUNDUR";
  } else if (strafe != 0) {
    currentMode = (strafe > 0) ? "STRAFE-KANAN" : "STRAFE-KIRI";
  }

  DEBUG_SERIAL.print("[");
  DEBUG_SERIAL.print(currentMode);
  DEBUG_SERIAL.print("]");
}

// ===== LOOP =====
void loop() {
  // Cek timeout koneksi
  if (millis() - lastReceiveTime > CONNECTION_TIMEOUT) {
    if (dataReceived) {
      DEBUG_SERIAL.println("!!! KONEKSI TERPUTUS - Motor STOP !!!");
      dataReceived = false;
    }
    stopAll();
    digitalWrite(LED_PIN, LOW);
    delay(100);
    return;
  }

  // Emergency stop jika tombol ditekan
  if (receivedData.btn1) {
    stopAll();
    return;
  }

  // Parse input 1 joystick ke throttle/strafe/rotation
  int throttle, strafe, rotation;
  parseJoystickInput(receivedData.throttle, receivedData.steering, throttle,
                     strafe, rotation);

  // Hitung kecepatan motor
  calculateMecanum(throttle, strafe, rotation);

  // Set motor
  setMotors();

  // Debug output
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 200) {
    DEBUG_SERIAL.print("Y:");
    DEBUG_SERIAL.print(receivedData.throttle);
    DEBUG_SERIAL.print(" X:");
    DEBUG_SERIAL.print(receivedData.steering);
    DEBUG_SERIAL.print(" -> T:");
    DEBUG_SERIAL.print(throttle);
    DEBUG_SERIAL.print(" S:");
    DEBUG_SERIAL.print(strafe);
    DEBUG_SERIAL.print(" R:");
    DEBUG_SERIAL.print(rotation);
    DEBUG_SERIAL.print(" | FL:");
    DEBUG_SERIAL.print(speedFL);
    DEBUG_SERIAL.print(" FR:");
    DEBUG_SERIAL.print(speedFR);
    DEBUG_SERIAL.print(" BL:");
    DEBUG_SERIAL.print(speedBL);
    DEBUG_SERIAL.print(" BR:");
    DEBUG_SERIAL.print(speedBR);
    DEBUG_SERIAL.print(" ");
    printMovement(throttle, strafe, rotation);
    DEBUG_SERIAL.println();
    lastDebug = millis();
  }

  delay(10);
}

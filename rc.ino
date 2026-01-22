/*
 * ===================================================
 * ESP-NOW RC RECEIVER - ESP32-S3
 * 4WD Robot dengan L298N Dual Driver
 * ===================================================
 *
 * WIRING L298N DEPAN:
 *   ENA  -> GPIO4  (PWM Motor Kiri Depan)
 *   IN1  -> GPIO14 (Direction)
 *   IN2  -> GPIO21 (Direction)
 *   ENB  -> GPIO5  (PWM Motor Kanan Depan)
 *   IN3  -> GPIO16 (Direction)
 *   IN4  -> GPIO17 (Direction)
 *   +12V -> Baterai 12V
 *   GND  -> GND (shared dengan ESP32)
 *
 * WIRING L298N BELAKANG:
 *   ENA  -> GPIO18 (PWM Motor Kiri Belakang)
 *   IN1  -> GPIO19 (Direction)
 *   IN2  -> GPIO20 (Direction)
 *   ENB  -> GPIO7  (PWM Motor Kanan Belakang)
 *   IN3  -> GPIO15 (Direction)
 *   IN4  -> GPIO8  (Direction)
 *   +12V -> Baterai 12V
 *   GND  -> GND (shared dengan ESP32)
 *
 * POWER ESP32-S3:
 *   5V   -> Output 5V dari L298N
 *   GND  -> GND
 *
 * LED Status:
 *   GPIO48 -> LED (Built-in)
 *
 * ===================================================
 */

#include <WiFi.h>
#include <esp_now.h>

// ===== PILIH SERIAL PORT =====
// Jika board punya USB-UART chip (CH340/CP2102), set ke true
// Jika pakai native USB CDC, set ke false
#define USE_SERIAL0 true

#if USE_SERIAL0
#define DEBUG_SERIAL Serial0
#else
#define DEBUG_SERIAL Serial
#endif

// ===== L298N DEPAN =====
#define F_ENA 4
#define F_IN1 14
#define F_IN2 21
#define F_ENB 5
#define F_IN3 16
#define F_IN4 17

// ===== L298N BELAKANG =====
#define B_ENA 18
#define B_IN1 19
#define B_IN2 20
#define B_ENB 7
#define B_IN3 15
#define B_IN4 8

// ===== LED STATUS =====
#define LED_PIN 48

// ===== PWM CONFIG =====
#define PWM_FREQ 20000
#define PWM_RES 8

#define CH_F_ENA 0
#define CH_F_ENB 1
#define CH_B_ENA 2
#define CH_B_ENB 3

// ===== TIMEOUT =====
#define CONNECTION_TIMEOUT 500 // ms tanpa data = stop motor

// ===== STRUKTUR DATA YANG DITERIMA =====
typedef struct {
  int16_t throttle; // -255 to 255 (maju/mundur)
  int16_t steering; // -255 to 255 (kiri/kanan)
  int16_t aux_x;    // -255 to 255 (auxiliary X)
  int16_t aux_y;    // -255 to 255 (auxiliary Y)
  bool btn1;        // Tombol joystick 1
  bool btn2;        // Tombol joystick 2
  uint8_t mode;     // Mode operasi
} RemoteData;

RemoteData receivedData;
unsigned long lastReceiveTime = 0;
bool dataReceived = false;

// ===== VARIABEL MOTOR =====
int leftSpeed = 0;
int rightSpeed = 0;

// ===== CALLBACK SAAT DATA DITERIMA =====
// Signature untuk ESP32 Arduino Core 2.0.x
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len) {
  if (len == sizeof(RemoteData)) {
    memcpy(&receivedData, data, sizeof(receivedData));
    lastReceiveTime = millis();
    dataReceived = true;

    // LED berkedip saat menerima data
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}

// ===== SETUP =====
void setup() {
  DEBUG_SERIAL.begin(115200);
  delay(1000);

  DEBUG_SERIAL.println("=====================================");
  DEBUG_SERIAL.println("  ESP-NOW RC RECEIVER");
  DEBUG_SERIAL.println("  4WD Robot Controller");
  DEBUG_SERIAL.println("=====================================");

  // Setup motor pins
  pinMode(F_IN1, OUTPUT);
  pinMode(F_IN2, OUTPUT);
  pinMode(F_IN3, OUTPUT);
  pinMode(F_IN4, OUTPUT);
  pinMode(B_IN1, OUTPUT);
  pinMode(B_IN2, OUTPUT);
  pinMode(B_IN3, OUTPUT);
  pinMode(B_IN4, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Setup PWM channels
  ledcSetup(CH_F_ENA, PWM_FREQ, PWM_RES);
  ledcSetup(CH_F_ENB, PWM_FREQ, PWM_RES);
  ledcSetup(CH_B_ENA, PWM_FREQ, PWM_RES);
  ledcSetup(CH_B_ENB, PWM_FREQ, PWM_RES);

  ledcAttachPin(F_ENA, CH_F_ENA);
  ledcAttachPin(F_ENB, CH_F_ENB);
  ledcAttachPin(B_ENA, CH_B_ENA);
  ledcAttachPin(B_ENB, CH_B_ENB);

  // Stop motor saat startup
  stopAll();

  // Inisialisasi WiFi dalam mode station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Tampilkan MAC Address - PENTING untuk dimasukkan ke transmitter!
  DEBUG_SERIAL.println("=====================================");
  DEBUG_SERIAL.print("MAC ADDRESS RECEIVER: ");
  DEBUG_SERIAL.println(WiFi.macAddress());
  DEBUG_SERIAL.println("=====================================");
  DEBUG_SERIAL.println("SALIN MAC ADDRESS DI ATAS KE KODE TRANSMITTER!");
  DEBUG_SERIAL.println("=====================================");

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

  DEBUG_SERIAL.println("Receiver siap! Menunggu data dari remote...");
  DEBUG_SERIAL.println("-------------------------------------");
}

// ===== FUNGSI MIXING THROTTLE-STEERING =====
void mixThrottleSteering(int throttle, int steering) {
  // Arcade/tank mixing
  // throttle: -255 to 255 (maju/mundur)
  // steering: -255 to 255 (kiri/kanan)

  // Hitung kecepatan kiri dan kanan
  leftSpeed = throttle + steering;
  rightSpeed = throttle - steering;

  // Constrain ke range -255 to 255
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);
}

// ===== KONTROL MOTOR =====
void setMotors(int left, int right) {
  // Left motors (F_ENA dan B_ENA)
  if (left > 0) {
    // Maju
    digitalWrite(F_IN1, HIGH);
    digitalWrite(F_IN2, LOW);
    digitalWrite(B_IN1, HIGH);
    digitalWrite(B_IN2, LOW);
    ledcWrite(CH_F_ENA, left);
    ledcWrite(CH_B_ENA, left);
  } else if (left < 0) {
    // Mundur
    digitalWrite(F_IN1, LOW);
    digitalWrite(F_IN2, HIGH);
    digitalWrite(B_IN1, LOW);
    digitalWrite(B_IN2, HIGH);
    ledcWrite(CH_F_ENA, -left);
    ledcWrite(CH_B_ENA, -left);
  } else {
    // Stop
    digitalWrite(F_IN1, LOW);
    digitalWrite(F_IN2, LOW);
    digitalWrite(B_IN1, LOW);
    digitalWrite(B_IN2, LOW);
    ledcWrite(CH_F_ENA, 0);
    ledcWrite(CH_B_ENA, 0);
  }

  // Right motors (F_ENB dan B_ENB)
  if (right > 0) {
    // Maju
    digitalWrite(F_IN3, HIGH);
    digitalWrite(F_IN4, LOW);
    digitalWrite(B_IN3, HIGH);
    digitalWrite(B_IN4, LOW);
    ledcWrite(CH_F_ENB, right);
    ledcWrite(CH_B_ENB, right);
  } else if (right < 0) {
    // Mundur
    digitalWrite(F_IN3, LOW);
    digitalWrite(F_IN4, HIGH);
    digitalWrite(B_IN3, LOW);
    digitalWrite(B_IN4, HIGH);
    ledcWrite(CH_F_ENB, -right);
    ledcWrite(CH_B_ENB, -right);
  } else {
    // Stop
    digitalWrite(F_IN3, LOW);
    digitalWrite(F_IN4, LOW);
    digitalWrite(B_IN3, LOW);
    digitalWrite(B_IN4, LOW);
    ledcWrite(CH_F_ENB, 0);
    ledcWrite(CH_B_ENB, 0);
  }
}

void stopAll() {
  ledcWrite(CH_F_ENA, 0);
  ledcWrite(CH_F_ENB, 0);
  ledcWrite(CH_B_ENA, 0);
  ledcWrite(CH_B_ENB, 0);

  digitalWrite(F_IN1, LOW);
  digitalWrite(F_IN2, LOW);
  digitalWrite(F_IN3, LOW);
  digitalWrite(F_IN4, LOW);
  digitalWrite(B_IN1, LOW);
  digitalWrite(B_IN2, LOW);
  digitalWrite(B_IN3, LOW);
  digitalWrite(B_IN4, LOW);
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

  // Update motor dari data yang diterima
  mixThrottleSteering(receivedData.throttle, receivedData.steering);
  setMotors(leftSpeed, rightSpeed);

  // Debug output
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 200) {
    DEBUG_SERIAL.print("T:");
    DEBUG_SERIAL.print(receivedData.throttle);
    DEBUG_SERIAL.print(" S:");
    DEBUG_SERIAL.print(receivedData.steering);
    DEBUG_SERIAL.print(" L:");
    DEBUG_SERIAL.print(leftSpeed);
    DEBUG_SERIAL.print(" R:");
    DEBUG_SERIAL.print(rightSpeed);
    DEBUG_SERIAL.print(" Mode:");
    DEBUG_SERIAL.print(receivedData.mode);
    DEBUG_SERIAL.print(" Btn1:");
    DEBUG_SERIAL.print(receivedData.btn1);
    DEBUG_SERIAL.print(" Btn2:");
    DEBUG_SERIAL.println(receivedData.btn2);
    lastDebug = millis();
  }

  delay(10);
}

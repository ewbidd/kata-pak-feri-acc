/*
 * ===================================================
 * ESP-NOW REMOTE TRANSMITTER - ESP32-S3
 * 2 Joystick Remote Controller
 * ===================================================
 *
 * WIRING JOYSTICK 1 (Kiri - Throttle/Steering):
 *   VCC  -> 3.3V
 *   GND  -> GND
 *   VRx  -> GPIO1 (ADC)
 *   VRy  -> GPIO2 (ADC)
 *   SW   -> GPIO42 (Optional button)
 *
 * WIRING JOYSTICK 2 (Kanan - Auxiliary):
 *   VCC  -> 3.3V
 *   GND  -> GND
 *   VRx  -> GPIO4 (ADC)
 *   VRy  -> GPIO5 (ADC)
 *   SW   -> GPIO6 (Optional button)
 *
 * LED Status:
 *   GPIO48 -> LED (Built-in pada beberapa ESP32-S3)
 *
 * ===================================================
 */

#include <WiFi.h>
#include <esp_now.h>

// ===== PIN JOYSTICK 1 (KIRI) =====
#define JOY1_X_PIN 1   // Steering (kiri-kanan)
#define JOY1_Y_PIN 2   // Throttle (maju-mundur)
#define JOY1_SW_PIN 42 // Tombol joystick 1

// ===== PIN JOYSTICK 2 (KANAN) =====
#define JOY2_X_PIN 4  // Auxiliary X
#define JOY2_Y_PIN 5  // Auxiliary Y (bisa untuk servo)
#define JOY2_SW_PIN 6 // Tombol joystick 2

// ===== LED STATUS =====
#define LED_PIN 48 // LED built-in ESP32-S3

// ===== INVERT AXES (sesuai hasil testing) =====
#define INVERT_JOY1_X false // X: false (sudah benar)
#define INVERT_JOY1_Y true  // Y: true (invert supaya atas=positif)
#define INVERT_JOY2_X true  // X: true
#define INVERT_JOY2_Y true  // Y: true

// ===== KONFIGURASI =====
#define SEND_INTERVAL 50 // Kirim data setiap 50ms (20Hz)
#define DEADZONE 30 // Zona mati joystick (diperbesar untuk menghindari drift)
#define DEBUG_SERIAL true // Debug via Serial

// ===== MAC ADDRESS RECEIVER =====
// GANTI DENGAN MAC ADDRESS ESP32-S3 RECEIVER ANDA!
// Untuk mendapatkan MAC, upload kode receiver dan lihat Serial Monitor
// FC:01:2C:D1:76:54
uint8_t receiverMAC[] = {0xFC, 0x01, 0x2C, 0xD1, 0x76, 0x54};

// ===== STRUKTUR DATA YANG DIKIRIM =====
typedef struct {
  int16_t throttle; // -255 to 255 (maju/mundur)
  int16_t steering; // -255 to 255 (kiri/kanan)
  int16_t aux_x;    // -255 to 255 (auxiliary X)
  int16_t aux_y;    // -255 to 255 (auxiliary Y)
  bool btn1;        // Tombol joystick 1
  bool btn2;        // Tombol joystick 2
  uint8_t mode;     // Mode operasi (0=manual, 1=semi-auto, 2=auto)
} RemoteData;

RemoteData dataToSend;

// ===== KALIBRASI (akan diisi saat startup) =====
int joy1_x_center = 2048;
int joy1_y_center = 2048;
int joy2_x_center = 2048;
int joy2_y_center = 2048;

// ===== VARIABEL ESP-NOW =====
esp_now_peer_info_t peerInfo;
bool peerConnected = false;
unsigned long lastSendTime = 0;
int sendFailCount = 0;

// ===== CALLBACK SAAT DATA TERKIRIM =====
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    digitalWrite(LED_PIN, HIGH);
    sendFailCount = 0;
  } else {
    digitalWrite(LED_PIN, LOW);
    sendFailCount++;
    if (DEBUG_SERIAL && sendFailCount > 10) {
      Serial.println("!!! Koneksi ke receiver terputus !!!");
    }
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=====================================");
  Serial.println("  ESP-NOW REMOTE TRANSMITTER");
  Serial.println("  2 Joystick Controller");
  Serial.println("=====================================");

  // Setup pin modes
  pinMode(JOY1_SW_PIN, INPUT_PULLUP);
  pinMode(JOY2_SW_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  // Setup ADC
  analogReadResolution(12);       // 12-bit ADC (0-4095)
  analogSetAttenuation(ADC_11db); // Full range 0-3.3V

  // === AUTO KALIBRASI JOYSTICK ===
  Serial.println();
  Serial.println(">>> KALIBRASI: Jangan sentuh joystick...");
  digitalWrite(LED_PIN, HIGH);
  delay(1000);

  // Baca nilai tengah (rata-rata 20 sample)
  long sum1x = 0, sum1y = 0, sum2x = 0, sum2y = 0;
  for (int i = 0; i < 20; i++) {
    sum1x += analogRead(JOY1_X_PIN);
    sum1y += analogRead(JOY1_Y_PIN);
    sum2x += analogRead(JOY2_X_PIN);
    sum2y += analogRead(JOY2_Y_PIN);
    delay(50);
  }

  // Simpan nilai tengah (dengan invert jika perlu)
  joy1_x_center = INVERT_JOY1_X ? (4095 - sum1x / 20) : (sum1x / 20);
  joy1_y_center = INVERT_JOY1_Y ? (4095 - sum1y / 20) : (sum1y / 20);
  joy2_x_center = INVERT_JOY2_X ? (4095 - sum2x / 20) : (sum2x / 20);
  joy2_y_center = INVERT_JOY2_Y ? (4095 - sum2y / 20) : (sum2y / 20);

  Serial.println(">>> KALIBRASI SELESAI!");
  Serial.print("  Joy1 Center: X=");
  Serial.print(joy1_x_center);
  Serial.print(", Y=");
  Serial.println(joy1_y_center);
  Serial.print("  Joy2 Center: X=");
  Serial.print(joy2_x_center);
  Serial.print(", Y=");
  Serial.println(joy2_y_center);
  Serial.println();

  // Inisialisasi WiFi dalam mode station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Tampilkan MAC Address
  Serial.print("MAC Address Transmitter: ");
  Serial.println(WiFi.macAddress());

  // Inisialisasi ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: Gagal inisialisasi ESP-NOW!");
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(100);
    }
  }
  Serial.println("ESP-NOW berhasil diinisialisasi");

  // Daftarkan callback
  esp_now_register_send_cb(OnDataSent);

  // Tambahkan peer (receiver)
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ERROR: Gagal menambahkan peer!");
  } else {
    Serial.println("Peer receiver berhasil ditambahkan");
    peerConnected = true;
  }

  // Inisialisasi data
  dataToSend.throttle = 0;
  dataToSend.steering = 0;
  dataToSend.aux_x = 0;
  dataToSend.aux_y = 0;
  dataToSend.btn1 = false;
  dataToSend.btn2 = false;
  dataToSend.mode = 0;

  // LED indikator ready
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }

  Serial.println("Remote siap!");
  Serial.println("-------------------------------------");
}

// ===== FUNGSI MAPPING DENGAN KALIBRASI =====
int16_t mapWithCenter(int value, int center) {
  // Dari center, map ke -255 sampai +255
  int result;
  if (value < center) {
    result = map(value, 0, center, -255, 0);
  } else {
    result = map(value, center, 4095, 0, 255);
  }

  // Aplikasikan deadzone
  if (abs(result) < DEADZONE) {
    return 0;
  }

  return constrain(result, -255, 255);
}

// ===== BACA SEMUA INPUT =====
void readInputs() {
  // Baca joystick analog (raw)
  int raw_joy1_x = analogRead(JOY1_X_PIN);
  int raw_joy1_y = analogRead(JOY1_Y_PIN);
  int raw_joy2_x = analogRead(JOY2_X_PIN);
  int raw_joy2_y = analogRead(JOY2_Y_PIN);

  // Debug nilai RAW ADC (uncomment untuk debug)
  // Serial.print("RAW: J1X="); Serial.print(raw_joy1_x);
  // Serial.print(" J1Y="); Serial.print(raw_joy1_y);
  // Serial.print(" J2X="); Serial.print(raw_joy2_x);
  // Serial.print(" J2Y="); Serial.println(raw_joy2_y);

  // Apply invert
  int joy1_x = INVERT_JOY1_X ? (4095 - raw_joy1_x) : raw_joy1_x;
  int joy1_y = INVERT_JOY1_Y ? (4095 - raw_joy1_y) : raw_joy1_y;
  int joy2_x = INVERT_JOY2_X ? (4095 - raw_joy2_x) : raw_joy2_x;
  int joy2_y = INVERT_JOY2_Y ? (4095 - raw_joy2_y) : raw_joy2_y;

  // Map dengan kalibrasi
  // Joystick 1: Y = throttle (maju/mundur), X = steering (kiri/kanan)
  dataToSend.throttle = mapWithCenter(joy1_y, joy1_y_center);
  dataToSend.steering = mapWithCenter(joy1_x, joy1_x_center);

  // Joystick 2: Auxiliary controls
  dataToSend.aux_x = mapWithCenter(joy2_x, joy2_x_center);
  dataToSend.aux_y = mapWithCenter(joy2_y, joy2_y_center);

  // Baca tombol (active LOW karena INPUT_PULLUP)
  dataToSend.btn1 = !digitalRead(JOY1_SW_PIN);
  dataToSend.btn2 = !digitalRead(JOY2_SW_PIN);

  // Mode toggle dengan tombol 2
  static bool lastBtn2 = false;
  if (dataToSend.btn2 && !lastBtn2) {
    dataToSend.mode = (dataToSend.mode + 1) % 3;
    Serial.print("Mode changed to: ");
    Serial.println(dataToSend.mode);
  }
  lastBtn2 = dataToSend.btn2;
}

// ===== KIRIM DATA =====
void sendData() {
  esp_err_t result =
      esp_now_send(receiverMAC, (uint8_t *)&dataToSend, sizeof(dataToSend));

  if (DEBUG_SERIAL) {
    Serial.print("T:");
    Serial.print(dataToSend.throttle);
    Serial.print(" S:");
    Serial.print(dataToSend.steering);
    Serial.print(" AX:");
    Serial.print(dataToSend.aux_x);
    Serial.print(" AY:");
    Serial.print(dataToSend.aux_y);
    Serial.print(" B1:");
    Serial.print(dataToSend.btn1);
    Serial.print(" B2:");
    Serial.print(dataToSend.btn2);
    Serial.print(" M:");
    Serial.print(dataToSend.mode);
    Serial.print(" -> ");
    Serial.println(result == ESP_OK ? "OK" : "FAIL");
  }
}

// ===== LOOP =====
void loop() {
  // Baca semua input
  readInputs();

  // Kirim data dengan interval
  if (millis() - lastSendTime >= SEND_INTERVAL) {
    sendData();
    lastSendTime = millis();
  }

  // Small delay untuk stabilitas ADC
  delay(5);
}

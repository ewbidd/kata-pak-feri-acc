#include <ESP32Servo.h>

/* ===== L298N DEPAN ===== */
#define F_ENA 4
#define F_IN1 14
#define F_IN2 21
#define F_ENB 5
#define F_IN3 16
#define F_IN4 17

/* ===== L298N BELAKANG ===== */
#define B_ENA 18
#define B_IN1 19
#define B_IN2 20
#define B_ENB 7
#define B_IN3 15
#define B_IN4 8

/* ===== SENSOR ===== */
#define TRIG_PIN 2
#define ECHO_PIN 9 // Diubah dari GPIO3 (strapping pin) ke GPIO9
#define SERVO_PIN 6

/* ===== BATTERY MONITOR ===== */
#define BATTERY_PIN 10    // GPIO10 (ADC capable)
#define R1 100000.0       // 100K ohm
#define R2 10000.0        // 10K ohm
#define BATT_FULL 12.6    // 3S penuh
#define BATT_EMPTY 9.0    // 3S kosong
#define BATT_WARNING 10.0 // Peringatan baterai rendah

/* ===== PWM CONFIG ===== */
#define PWM_FREQ 20000
#define PWM_RES 8

#define CH_F_ENA 0
#define CH_F_ENB 1
#define CH_B_ENA 2
#define CH_B_ENB 3

/* ===== SPEED SETTINGS ===== */
#define SPEED_FWD 180     // Kecepatan maju
#define SPEED_TURN 160    // Kecepatan belok
#define SPEED_REVERSE 200 // Kecepatan mundur (lebih kuat!)
#define SPEED_SLOW 120    // Kecepatan pelan saat mendekati obstacle

/* ===== DISTANCE THRESHOLDS ===== */
#define STOP_DIST 30     // Jarak berhenti (cm)
#define SLOW_DIST 50     // Jarak mulai pelan (cm)
#define CRITICAL_DIST 15 // Jarak kritis - harus mundur (cm)
#define MIN_SIDE_DIST 25 // Jarak minimum samping untuk belok

/* ===== TIMING ===== */
#define SCAN_DELAY 250   // Delay scan servo (ms)
#define REVERSE_TIME 400 // Waktu mundur (ms)
#define TURN_TIME 500    // Waktu belok (ms)
#define STUCK_COUNT 5    // Jumlah loop stuck sebelum recovery

Servo servo;

int stuckCounter = 0;
int lastAction = 0; // 0=forward, 1=left, 2=right, 3=reverse

// Battery monitoring
float batteryVoltage = 12.6;
int batteryPercent = 100;
unsigned long lastBatteryCheck = 0;
#define BATTERY_CHECK_INTERVAL 5000 // Cek baterai setiap 5 detik

/* ================= SETUP ================= */
void setup() {
  delay(1000); // Tunggu USB CDC siap
  Serial.begin(115200);

  while (!Serial && millis() < 3000) {
    delay(10); // Tunggu Serial max 3 detik
  }

  Serial.println("=====================================");
  Serial.println("  OBSTACLE AVOIDING ROBOT - ESP32S3");
  Serial.println("=====================================");

  // Setup motor pins
  pinMode(F_IN1, OUTPUT);
  pinMode(F_IN2, OUTPUT);
  pinMode(F_IN3, OUTPUT);
  pinMode(F_IN4, OUTPUT);
  pinMode(B_IN1, OUTPUT);
  pinMode(B_IN2, OUTPUT);
  pinMode(B_IN3, OUTPUT);
  pinMode(B_IN4, OUTPUT);

  // Setup PWM channels
  ledcSetup(CH_F_ENA, PWM_FREQ, PWM_RES);
  ledcSetup(CH_F_ENB, PWM_FREQ, PWM_RES);
  ledcSetup(CH_B_ENA, PWM_FREQ, PWM_RES);
  ledcSetup(CH_B_ENB, PWM_FREQ, PWM_RES);

  ledcAttachPin(F_ENA, CH_F_ENA);
  ledcAttachPin(F_ENB, CH_F_ENB);
  ledcAttachPin(B_ENA, CH_B_ENA);
  ledcAttachPin(B_ENB, CH_B_ENB);

  // Setup sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Setup battery monitor ADC
  analogReadResolution(12);       // 12-bit ADC (0-4095)
  analogSetAttenuation(ADC_11db); // Full range 0-3.3V

  // Setup servo
  servo.setPeriodHertz(50);
  servo.attach(SERVO_PIN, 500, 2500);
  servo.write(90);
  delay(500);

  stopAll();

  // Baca baterai awal
  updateBattery();
  Serial.print("Baterai: ");
  Serial.print(batteryVoltage, 1);
  Serial.print("V (");
  Serial.print(batteryPercent);
  Serial.println("%)");

  Serial.println("Robot siap!");
}

/* ================= LOOP ================= */
void loop() {
  // Cek baterai secara berkala
  if (millis() - lastBatteryCheck > BATTERY_CHECK_INTERVAL) {
    updateBattery();
    lastBatteryCheck = millis();

    // Peringatan baterai rendah
    if (batteryVoltage < BATT_WARNING) {
      Serial.println("!!! PERINGATAN: BATERAI RENDAH !!!");
      Serial.print("Baterai: ");
      Serial.print(batteryVoltage, 1);
      Serial.print("V (");
      Serial.print(batteryPercent);
      Serial.println("%)");
    }
  }

  int frontDist = getDistance();
  Serial.print("Jarak: ");
  Serial.print(frontDist);
  Serial.print("cm | Batt: ");
  Serial.print(batteryVoltage, 1);
  Serial.print("V (");
  Serial.print(batteryPercent);
  Serial.println("%)");

  // === CRITICAL: Terlalu dekat, harus mundur! ===
  if (frontDist < CRITICAL_DIST) {
    Serial.println(">>> CRITICAL! Mundur...");
    stopAll();
    delay(100);
    reverse(REVERSE_TIME);
    stuckCounter++;

    // Jika sudah beberapa kali mundur, coba recovery
    if (stuckCounter >= STUCK_COUNT) {
      Serial.println(">>> STUCK DETECTED! Recovery mode...");
      recoveryMode();
      stuckCounter = 0;
    }
    return;
  }

  // === OBSTACLE DETECTED ===
  if (frontDist < STOP_DIST) {
    Serial.println(">>> Obstacle terdeteksi! Scanning...");
    stopAll();
    delay(150);

    // Mundur sedikit dulu untuk jaga jarak
    reverse(200);

    // Scan ke semua arah
    int scanResult[5];
    scanAllDirections(scanResult);

    // Pilih arah terbaik
    int bestDirection = chooseBestDirection(scanResult);
    executeManeuver(bestDirection);

    stuckCounter = 0; // Reset stuck counter setelah berhasil manuver
  }
  // === SLOW DOWN ZONE ===
  else if (frontDist < SLOW_DIST) {
    Serial.println(">>> Mendekati obstacle, pelan...");
    forward(SPEED_SLOW);
    stuckCounter = 0;
  }
  // === CLEAR PATH ===
  else {
    forward(SPEED_FWD);
    stuckCounter = 0;
  }

  delay(50); // Loop delay
}

/* ================= SCANNING ================= */
void scanAllDirections(int *results) {
  // results[0] = kiri jauh (150°)
  // results[1] = kiri dekat (120°)
  // results[2] = depan (90°)
  // results[3] = kanan dekat (60°)
  // results[4] = kanan jauh (30°)

  int angles[] = {150, 120, 90, 60, 30};

  for (int i = 0; i < 5; i++) {
    servo.write(angles[i]);
    delay(SCAN_DELAY);
    results[i] = getDistance();
    Serial.print("Angle ");
    Serial.print(angles[i]);
    Serial.print("°: ");
    Serial.print(results[i]);
    Serial.println(" cm");
  }

  // Kembali ke tengah
  servo.write(90);
  delay(100);
}

int chooseBestDirection(int *scan) {
  // scan[0]=kiri jauh, scan[1]=kiri dekat, scan[2]=depan
  // scan[3]=kanan dekat, scan[4]=kanan jauh

  int leftAvg = (scan[0] + scan[1]) / 2;
  int rightAvg = (scan[3] + scan[4]) / 2;
  int frontDist = scan[2];

  Serial.print("Left avg: ");
  Serial.print(leftAvg);
  Serial.print(" | Right avg: ");
  Serial.print(rightAvg);
  Serial.print(" | Front: ");
  Serial.println(frontDist);

  // Jika depan clear, maju
  if (frontDist > SLOW_DIST) {
    Serial.println("Decision: MAJU");
    return 0; // Forward
  }

  // Jika kiri dan kanan sama-sama blocked, mundur
  if (leftAvg < MIN_SIDE_DIST && rightAvg < MIN_SIDE_DIST) {
    Serial.println("Decision: MUNDUR (terjepit)");
    return 3; // Reverse then turn
  }

  // Pilih arah yang lebih luas
  if (leftAvg > rightAvg && leftAvg > MIN_SIDE_DIST) {
    // Jika selisih kecil dan sebelumnya belok kanan, tetap kanan (hindari
    // zigzag)
    if (abs(leftAvg - rightAvg) < 15 && lastAction == 2) {
      Serial.println("Decision: KANAN (anti-zigzag)");
      return 2;
    }
    Serial.println("Decision: KIRI");
    return 1; // Left
  } else if (rightAvg > MIN_SIDE_DIST) {
    // Sama seperti di atas
    if (abs(leftAvg - rightAvg) < 15 && lastAction == 1) {
      Serial.println("Decision: KIRI (anti-zigzag)");
      return 1;
    }
    Serial.println("Decision: KANAN");
    return 2; // Right
  }

  // Default: mundur dan putar
  Serial.println("Decision: MUNDUR + PUTAR");
  return 3;
}

void executeManeuver(int direction) {
  switch (direction) {
  case 0: // Forward
    forward(SPEED_FWD);
    delay(200);
    lastAction = 0;
    break;

  case 1: // Left
    turnLeft(TURN_TIME);
    lastAction = 1;
    break;

  case 2: // Right
    turnRight(TURN_TIME);
    lastAction = 2;
    break;

  case 3: // Reverse and turn
    reverse(REVERSE_TIME);
    delay(100);
    // Putar ke arah yang terakhir berhasil, atau random
    if (lastAction == 1) {
      turnLeft(TURN_TIME + 200); // Belok lebih lama
    } else {
      turnRight(TURN_TIME + 200);
    }
    lastAction = 3;
    break;
  }
}

void recoveryMode() {
  Serial.println("=== RECOVERY MODE ===");

  // Mundur lebih lama
  reverse(800);
  delay(200);

  // Putar 180 derajat (atau hampir)
  Serial.println("Putar balik...");
  turnRight(1200); // Putar lama

  // Cek depan
  int frontDist = getDistance();
  if (frontDist < STOP_DIST) {
    // Masih blocked, coba arah lain
    turnLeft(1500);
  }

  Serial.println("=== RECOVERY COMPLETE ===");
}

/* ================= MOTOR FUNCTIONS ================= */
void forward(int spd) {
  digitalWrite(F_IN1, HIGH);
  digitalWrite(F_IN2, LOW);
  digitalWrite(F_IN3, HIGH);
  digitalWrite(F_IN4, LOW);
  digitalWrite(B_IN1, HIGH);
  digitalWrite(B_IN2, LOW);
  digitalWrite(B_IN3, HIGH);
  digitalWrite(B_IN4, LOW);

  ledcWrite(CH_F_ENA, spd);
  ledcWrite(CH_F_ENB, spd);
  ledcWrite(CH_B_ENA, spd);
  ledcWrite(CH_B_ENB, spd);
}

void reverse(int duration) {
  // Mundur dengan kecepatan penuh!
  digitalWrite(F_IN1, LOW);
  digitalWrite(F_IN2, HIGH);
  digitalWrite(F_IN3, LOW);
  digitalWrite(F_IN4, HIGH);
  digitalWrite(B_IN1, LOW);
  digitalWrite(B_IN2, HIGH);
  digitalWrite(B_IN3, LOW);
  digitalWrite(B_IN4, HIGH);

  ledcWrite(CH_F_ENA, SPEED_REVERSE);
  ledcWrite(CH_F_ENB, SPEED_REVERSE);
  ledcWrite(CH_B_ENA, SPEED_REVERSE);
  ledcWrite(CH_B_ENB, SPEED_REVERSE);

  delay(duration);
  stopAll();
}

void turnLeft(int duration) {
  // Pivot turn: kiri mundur, kanan maju
  digitalWrite(F_IN1, LOW);
  digitalWrite(F_IN2, HIGH); // Kiri mundur
  digitalWrite(F_IN3, HIGH);
  digitalWrite(F_IN4, LOW); // Kanan maju
  digitalWrite(B_IN1, LOW);
  digitalWrite(B_IN2, HIGH); // Kiri mundur
  digitalWrite(B_IN3, HIGH);
  digitalWrite(B_IN4, LOW); // Kanan maju

  ledcWrite(CH_F_ENA, SPEED_TURN);
  ledcWrite(CH_F_ENB, SPEED_TURN);
  ledcWrite(CH_B_ENA, SPEED_TURN);
  ledcWrite(CH_B_ENB, SPEED_TURN);

  delay(duration);
  stopAll();
}

void turnRight(int duration) {
  // Pivot turn: kiri maju, kanan mundur
  digitalWrite(F_IN1, HIGH);
  digitalWrite(F_IN2, LOW); // Kiri maju
  digitalWrite(F_IN3, LOW);
  digitalWrite(F_IN4, HIGH); // Kanan mundur
  digitalWrite(B_IN1, HIGH);
  digitalWrite(B_IN2, LOW); // Kiri maju
  digitalWrite(B_IN3, LOW);
  digitalWrite(B_IN4, HIGH); // Kanan mundur

  ledcWrite(CH_F_ENA, SPEED_TURN);
  ledcWrite(CH_F_ENB, SPEED_TURN);
  ledcWrite(CH_B_ENA, SPEED_TURN);
  ledcWrite(CH_B_ENB, SPEED_TURN);

  delay(duration);
  stopAll();
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

/* ================= SENSOR ================= */
int getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return 999; // Tidak ada echo = jarak sangat jauh
  }

  int distance = duration * 0.034 / 2;

  // Filter nilai yang tidak masuk akal
  if (distance < 2 || distance > 400) {
    return 999;
  }

  return distance;
}

/* ================= BATTERY ================= */
float readBatteryVoltage() {
  // Baca ADC beberapa kali untuk rata-rata (filter noise)
  long total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(BATTERY_PIN);
    delay(2);
  }
  int adcValue = total / 10;

  // Konversi ke tegangan (ESP32-S3 ADC 12-bit, 3.3V referensi)
  float voltage = (adcValue / 4095.0) * 3.3;

  // Hitung tegangan baterai asli (voltage divider)
  float battVoltage = voltage * ((R1 + R2) / R2);

  return battVoltage;
}

int getBatteryPercentage(float voltage) {
  if (voltage >= BATT_FULL)
    return 100;
  if (voltage <= BATT_EMPTY)
    return 0;

  return (int)((voltage - BATT_EMPTY) / (BATT_FULL - BATT_EMPTY) * 100);
}

void updateBattery() {
  batteryVoltage = readBatteryVoltage();
  batteryPercent = getBatteryPercentage(batteryVoltage);
}

/**
 * ============================================================
 * ESP32-S3 MASTER - WIRING TEST
 * ============================================================
 * Test semua pin wiring untuk Master device
 *
 * Hardware yang ditest:
 * - 4x Motor (FL, FR, BL, BR) via L298N
 * - OLED SSD1306 128x64 (I2C)
 * - Buzzer
 * - 3x Button (UP, DOWN, OK)
 * - Status LED
 *
 * Cara pakai:
 * 1. Upload sketch ini
 * 2. Buka Serial Monitor (115200 baud)
 * 3. Ketik perintah: m (motor), b (buzzer), o (oled), t (button), l (led), a
 * (all)
 * ============================================================
 */

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>


// ============================================================
// PIN DEFINITIONS - SESUAIKAN DENGAN WIRING ANDA
// ============================================================

// Motor Driver - Front Left (FL)
#define PIN_FL_ENA 4
#define PIN_FL_IN1 14
#define PIN_FL_IN2 21

// Motor Driver - Front Right (FR)
#define PIN_FR_ENA 5
#define PIN_FR_IN1 16
#define PIN_FR_IN2 17

// Motor Driver - Back Left (BL)
#define PIN_BL_ENA 18
#define PIN_BL_IN1 19
#define PIN_BL_IN2 20

// Motor Driver - Back Right (BR)
#define PIN_BR_ENA 7
#define PIN_BR_IN1 15
#define PIN_BR_IN2 8

// OLED Display (I2C)
#define PIN_OLED_SDA 9  // SDK
#define PIN_OLED_SCK 10 // SCK
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDRESS 0x3C

// Buzzer
#define PIN_BUZZER 11

// Navigation Buttons (3 buttons)
#define PIN_BTN_UP 12
#define PIN_BTN_DOWN 13
#define PIN_BTN_OK 38

// Status LED
#define PIN_LED_STATUS 48

// PWM Configuration
#define PWM_FREQUENCY 20000
#define PWM_RESOLUTION 8

// Double-click timing
#define DOUBLE_CLICK_MS 400

// ============================================================
// GLOBALS
// ============================================================
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// Button state
unsigned long lastOkClickTime = 0;
bool waitingForDoubleClick = false;

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println(
      "============================================================");
  Serial.println("       ESP32-S3 MASTER - WIRING TEST");
  Serial.println(
      "============================================================");
  Serial.println();

  // Initialize all pins
  initMotorPins();
  initBuzzer();
  initButtons();
  initLED();
  initOLED();

  printMenu();
}

// ============================================================
// INIT FUNCTIONS
// ============================================================
void initMotorPins() {
  Serial.println("[INIT] Motor pins...");

  // FL Motor
  pinMode(PIN_FL_IN1, OUTPUT);
  pinMode(PIN_FL_IN2, OUTPUT);
  ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(PIN_FL_ENA, 0);

  // FR Motor
  pinMode(PIN_FR_IN1, OUTPUT);
  pinMode(PIN_FR_IN2, OUTPUT);
  ledcSetup(1, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(PIN_FR_ENA, 1);

  // BL Motor
  pinMode(PIN_BL_IN1, OUTPUT);
  pinMode(PIN_BL_IN2, OUTPUT);
  ledcSetup(2, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(PIN_BL_ENA, 2);

  // BR Motor
  pinMode(PIN_BR_IN1, OUTPUT);
  pinMode(PIN_BR_IN2, OUTPUT);
  ledcSetup(3, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(PIN_BR_ENA, 3);

  stopAllMotors();
  Serial.println("[INIT] Motor pins OK");
}

void initBuzzer() {
  Serial.println("[INIT] Buzzer...");
  ledcSetup(4, 2000, 8);
  ledcAttachPin(PIN_BUZZER, 4);
  ledcWriteTone(4, 0);
  Serial.println("[INIT] Buzzer OK");
}

void initButtons() {
  Serial.println("[INIT] Buttons...");
  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BTN_OK, INPUT_PULLUP);
  Serial.printf("[INIT] UP=GPIO%d, DOWN=GPIO%d, OK=GPIO%d\n", PIN_BTN_UP,
                PIN_BTN_DOWN, PIN_BTN_OK);
}

void initLED() {
  Serial.println("[INIT] Status LED...");
  pinMode(PIN_LED_STATUS, OUTPUT);
  digitalWrite(PIN_LED_STATUS, LOW);
  Serial.println("[INIT] LED OK");
}

void initOLED() {
  Serial.println("[INIT] OLED Display...");
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCK);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("[INIT] OLED FAILED! Check wiring.");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("WIRING TEST");
    display.println("===========");
    display.println();
    display.println("OLED: OK!");
    display.display();
    Serial.println("[INIT] OLED OK");
  }
}

void printMenu() {
  Serial.println();
  Serial.println(
      "============================================================");
  Serial.println("COMMANDS (ketik di Serial Monitor):");
  Serial.println("  m - Test semua motor");
  Serial.println("  1 - Test motor FL saja");
  Serial.println("  2 - Test motor FR saja");
  Serial.println("  3 - Test motor BL saja");
  Serial.println("  4 - Test motor BR saja");
  Serial.println("  b - Test buzzer");
  Serial.println("  o - Test OLED display");
  Serial.println("  t - Test buttons (tekan button untuk test)");
  Serial.println("  l - Test LED status");
  Serial.println("  a - Test ALL");
  Serial.println("  s - Stop all motors");
  Serial.println(
      "============================================================");
  Serial.println();
}

// ============================================================
// MOTOR FUNCTIONS
// ============================================================
void setMotor(int channel, int in1, int in2, int speed, const char *name) {
  Serial.printf("[MOTOR] %s: speed=%d\n", name, speed);

  if (speed > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    ledcWrite(channel, speed);
  } else if (speed < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    ledcWrite(channel, -speed);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    ledcWrite(channel, 0);
  }
}

void stopAllMotors() {
  setMotor(0, PIN_FL_IN1, PIN_FL_IN2, 0, "FL");
  setMotor(1, PIN_FR_IN1, PIN_FR_IN2, 0, "FR");
  setMotor(2, PIN_BL_IN1, PIN_BL_IN2, 0, "BL");
  setMotor(3, PIN_BR_IN1, PIN_BR_IN2, 0, "BR");
}

void testSingleMotor(int channel, int in1, int in2, const char *name) {
  Serial.printf("\n[TEST] Motor %s\n", name);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("Motor: %s", name);
  display.setCursor(0, 20);
  display.println("FWD...");
  display.display();

  // Forward
  Serial.printf("  %s Forward...\n", name);
  setMotor(channel, in1, in2, 150, name);
  delay(1000);

  // Stop
  setMotor(channel, in1, in2, 0, name);
  delay(300);

  display.setCursor(0, 30);
  display.println("REV...");
  display.display();

  // Reverse
  Serial.printf("  %s Reverse...\n", name);
  setMotor(channel, in1, in2, -150, name);
  delay(1000);

  // Stop
  setMotor(channel, in1, in2, 0, name);
  Serial.printf("  %s DONE\n", name);

  display.setCursor(0, 40);
  display.println("DONE!");
  display.display();
}

void testAllMotors() {
  Serial.println("\n========== TEST ALL MOTORS ==========");

  testSingleMotor(0, PIN_FL_IN1, PIN_FL_IN2, "FL");
  delay(500);
  testSingleMotor(1, PIN_FR_IN1, PIN_FR_IN2, "FR");
  delay(500);
  testSingleMotor(2, PIN_BL_IN1, PIN_BL_IN2, "BL");
  delay(500);
  testSingleMotor(3, PIN_BR_IN1, PIN_BR_IN2, "BR");

  Serial.println("========== MOTOR TEST COMPLETE ==========\n");
}

// ============================================================
// BUZZER FUNCTIONS
// ============================================================
void testBuzzer() {
  Serial.println("\n========== TEST BUZZER ==========");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("BUZZER TEST");
  display.display();

  // Startup sound
  Serial.println("  Playing startup...");
  ledcWriteTone(4, 1000);
  delay(100);
  ledcWriteTone(4, 1500);
  delay(100);
  ledcWriteTone(4, 2000);
  delay(100);
  ledcWriteTone(4, 0);
  delay(200);

  // Beep beep
  Serial.println("  Playing beeps...");
  for (int i = 0; i < 3; i++) {
    ledcWriteTone(4, 1500);
    delay(100);
    ledcWriteTone(4, 0);
    delay(100);
  }

  display.setCursor(0, 20);
  display.println("DONE!");
  display.display();

  Serial.println("========== BUZZER TEST COMPLETE ==========\n");
}

// ============================================================
// OLED FUNCTIONS
// ============================================================
void testOLED() {
  Serial.println("\n========== TEST OLED ==========");

  // Test 1: Text
  Serial.println("  Drawing text...");
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("MINI OS");
  display.setTextSize(1);
  display.println();
  display.println("OLED TEST OK!");
  display.println();
  display.println("128x64 SSD1306");
  display.display();
  delay(1500);

  // Test 2: Shapes
  Serial.println("  Drawing shapes...");
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.drawCircle(64, 32, 20, SSD1306_WHITE);
  display.drawLine(0, 0, 127, 63, SSD1306_WHITE);
  display.drawLine(127, 0, 0, 63, SSD1306_WHITE);
  display.display();
  delay(1500);

  // Test 3: Fill
  Serial.println("  Fill test...");
  display.clearDisplay();
  display.fillRect(10, 10, 50, 44, SSD1306_WHITE);
  display.fillCircle(100, 32, 15, SSD1306_WHITE);
  display.display();
  delay(1500);

  // Done
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("OLED TEST DONE!");
  display.display();

  Serial.println("========== OLED TEST COMPLETE ==========\n");
}

// ============================================================
// BUTTON FUNCTIONS
// ============================================================
void testButtons() {
  Serial.println("\n========== TEST BUTTONS ==========");
  Serial.println("Tekan button untuk test (5 detik)...");
  Serial.println("OK double-click = BACK function");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("BUTTON TEST");
  display.println("Press buttons!");
  display.println();
  display.println("UP:   GPIO 12");
  display.println("DOWN: GPIO 13");
  display.println("OK:   GPIO 38");
  display.println("(2x click = BACK)");
  display.display();

  unsigned long startTime = millis();
  bool lastUp = true, lastDown = true, lastOk = true;

  while (millis() - startTime < 5000) {
    bool up = digitalRead(PIN_BTN_UP);
    bool down = digitalRead(PIN_BTN_DOWN);
    bool ok = digitalRead(PIN_BTN_OK);

    // UP pressed (LOW = pressed with pullup)
    if (!up && lastUp) {
      Serial.println("  [BTN] UP PRESSED!");
      ledcWriteTone(4, 1000);
      delay(50);
      ledcWriteTone(4, 0);
    }

    // DOWN pressed
    if (!down && lastDown) {
      Serial.println("  [BTN] DOWN PRESSED!");
      ledcWriteTone(4, 800);
      delay(50);
      ledcWriteTone(4, 0);
    }

    // OK pressed (with double-click detection)
    if (!ok && lastOk) {
      if (waitingForDoubleClick &&
          (millis() - lastOkClickTime) <= DOUBLE_CLICK_MS) {
        Serial.println("  [BTN] OK DOUBLE-CLICK = BACK!");
        ledcWriteTone(4, 1500);
        delay(50);
        ledcWriteTone(4, 1000);
        delay(50);
        ledcWriteTone(4, 0);
        waitingForDoubleClick = false;
      } else {
        lastOkClickTime = millis();
        waitingForDoubleClick = true;
      }
    }

    // Single click timeout
    if (waitingForDoubleClick &&
        (millis() - lastOkClickTime) > DOUBLE_CLICK_MS) {
      Serial.println("  [BTN] OK SINGLE-CLICK!");
      ledcWriteTone(4, 1200);
      delay(50);
      ledcWriteTone(4, 0);
      waitingForDoubleClick = false;
    }

    lastUp = up;
    lastDown = down;
    lastOk = ok;
    delay(10);
  }

  Serial.println("========== BUTTON TEST COMPLETE ==========\n");
}

// ============================================================
// LED FUNCTIONS
// ============================================================
void testLED() {
  Serial.println("\n========== TEST LED ==========");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("LED TEST");
  display.display();

  for (int i = 0; i < 5; i++) {
    Serial.println("  LED ON");
    digitalWrite(PIN_LED_STATUS, HIGH);
    display.setCursor(0, 20);
    display.println("LED: ON ");
    display.display();
    delay(300);

    Serial.println("  LED OFF");
    digitalWrite(PIN_LED_STATUS, LOW);
    display.setCursor(0, 20);
    display.println("LED: OFF");
    display.display();
    delay(300);
  }

  digitalWrite(PIN_LED_STATUS, HIGH);
  Serial.println("========== LED TEST COMPLETE ==========\n");
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    switch (cmd) {
    case 'm':
      testAllMotors();
      break;
    case '1':
      testSingleMotor(0, PIN_FL_IN1, PIN_FL_IN2, "FL");
      break;
    case '2':
      testSingleMotor(1, PIN_FR_IN1, PIN_FR_IN2, "FR");
      break;
    case '3':
      testSingleMotor(2, PIN_BL_IN1, PIN_BL_IN2, "BL");
      break;
    case '4':
      testSingleMotor(3, PIN_BR_IN1, PIN_BR_IN2, "BR");
      break;
    case 'b':
      testBuzzer();
      break;
    case 'o':
      testOLED();
      break;
    case 't':
      testButtons();
      break;
    case 'l':
      testLED();
      break;
    case 'a':
      testLED();
      testBuzzer();
      testOLED();
      testAllMotors();
      testButtons();
      Serial.println("\n*** ALL TESTS COMPLETE ***\n");
      break;
    case 's':
      stopAllMotors();
      Serial.println("[STOP] All motors stopped");
      break;
    case '\n':
    case '\r':
      break;
    default:
      printMenu();
      break;
    }
  }

  delay(10);
}

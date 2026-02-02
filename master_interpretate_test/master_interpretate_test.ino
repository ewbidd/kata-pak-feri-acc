/**
 * ============================================================
 * ESP32-S3 MASTER - INTERPRETATION TEST
 * ============================================================
 * Test interpretasi master terhadap data yang dikirim slave
 * Output: Serial Monitor (detail + speed) & OLED (status only)
 *
 * Modes:
 * - Mecanum Mode: Maju/Mundur/Strafe Kiri/Strafe Kanan/Rotasi
 * - RC Mode: Maju/Mundur/Belok Kiri/Belok Kanan (no strafe)
 * - Voice Mode: Forward/Backward/Left/Right via voice command
 * - Settings Mode: Brightness, Volume, dll
 *
 * Controls:
 * - UP/DOWN: Navigate menu
 * - OK (single): Select/Enter
 * - OK (double): Back
 * - OK (long): Quick settings
 * ============================================================
 */

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_now.h>

// ============================================================
// PIN DEFINITIONS
// ============================================================
#define PIN_OLED_SDA 9
#define PIN_OLED_SCK 10
#define PIN_BUZZER 11
#define PIN_BTN_UP 12
#define PIN_BTN_DOWN 13
#define PIN_BTN_OK 38
#define PIN_LED_STATUS 48

// Display
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDRESS 0x3C

// Timing
#define DEBOUNCE_MS 50
#define LONG_PRESS_MS 1000
#define DOUBLE_CLICK_MS 400

// EEPROM
#define EEPROM_SIZE 64
#define EEPROM_MAGIC 0xAB
#define EEPROM_ADDR_MAGIC 0
#define EEPROM_ADDR_BRIGHTNESS 1
#define EEPROM_ADDR_VOLUME 2
#define EEPROM_ADDR_LED_MODE 7

// ESP-NOW
#define CONNECTION_TIMEOUT 500 // ms tanpa data = disconnected
#define DEADZONE 25            // Nilai joystick di bawah ini dianggap 0
#define DIAGONAL_RATIO 0.6     // Rasio min untuk deteksi diagonal (mecanum)
#define MAX_SPEED 255

// ============================================================
// ENUMS
// ============================================================
enum class SystemState {
  MAIN_MENU,
  MODE_MECANUM,
  MODE_RC,
  MODE_VOICE,
  MODE_SETTINGS
};

enum class SettingsMenu {
  MAIN,
  BRIGHTNESS,
  VOLUME,
  LED_MODE,
  ABOUT,
  SAVE_EXIT
};

enum class ButtonEvent {
  NONE,
  UP_PRESSED,
  DOWN_PRESSED,
  OK_SINGLE,
  OK_DOUBLE,
  OK_LONG
};

// Movement interpretation
enum class MovementType {
  STOP,
  FORWARD,
  BACKWARD,
  STRAFE_LEFT,
  STRAFE_RIGHT,
  ROTATE_LEFT,
  ROTATE_RIGHT,
  TURN_LEFT,
  TURN_RIGHT,
  EMERGENCY
};

// ============================================================
// VOICE COMMAND DEFINITIONS
// ============================================================
// Voice Command IDs
#define ESPNOW_CMD_STOP 0x00
#define ESPNOW_CMD_FORWARD 0x01
#define ESPNOW_CMD_BACKWARD 0x02
#define ESPNOW_CMD_LEFT 0x03
#define ESPNOW_CMD_RIGHT 0x04
#define ESPNOW_CMD_STRAFE_LEFT 0x05
#define ESPNOW_CMD_STRAFE_RIGHT 0x06
#define ESPNOW_CMD_EMERGENCY 0xFF

// Message Types
#define MSG_TYPE_COMMAND 0x01
#define MSG_TYPE_HEARTBEAT 0x02
#define MSG_TYPE_ACK 0x03
#define MSG_TYPE_EMERGENCY 0xFF

// Voice Packet Header
#define VOICE_PACKET_HEADER 0xAA

// ============================================================
// STRUKTUR DATA YANG DITERIMA DARI SLAVE
// ============================================================
// Joystick data structure
typedef struct {
  int16_t throttle; // -255 to 255 (Y axis)
  int16_t steering; // -255 to 255 (X axis)
  int16_t aux_x;    // tidak dipakai
  int16_t aux_y;    // tidak dipakai
  bool btn1;        // Tombol joystick (emergency stop)
  bool btn2;        // Tombol 2
  uint8_t mode;     // Mode operasi (dari slave, tidak dipakai)
} RemoteData;

// Voice command packet structure (7 bytes)
typedef struct __attribute__((packed)) {
  uint8_t header;       // 0xAA - start marker
  uint8_t msg_type;     // Message type
  uint8_t command;      // Command ID
  uint8_t speed;        // Speed 0-255
  uint16_t duration_ms; // Duration (0 = continuous)
  uint8_t checksum;     // XOR checksum
} VoicePacket;

// ============================================================
// GLOBALS
// ============================================================
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// State
SystemState currentState = SystemState::MAIN_MENU;
SettingsMenu settingsMenu = SettingsMenu::MAIN;
int8_t menuIndex = 0;
int8_t settingsIndex = 0;

// Settings values
uint8_t brightness = 255;
uint8_t volume = 80;
uint8_t ledMode = 1;

const char *ledModeNames[] = {"Off", "On", "Blink", "Breath"};

// ESP-NOW data - Joystick
RemoteData receivedData;
unsigned long lastReceiveTime = 0;
bool slaveConnected = false;

// ESP-NOW data - Voice
VoicePacket voicePacket;
unsigned long lastVoiceReceiveTime = 0;
bool voiceSlaveConnected = false;
uint8_t lastVoiceCommand = ESPNOW_CMD_STOP;
uint8_t lastVoiceSpeed = 0;

// Movement interpretation
MovementType currentMovement = MovementType::STOP;
int interpretedSpeedFL = 0;
int interpretedSpeedFR = 0;
int interpretedSpeedBL = 0;
int interpretedSpeedBR = 0;
int throttleValue = 0;
int strafeValue = 0;
int rotationValue = 0;

// Button state
struct ButtonState {
  bool lastState = true;
  bool currentState = true;
  unsigned long pressTime = 0;
  unsigned long lastClickTime = 0;
  bool waitingDoubleClick = false;
  bool longPressHandled = false;
};
ButtonState btnUp, btnDown, btnOk;

// Menu items
const char *mainMenuItems[] = {"Mecanum Mode", "RC Mode", "Voice Mode",
                               "Settings"};
const uint8_t mainMenuCount = 4;
const char *settingsMenuItems[] = {"Brightness", "Volume", "LED Mode", "About",
                                   "Save & Exit"};
const uint8_t settingsMenuCount = 5;

// Display update control
unsigned long lastDisplayUpdate = 0;
#define DISPLAY_UPDATE_INTERVAL 100 // ms

// ============================================================
// ESP-NOW CALLBACK
// ============================================================
bool validateVoiceChecksum(const uint8_t *data, int len) {
  if (len != sizeof(VoicePacket))
    return false;
  uint8_t checksum = 0;
  for (int i = 0; i < len - 1; i++) {
    checksum ^= data[i];
  }
  return checksum == data[len - 1];
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len) {
  // Check if it's a voice command packet (7 bytes with header 0xAA)
  if (len == sizeof(VoicePacket) && data[0] == VOICE_PACKET_HEADER) {
    // Validate checksum
    if (validateVoiceChecksum(data, len)) {
      memcpy(&voicePacket, data, sizeof(voicePacket));

      // Check msg_type for command or emergency
      if (voicePacket.msg_type == MSG_TYPE_COMMAND ||
          voicePacket.msg_type == MSG_TYPE_EMERGENCY) {
        lastVoiceReceiveTime = millis();
        voiceSlaveConnected = true;
        lastVoiceCommand = voicePacket.command;
        lastVoiceSpeed = voicePacket.speed;

        // LED berkedip saat menerima voice command
        digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
      }
    }
  }
  // Check if it's joystick data
  else if (len == sizeof(RemoteData)) {
    memcpy(&receivedData, data, sizeof(receivedData));
    lastReceiveTime = millis();
    slaveConnected = true;

    // LED berkedip saat menerima data
    digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
  }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("=============================================");
  Serial.println("  MASTER INTERPRETATION TEST");
  Serial.println("  ESP32-S3 - Serial Monitor & OLED Output");
  Serial.println("=============================================");

  // Init pins
  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BTN_OK, INPUT_PULLUP);
  pinMode(PIN_LED_STATUS, OUTPUT);
  digitalWrite(PIN_LED_STATUS, HIGH);

  // Init buzzer
  ledcSetup(0, 2000, 8);
  ledcAttachPin(PIN_BUZZER, 0);

  // Init EEPROM
  EEPROM.begin(EEPROM_SIZE);
  loadSettings();

  // Init OLED
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCK);
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    display.setTextColor(SSD1306_WHITE);
    applyBrightness();
    showBootScreen();
  }

  // Init ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.println("=============================================");
  Serial.print("MAC ADDRESS MASTER: ");
  Serial.println(WiFi.macAddress());
  Serial.println("=============================================");

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: Gagal inisialisasi ESP-NOW!");
    display.clearDisplay();
    display.setCursor(0, 20);
    display.println("ESP-NOW FAILED!");
    display.display();
    while (1) {
      digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
      delay(100);
    }
  }
  Serial.println("ESP-NOW berhasil diinisialisasi");

  // Daftarkan callback receive
  esp_now_register_recv_cb(OnDataRecv);

  // Inisialisasi data
  receivedData.throttle = 0;
  receivedData.steering = 0;
  receivedData.aux_x = 0;
  receivedData.aux_y = 0;
  receivedData.btn1 = false;
  receivedData.btn2 = false;
  receivedData.mode = 0;

  // Startup sound
  playTone(1000, 100);
  playTone(1500, 100);
  playTone(2000, 100);

  delay(1000);
  drawMainMenu();

  Serial.println("");
  Serial.println("MODE TERSEDIA:");
  Serial.println("  1. Mecanum - Strafe + Rotasi (joystick)");
  Serial.println("  2. RC Basic - Belok tanpa strafe (joystick)");
  Serial.println("  3. Voice - Voice command (INMP441)");
  Serial.println("");
  Serial.println("Menunggu data dari slave...");
  Serial.println("---------------------------------------------");
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
  // Check connection timeout
  checkConnection();

  // Read buttons
  ButtonEvent event = readButtons();
  if (event != ButtonEvent::NONE) {
    handleEvent(event);
  }

  // Process data if in active mode
  if (currentState == SystemState::MODE_MECANUM ||
      currentState == SystemState::MODE_RC ||
      currentState == SystemState::MODE_VOICE) {
    processSlaveData();

    // Update display periodically
    if (millis() - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL) {
      updateModeDisplay();
      printSerialDebug();
      lastDisplayUpdate = millis();
    }
  }

  delay(10);
}

// ============================================================
// CONNECTION CHECK
// ============================================================
void checkConnection() {
  // Check joystick slave connection
  if (millis() - lastReceiveTime > CONNECTION_TIMEOUT) {
    if (slaveConnected) {
      Serial.println("!!! JOYSTICK SLAVE DISCONNECTED !!!");
      slaveConnected = false;
      if (currentState != SystemState::MODE_VOICE) {
        currentMovement = MovementType::STOP;
        interpretedSpeedFL = 0;
        interpretedSpeedFR = 0;
        interpretedSpeedBL = 0;
        interpretedSpeedBR = 0;
      }
    }
  }

  // Check voice slave connection
  if (millis() - lastVoiceReceiveTime > CONNECTION_TIMEOUT) {
    if (voiceSlaveConnected) {
      Serial.println("!!! VOICE SLAVE DISCONNECTED !!!");
      voiceSlaveConnected = false;
      if (currentState == SystemState::MODE_VOICE) {
        currentMovement = MovementType::STOP;
        interpretedSpeedFL = 0;
        interpretedSpeedFR = 0;
        interpretedSpeedBL = 0;
        interpretedSpeedBR = 0;
        lastVoiceCommand = ESPNOW_CMD_STOP;
        lastVoiceSpeed = 0;
      }
    }
  }

  // LED status based on current mode connection
  if (currentState == SystemState::MODE_VOICE) {
    if (!voiceSlaveConnected)
      digitalWrite(PIN_LED_STATUS, LOW);
  } else {
    if (!slaveConnected)
      digitalWrite(PIN_LED_STATUS, LOW);
  }
}

// ============================================================
// PROCESS SLAVE DATA
// ============================================================
int applyDeadzone(int value) {
  if (abs(value) < DEADZONE) {
    return 0;
  }
  return value;
}

void processSlaveData() {
  // Voice mode uses different connection check
  if (currentState == SystemState::MODE_VOICE) {
    processVoiceMode();
    return;
  }

  // Joystick modes
  if (!slaveConnected) {
    currentMovement = MovementType::STOP;
    interpretedSpeedFL = 0;
    interpretedSpeedFR = 0;
    interpretedSpeedBL = 0;
    interpretedSpeedBR = 0;
    return;
  }

  // Emergency stop from joystick button
  if (receivedData.btn1) {
    currentMovement = MovementType::STOP;
    interpretedSpeedFL = 0;
    interpretedSpeedFR = 0;
    interpretedSpeedBL = 0;
    interpretedSpeedBR = 0;
    return;
  }

  switch (currentState) {
  case SystemState::MODE_MECANUM:
    processMecanumMode();
    break;
  case SystemState::MODE_RC:
    processRCMode();
    break;
  default:
    break;
  }
}

// ============================================================
// MECANUM MODE PROCESSING
// ============================================================
void processMecanumMode() {
  int y = applyDeadzone(receivedData.throttle);
  int x = applyDeadzone(receivedData.steering);

  throttleValue = 0;
  strafeValue = 0;
  rotationValue = 0;

  if (x == 0 && y == 0) {
    currentMovement = MovementType::STOP;
    interpretedSpeedFL = 0;
    interpretedSpeedFR = 0;
    interpretedSpeedBL = 0;
    interpretedSpeedBR = 0;
    return;
  }

  int absX = abs(x);
  int absY = abs(y);

  // Cek diagonal untuk rotasi
  bool isDiagonal = false;
  if (absX > DEADZONE && absY > DEADZONE) {
    float ratio = (float)min(absX, absY) / (float)max(absX, absY);
    isDiagonal = (ratio >= DIAGONAL_RATIO);
  }

  if (isDiagonal) {
    // ROTASI
    int magnitude = max(absX, absY);
    rotationValue = (x > 0) ? magnitude : -magnitude;
    currentMovement =
        (x > 0) ? MovementType::ROTATE_RIGHT : MovementType::ROTATE_LEFT;
  } else if (absY > absX) {
    // MAJU/MUNDUR
    throttleValue = y;
    currentMovement = (y > 0) ? MovementType::FORWARD : MovementType::BACKWARD;
  } else {
    // STRAFE
    strafeValue = x;
    currentMovement =
        (x > 0) ? MovementType::STRAFE_RIGHT : MovementType::STRAFE_LEFT;
  }

  // Calculate mecanum wheel speeds
  calculateMecanumSpeeds();
}

void calculateMecanumSpeeds() {
  /*
   * Formula Mecanum:
   * FL = throttle + strafe + rotation
   * FR = throttle - strafe - rotation
   * BL = throttle - strafe + rotation
   * BR = throttle + strafe - rotation
   */
  interpretedSpeedFL = throttleValue + strafeValue + rotationValue;
  interpretedSpeedFR = throttleValue - strafeValue - rotationValue;
  interpretedSpeedBL = throttleValue - strafeValue + rotationValue;
  interpretedSpeedBR = throttleValue + strafeValue - rotationValue;

  // Normalisasi
  int maxSpeed = max(max(abs(interpretedSpeedFL), abs(interpretedSpeedFR)),
                     max(abs(interpretedSpeedBL), abs(interpretedSpeedBR)));

  if (maxSpeed > MAX_SPEED) {
    float scale = (float)MAX_SPEED / maxSpeed;
    interpretedSpeedFL = (int)(interpretedSpeedFL * scale);
    interpretedSpeedFR = (int)(interpretedSpeedFR * scale);
    interpretedSpeedBL = (int)(interpretedSpeedBL * scale);
    interpretedSpeedBR = (int)(interpretedSpeedBR * scale);
  }
}

// ============================================================
// RC MODE PROCESSING (No Strafe - Basic Turn)
// ============================================================
void processRCMode() {
  int y = applyDeadzone(receivedData.throttle);
  int x = applyDeadzone(receivedData.steering);

  throttleValue = y;
  rotationValue = 0;

  if (x == 0 && y == 0) {
    currentMovement = MovementType::STOP;
    interpretedSpeedFL = 0;
    interpretedSpeedFR = 0;
    interpretedSpeedBL = 0;
    interpretedSpeedBR = 0;
    return;
  }

  // RC mode: throttle + steering mixed
  // Left wheels = throttle + steering
  // Right wheels = throttle - steering
  int leftSpeed = y + x;
  int rightSpeed = y - x;

  // Determine movement type
  if (y == 0 && x != 0) {
    // Pivot turn (rotate in place)
    currentMovement =
        (x > 0) ? MovementType::TURN_RIGHT : MovementType::TURN_LEFT;
  } else if (y > 0) {
    if (x > 0) {
      currentMovement = MovementType::TURN_RIGHT; // Forward + turn right
    } else if (x < 0) {
      currentMovement = MovementType::TURN_LEFT; // Forward + turn left
    } else {
      currentMovement = MovementType::FORWARD;
    }
  } else if (y < 0) {
    if (x > 0) {
      currentMovement = MovementType::TURN_RIGHT; // Backward + turn right
    } else if (x < 0) {
      currentMovement = MovementType::TURN_LEFT; // Backward + turn left
    } else {
      currentMovement = MovementType::BACKWARD;
    }
  }

  // Normalize speeds
  int maxSpeed = max(abs(leftSpeed), abs(rightSpeed));
  if (maxSpeed > MAX_SPEED) {
    float scale = (float)MAX_SPEED / maxSpeed;
    leftSpeed = (int)(leftSpeed * scale);
    rightSpeed = (int)(rightSpeed * scale);
  }

  // Apply to 4 wheels (tank style - left side same, right side same)
  interpretedSpeedFL = leftSpeed;
  interpretedSpeedBL = leftSpeed;
  interpretedSpeedFR = rightSpeed;
  interpretedSpeedBR = rightSpeed;
}

// ============================================================
// VOICE MODE PROCESSING
// ============================================================
void processVoiceMode() {
  if (!voiceSlaveConnected) {
    currentMovement = MovementType::STOP;
    interpretedSpeedFL = 0;
    interpretedSpeedFR = 0;
    interpretedSpeedBL = 0;
    interpretedSpeedBR = 0;
    return;
  }

  // Get speed from voice packet
  int speed = lastVoiceSpeed;

  // Map voice command to movement
  switch (lastVoiceCommand) {
  case ESPNOW_CMD_STOP:
    currentMovement = MovementType::STOP;
    interpretedSpeedFL = 0;
    interpretedSpeedFR = 0;
    interpretedSpeedBL = 0;
    interpretedSpeedBR = 0;
    break;

  case ESPNOW_CMD_FORWARD:
    currentMovement = MovementType::FORWARD;
    interpretedSpeedFL = speed;
    interpretedSpeedFR = speed;
    interpretedSpeedBL = speed;
    interpretedSpeedBR = speed;
    break;

  case ESPNOW_CMD_BACKWARD:
    currentMovement = MovementType::BACKWARD;
    interpretedSpeedFL = -speed;
    interpretedSpeedFR = -speed;
    interpretedSpeedBL = -speed;
    interpretedSpeedBR = -speed;
    break;

  case ESPNOW_CMD_LEFT:
    currentMovement = MovementType::TURN_LEFT;
    // Turn left: left wheels backward, right wheels forward
    interpretedSpeedFL = -speed;
    interpretedSpeedBL = -speed;
    interpretedSpeedFR = speed;
    interpretedSpeedBR = speed;
    break;

  case ESPNOW_CMD_RIGHT:
    currentMovement = MovementType::TURN_RIGHT;
    // Turn right: left wheels forward, right wheels backward
    interpretedSpeedFL = speed;
    interpretedSpeedBL = speed;
    interpretedSpeedFR = -speed;
    interpretedSpeedBR = -speed;
    break;

  case ESPNOW_CMD_STRAFE_LEFT:
    currentMovement = MovementType::STRAFE_LEFT;
    // Mecanum strafe left
    interpretedSpeedFL = -speed;
    interpretedSpeedFR = speed;
    interpretedSpeedBL = speed;
    interpretedSpeedBR = -speed;
    break;

  case ESPNOW_CMD_STRAFE_RIGHT:
    currentMovement = MovementType::STRAFE_RIGHT;
    // Mecanum strafe right
    interpretedSpeedFL = speed;
    interpretedSpeedFR = -speed;
    interpretedSpeedBL = -speed;
    interpretedSpeedBR = speed;
    break;

  case ESPNOW_CMD_EMERGENCY:
    currentMovement = MovementType::EMERGENCY;
    interpretedSpeedFL = 0;
    interpretedSpeedFR = 0;
    interpretedSpeedBL = 0;
    interpretedSpeedBR = 0;
    break;

  default:
    currentMovement = MovementType::STOP;
    interpretedSpeedFL = 0;
    interpretedSpeedFR = 0;
    interpretedSpeedBL = 0;
    interpretedSpeedBR = 0;
    break;
  }
}

// ============================================================
// GET MOVEMENT STRING
// ============================================================
const char *getMovementString() {
  switch (currentMovement) {
  case MovementType::STOP:
    return "STOP";
  case MovementType::FORWARD:
    return "FORWARD";
  case MovementType::BACKWARD:
    return "BACKWARD";
  case MovementType::STRAFE_LEFT:
    return "STRAFE LEFT";
  case MovementType::STRAFE_RIGHT:
    return "STRAFE RIGHT";
  case MovementType::ROTATE_LEFT:
    return "ROTATE LEFT";
  case MovementType::ROTATE_RIGHT:
    return "ROTATE RIGHT";
  case MovementType::TURN_LEFT:
    return "TURN LEFT";
  case MovementType::TURN_RIGHT:
    return "TURN RIGHT";
  case MovementType::EMERGENCY:
    return "EMERGENCY STOP";
  default:
    return "UNKNOWN";
  }
}

const char *getVoiceCommandString() {
  switch (lastVoiceCommand) {
  case ESPNOW_CMD_STOP:
    return "STOP";
  case ESPNOW_CMD_FORWARD:
    return "FORWARD";
  case ESPNOW_CMD_BACKWARD:
    return "BACKWARD";
  case ESPNOW_CMD_LEFT:
    return "LEFT";
  case ESPNOW_CMD_RIGHT:
    return "RIGHT";
  case ESPNOW_CMD_STRAFE_LEFT:
    return "STRAFE_LEFT";
  case ESPNOW_CMD_STRAFE_RIGHT:
    return "STRAFE_RIGHT";
  case ESPNOW_CMD_EMERGENCY:
    return "EMERGENCY";
  default:
    return "UNKNOWN";
  }
}

const char *getModePrefix() {
  switch (currentState) {
  case SystemState::MODE_MECANUM:
    return "MECANUM";
  case SystemState::MODE_RC:
    return "RC";
  case SystemState::MODE_VOICE:
    return "VOICE";
  default:
    return "";
  }
}

// ============================================================
// SERIAL DEBUG OUTPUT
// ============================================================
void printSerialDebug() {
  Serial.print("[");
  Serial.print(getModePrefix());
  Serial.print("] ");

  // Voice mode has different debug output
  if (currentState == SystemState::MODE_VOICE) {
    if (!voiceSlaveConnected) {
      Serial.println("DISCONNECTED");
      return;
    }

    Serial.print("Cmd: ");
    Serial.print(getVoiceCommandString());
    Serial.print(" (0x");
    Serial.print(lastVoiceCommand, HEX);
    Serial.print(") Speed: ");
    Serial.print(lastVoiceSpeed);
    Serial.print(" | Status: ");
    Serial.print(getMovementString());
    Serial.print(" | FL:");
    Serial.print(interpretedSpeedFL);
    Serial.print(" FR:");
    Serial.print(interpretedSpeedFR);
    Serial.print(" BL:");
    Serial.print(interpretedSpeedBL);
    Serial.print(" BR:");
    Serial.println(interpretedSpeedBR);
    return;
  }

  // Joystick mode debug output
  if (!slaveConnected) {
    Serial.println("DISCONNECTED");
    return;
  }

  Serial.print("Raw Y:");
  Serial.print(receivedData.throttle);
  Serial.print(" X:");
  Serial.print(receivedData.steering);
  Serial.print(" | Status: ");
  Serial.print(getMovementString());
  Serial.print(" | Speed FL:");
  Serial.print(interpretedSpeedFL);
  Serial.print(" FR:");
  Serial.print(interpretedSpeedFR);
  Serial.print(" BL:");
  Serial.print(interpretedSpeedBL);
  Serial.print(" BR:");
  Serial.println(interpretedSpeedBR);
}

// ============================================================
// BUTTON HANDLING
// ============================================================
ButtonEvent readButtons() {
  ButtonEvent event = ButtonEvent::NONE;
  unsigned long now = millis();

  btnUp.currentState = digitalRead(PIN_BTN_UP);
  btnDown.currentState = digitalRead(PIN_BTN_DOWN);
  btnOk.currentState = digitalRead(PIN_BTN_OK);

  // UP button
  if (!btnUp.currentState && btnUp.lastState) {
    event = ButtonEvent::UP_PRESSED;
    playClick();
  }

  // DOWN button
  if (!btnDown.currentState && btnDown.lastState) {
    event = ButtonEvent::DOWN_PRESSED;
    playClick();
  }

  // OK button - complex handling
  if (!btnOk.currentState && btnOk.lastState) {
    btnOk.pressTime = now;
    btnOk.longPressHandled = false;
  }

  // Long press detection
  if (!btnOk.currentState && !btnOk.longPressHandled) {
    if (now - btnOk.pressTime >= LONG_PRESS_MS) {
      event = ButtonEvent::OK_LONG;
      btnOk.longPressHandled = true;
      btnOk.waitingDoubleClick = false;
      playLongPress();
    }
  }

  // Button released
  if (btnOk.currentState && !btnOk.lastState && !btnOk.longPressHandled) {
    if (btnOk.waitingDoubleClick &&
        (now - btnOk.lastClickTime) <= DOUBLE_CLICK_MS) {
      event = ButtonEvent::OK_DOUBLE;
      btnOk.waitingDoubleClick = false;
      playDoubleClick();
    } else {
      btnOk.lastClickTime = now;
      btnOk.waitingDoubleClick = true;
    }
  }

  // Single click timeout
  if (btnOk.waitingDoubleClick &&
      (now - btnOk.lastClickTime) > DOUBLE_CLICK_MS) {
    event = ButtonEvent::OK_SINGLE;
    btnOk.waitingDoubleClick = false;
    playClick();
  }

  btnUp.lastState = btnUp.currentState;
  btnDown.lastState = btnDown.currentState;
  btnOk.lastState = btnOk.currentState;

  return event;
}

// ============================================================
// EVENT HANDLING
// ============================================================
void handleEvent(ButtonEvent event) {
  switch (currentState) {
  case SystemState::MAIN_MENU:
    handleMainMenu(event);
    break;
  case SystemState::MODE_MECANUM:
  case SystemState::MODE_RC:
  case SystemState::MODE_VOICE:
    handleActiveMode(event);
    break;
  case SystemState::MODE_SETTINGS:
    handleSettingsMode(event);
    break;
  }
}

void handleMainMenu(ButtonEvent event) {
  switch (event) {
  case ButtonEvent::UP_PRESSED:
    menuIndex = (menuIndex - 1 + mainMenuCount) % mainMenuCount;
    drawMainMenu();
    break;

  case ButtonEvent::DOWN_PRESSED:
    menuIndex = (menuIndex + 1) % mainMenuCount;
    drawMainMenu();
    break;

  case ButtonEvent::OK_SINGLE:
    enterMode(menuIndex);
    break;

  case ButtonEvent::OK_LONG:
    currentState = SystemState::MODE_SETTINGS;
    settingsMenu = SettingsMenu::MAIN;
    settingsIndex = 0;
    drawSettingsMenu();
    break;

  default:
    break;
  }
}

void handleActiveMode(ButtonEvent event) {
  if (event == ButtonEvent::OK_DOUBLE) {
    // Return to main menu
    currentState = SystemState::MAIN_MENU;
    currentMovement = MovementType::STOP;
    Serial.println("---------------------------------------------");
    Serial.println("Kembali ke Main Menu");
    Serial.println("---------------------------------------------");
    drawMainMenu();
  }
}

void handleSettingsMode(ButtonEvent event) {
  switch (settingsMenu) {
  case SettingsMenu::MAIN:
    handleSettingsMain(event);
    break;
  case SettingsMenu::BRIGHTNESS:
    handleBrightness(event);
    break;
  case SettingsMenu::VOLUME:
    handleVolume(event);
    break;
  case SettingsMenu::LED_MODE:
    handleLedMode(event);
    break;
  case SettingsMenu::ABOUT:
    handleAbout(event);
    break;
  case SettingsMenu::SAVE_EXIT:
    break;
  }
}

void handleSettingsMain(ButtonEvent event) {
  switch (event) {
  case ButtonEvent::UP_PRESSED:
    settingsIndex = (settingsIndex - 1 + settingsMenuCount) % settingsMenuCount;
    drawSettingsMenu();
    break;

  case ButtonEvent::DOWN_PRESSED:
    settingsIndex = (settingsIndex + 1) % settingsMenuCount;
    drawSettingsMenu();
    break;

  case ButtonEvent::OK_SINGLE:
    enterSettingsSubmenu(settingsIndex);
    break;

  case ButtonEvent::OK_DOUBLE:
    currentState = SystemState::MAIN_MENU;
    drawMainMenu();
    break;

  default:
    break;
  }
}

void handleBrightness(ButtonEvent event) {
  switch (event) {
  case ButtonEvent::UP_PRESSED:
    if (brightness < 255) {
      brightness = min(255, brightness + 25);
      applyBrightness();
      drawBrightnessScreen();
    }
    break;

  case ButtonEvent::DOWN_PRESSED:
    if (brightness > 25) {
      brightness = max(25, brightness - 25);
      applyBrightness();
      drawBrightnessScreen();
    }
    break;

  case ButtonEvent::OK_SINGLE:
  case ButtonEvent::OK_DOUBLE:
    settingsMenu = SettingsMenu::MAIN;
    drawSettingsMenu();
    break;

  default:
    break;
  }
}

void handleVolume(ButtonEvent event) {
  switch (event) {
  case ButtonEvent::UP_PRESSED:
    if (volume < 100) {
      volume = (volume + 10 <= 100) ? volume + 10 : 100;
      drawVolumeScreen();
      playClick();
    }
    break;

  case ButtonEvent::DOWN_PRESSED:
    if (volume > 0) {
      volume = (volume >= 10) ? volume - 10 : 0;
      drawVolumeScreen();
      if (volume > 0)
        playClick();
    }
    break;

  case ButtonEvent::OK_SINGLE:
  case ButtonEvent::OK_DOUBLE:
    settingsMenu = SettingsMenu::MAIN;
    drawSettingsMenu();
    break;

  default:
    break;
  }
}

void handleLedMode(ButtonEvent event) {
  switch (event) {
  case ButtonEvent::UP_PRESSED:
    ledMode = (ledMode + 1) % 4;
    applyLedMode();
    drawLedModeScreen();
    break;

  case ButtonEvent::DOWN_PRESSED:
    ledMode = (ledMode + 3) % 4;
    applyLedMode();
    drawLedModeScreen();
    break;

  case ButtonEvent::OK_SINGLE:
  case ButtonEvent::OK_DOUBLE:
    settingsMenu = SettingsMenu::MAIN;
    drawSettingsMenu();
    break;

  default:
    break;
  }
}

void handleAbout(ButtonEvent event) {
  if (event == ButtonEvent::OK_SINGLE || event == ButtonEvent::OK_DOUBLE) {
    settingsMenu = SettingsMenu::MAIN;
    drawSettingsMenu();
  }
}

// ============================================================
// NAVIGATION
// ============================================================
void enterMode(int index) {
  switch (index) {
  case 0:
    currentState = SystemState::MODE_MECANUM;
    Serial.println("=============================================");
    Serial.println("  MECANUM MODE AKTIF");
    Serial.println("  Gerakan: Maju/Mundur/Strafe/Rotasi");
    Serial.println("=============================================");
    break;
  case 1:
    currentState = SystemState::MODE_RC;
    Serial.println("=============================================");
    Serial.println("  RC MODE AKTIF");
    Serial.println("  Gerakan: Maju/Mundur/Belok (tanpa strafe)");
    Serial.println("=============================================");
    break;
  case 2:
    currentState = SystemState::MODE_VOICE;
    Serial.println("=============================================");
    Serial.println("  VOICE MODE AKTIF (placeholder)");
    Serial.println("=============================================");
    break;
  case 3:
    currentState = SystemState::MODE_SETTINGS;
    settingsMenu = SettingsMenu::MAIN;
    settingsIndex = 0;
    drawSettingsMenu();
    return;
  }
  updateModeDisplay();
}

void enterSettingsSubmenu(int index) {
  switch (index) {
  case 0:
    settingsMenu = SettingsMenu::BRIGHTNESS;
    drawBrightnessScreen();
    break;
  case 1:
    settingsMenu = SettingsMenu::VOLUME;
    drawVolumeScreen();
    break;
  case 2:
    settingsMenu = SettingsMenu::LED_MODE;
    drawLedModeScreen();
    break;
  case 3:
    settingsMenu = SettingsMenu::ABOUT;
    drawAboutScreen();
    break;
  case 4:
    // Save & Exit
    saveSettings();
    playTone(2000, 100);
    playTone(2500, 100);
    currentState = SystemState::MAIN_MENU;
    drawMainMenu();
    break;
  }
}

// ============================================================
// DISPLAY FUNCTIONS
// ============================================================
void applyBrightness() {
  display.ssd1306_command(0x81);
  display.ssd1306_command(brightness);
}

void applyLedMode() {
  switch (ledMode) {
  case 0:
    digitalWrite(PIN_LED_STATUS, LOW);
    break;
  case 1:
    digitalWrite(PIN_LED_STATUS, HIGH);
    break;
  case 2:
  case 3:
    digitalWrite(PIN_LED_STATUS, HIGH);
    break;
  }
}

void showBootScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(5, 5);
  display.println("INTERPRET");
  display.setCursor(25, 25);
  display.println("TEST");
  display.setTextSize(1);
  display.setCursor(25, 50);
  display.println("ESP32-S3 Master");
  display.display();
}

void drawMainMenu() {
  display.clearDisplay();

  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SELECT MODE");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Connection status
  display.setCursor(90, 0);
  display.print(slaveConnected ? "[CON]" : "[---]");

  // Menu items
  int startIdx = (menuIndex > 1) ? menuIndex - 1 : 0;
  int endIdx =
      (startIdx + 3 < (int)mainMenuCount) ? startIdx + 3 : (int)mainMenuCount;

  int row = 0;
  for (int i = startIdx; i < endIdx; i++) {
    int y = 16 + row * 12;
    if (i == menuIndex) {
      display.fillRect(0, y - 1, 128, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(4, y);
    display.print(mainMenuItems[i]);
    display.setTextColor(SSD1306_WHITE);
    row++;
  }

  // Footer
  display.setCursor(0, 56);
  display.print("OK:Select");

  display.display();
}

void updateModeDisplay() {
  display.clearDisplay();

  // Header with mode name
  display.setTextSize(1);
  display.setCursor(0, 0);

  switch (currentState) {
  case SystemState::MODE_MECANUM:
    display.println("MECANUM MODE");
    break;
  case SystemState::MODE_RC:
    display.println("RC MODE");
    break;
  case SystemState::MODE_VOICE:
    display.println("VOICE MODE");
    break;
  default:
    break;
  }

  // Connection status - based on current mode
  display.setCursor(90, 0);
  bool isConnected = (currentState == SystemState::MODE_VOICE)
                         ? voiceSlaveConnected
                         : slaveConnected;
  display.print(isConnected ? "[CON]" : "[---]");

  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Movement status (large text)
  display.setTextSize(2);
  display.setCursor(0, 18);

  if (!isConnected) {
    display.println("WAITING");
    display.setCursor(0, 36);
    display.println("SLAVE...");
  } else {
    // Display movement status based on type
    switch (currentMovement) {
    case MovementType::STOP:
      display.setCursor(30, 25);
      display.println("STOP");
      break;
    case MovementType::FORWARD:
      display.setCursor(10, 25);
      display.println("FORWARD");
      break;
    case MovementType::BACKWARD:
      display.setCursor(5, 25);
      display.println("BACKWARD");
      break;
    case MovementType::STRAFE_LEFT:
      display.setTextSize(1);
      display.setCursor(25, 25);
      display.println("STRAFE LEFT");
      break;
    case MovementType::STRAFE_RIGHT:
      display.setTextSize(1);
      display.setCursor(20, 25);
      display.println("STRAFE RIGHT");
      break;
    case MovementType::ROTATE_LEFT:
      display.setTextSize(1);
      display.setCursor(20, 25);
      display.println("ROTATE LEFT");
      break;
    case MovementType::ROTATE_RIGHT:
      display.setTextSize(1);
      display.setCursor(15, 25);
      display.println("ROTATE RIGHT");
      break;
    case MovementType::TURN_LEFT:
      display.setTextSize(1);
      display.setCursor(25, 25);
      display.println("TURN LEFT");
      break;
    case MovementType::TURN_RIGHT:
      display.setTextSize(1);
      display.setCursor(20, 25);
      display.println("TURN RIGHT");
      break;
    case MovementType::EMERGENCY:
      display.setTextSize(2);
      display.setCursor(0, 25);
      display.println("EMERGENCY!");
      break;
    default:
      break;
    }
  }

  // Footer
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.println("2x OK to back");

  display.display();
}

void drawSettingsMenu() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SETTINGS");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  int startIdx = (settingsIndex > 1) ? settingsIndex - 1 : 0;
  int endIdx = (startIdx + 3 < (int)settingsMenuCount) ? startIdx + 3
                                                       : (int)settingsMenuCount;

  int row = 0;
  for (int i = startIdx; i < endIdx; i++) {
    int y = 16 + row * 12;
    if (i == settingsIndex) {
      display.fillRect(0, y - 1, 128, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(4, y);
    display.print(settingsMenuItems[i]);
    display.setTextColor(SSD1306_WHITE);
    row++;
  }

  display.setCursor(0, 56);
  display.print("OK:Enter 2xOK:Back");

  display.display();
}

void drawBrightnessScreen() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("BRIGHTNESS");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  int barWidth = map(brightness, 0, 255, 0, 100);
  display.drawRect(14, 25, 100, 15, SSD1306_WHITE);
  display.fillRect(14, 25, barWidth, 15, SSD1306_WHITE);

  display.setCursor(45, 45);
  display.printf("%d%%", (brightness * 100) / 255);

  display.setCursor(0, 56);
  display.println("UP/DN:Adj OK:Done");

  display.display();
}

void drawVolumeScreen() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("VOLUME");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  int barWidth = volume;
  display.drawRect(14, 25, 100, 15, SSD1306_WHITE);
  display.fillRect(14, 25, barWidth, 15, SSD1306_WHITE);

  display.setCursor(50, 45);
  display.printf("%d%%", volume);

  display.setCursor(0, 56);
  display.println("UP/DN:Adj OK:Done");

  display.display();
}

void drawLedModeScreen() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("LED MODE");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(25, 25);
  display.println(ledModeNames[ledMode]);

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.println("UP/DN:Mode OK:Done");

  display.display();
}

void drawAboutScreen() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ABOUT");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 18);
  display.println("Interpretation Test");
  display.println("Version: 1.0.0");
  display.println("Build: Feb 2026");
  display.println("");
  display.println("Slave -> Master Test");

  display.display();
}

// ============================================================
// SOUND FUNCTIONS
// ============================================================
void playTone(int freq, int duration) {
  if (volume == 0)
    return;
  ledcWriteTone(0, freq);
  delay(duration);
  ledcWriteTone(0, 0);
}

void playClick() {
  if (volume > 0) {
    ledcWriteTone(0, 1500);
    delay(30);
    ledcWriteTone(0, 0);
  }
}

void playDoubleClick() {
  if (volume > 0) {
    ledcWriteTone(0, 1500);
    delay(30);
    ledcWriteTone(0, 0);
    delay(30);
    ledcWriteTone(0, 1500);
    delay(30);
    ledcWriteTone(0, 0);
  }
}

void playLongPress() {
  if (volume > 0) {
    ledcWriteTone(0, 800);
    delay(100);
    ledcWriteTone(0, 1200);
    delay(100);
    ledcWriteTone(0, 0);
  }
}

// ============================================================
// EEPROM FUNCTIONS
// ============================================================
void loadSettings() {
  if (EEPROM.read(EEPROM_ADDR_MAGIC) == EEPROM_MAGIC) {
    brightness = EEPROM.read(EEPROM_ADDR_BRIGHTNESS);
    volume = EEPROM.read(EEPROM_ADDR_VOLUME);
    ledMode = EEPROM.read(EEPROM_ADDR_LED_MODE);
    Serial.println("[EEPROM] Settings loaded");
  } else {
    Serial.println("[EEPROM] No saved settings, using defaults");
  }
}

void saveSettings() {
  EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);
  EEPROM.write(EEPROM_ADDR_BRIGHTNESS, brightness);
  EEPROM.write(EEPROM_ADDR_VOLUME, volume);
  EEPROM.write(EEPROM_ADDR_LED_MODE, ledMode);
  EEPROM.commit();
  Serial.println("[EEPROM] Settings saved");
}

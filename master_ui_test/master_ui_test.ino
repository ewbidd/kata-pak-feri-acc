/**
 * ============================================================
 * ESP32-S3 MASTER - EVENT-DRIVEN UI TEST
 * ============================================================
 * Test menu system dengan button navigation (tanpa serial monitor)
 *
 * Modes:
 * - Mecanum Mode (placeholder)
 * - RC Mode (placeholder)
 * - Voice Mode (placeholder)
 * - Settings Mode (FULL)
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
#include <Wire.h>

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

// Motor pins (untuk test di settings)
#define PIN_FL_ENA 4
#define PIN_FL_IN1 14
#define PIN_FL_IN2 21
#define PIN_FR_ENA 5
#define PIN_FR_IN1 16
#define PIN_FR_IN2 17
#define PIN_BL_ENA 18
#define PIN_BL_IN1 19
#define PIN_BL_IN2 20
#define PIN_BR_ENA 7
#define PIN_BR_IN1 15
#define PIN_BR_IN2 8

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
#define EEPROM_ADDR_MOTOR_CAL 3

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
  SLEEP_TIMER,
  MOTOR_TEST,
  CALIBRATION,
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

// ============================================================
// GLOBALS
// ============================================================
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// State
SystemState currentState = SystemState::MAIN_MENU;
SettingsMenu settingsMenu = SettingsMenu::MAIN;
int8_t menuIndex = 0;
int8_t settingsIndex = 0;
int8_t motorTestIndex = 0;
bool motorRunning = false;

// Settings values
uint8_t brightness = 255;
uint8_t volume = 80;    // 0-100%
uint8_t ledMode = 0;    // 0=Off, 1=On, 2=Blink, 3=Breath
uint8_t sleepTimer = 0; // 0=Off, 1=1min, 2=5min, 3=10min
uint8_t motorCalibration[4] = {255, 255, 255,
                               255}; // FL, FR, BL, BR (speed 10-255)

const char *ledModeNames[] = {"Off", "On", "Blink", "Breath"};
const char *sleepTimerNames[] = {"Off", "1 min", "5 min", "10 min"};

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

const char *settingsMenuItems[] = {"Brightness",  "Volume",     "LED Mode",
                                   "Sleep Timer", "Motor Test", "Calibration",
                                   "About",       "Save & Exit"};
const uint8_t settingsMenuCount = 8;

const char *motorNames[] = {"Front Left", "Front Right", "Back Left",
                            "Back Right", "All Motors"};

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  // Init pins
  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BTN_OK, INPUT_PULLUP);
  pinMode(PIN_LED_STATUS, OUTPUT);
  digitalWrite(PIN_LED_STATUS, HIGH);

  // Init buzzer
  ledcSetup(0, 2000, 8);
  ledcAttachPin(PIN_BUZZER, 0);

  // Init motors
  initMotors();

  // Init EEPROM
  EEPROM.begin(EEPROM_SIZE);
  loadSettings();

  // Init OLED
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCK);
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    display.setTextColor(SSD1306_WHITE);
    applyBrightness(); // Apply saved brightness
    showBootScreen();
  }

  // Startup sound
  playTone(1000, 100);
  playTone(1500, 100);
  playTone(2000, 100);

  delay(1000);
  drawMainMenu();
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
  ButtonEvent event = readButtons();

  if (event != ButtonEvent::NONE) {
    handleEvent(event);
  }

  // Motor test auto-update
  if (currentState == SystemState::MODE_SETTINGS &&
      settingsMenu == SettingsMenu::MOTOR_TEST && motorRunning) {
    // Motor akan terus jalan selama di test mode
  }

  delay(10);
}

// ============================================================
// BUTTON HANDLING
// ============================================================
ButtonEvent readButtons() {
  ButtonEvent event = ButtonEvent::NONE;
  unsigned long now = millis();

  // Read current states
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

  // OK button - complex handling for single/double/long
  if (!btnOk.currentState && btnOk.lastState) {
    // Button just pressed
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
      // Double click!
      event = ButtonEvent::OK_DOUBLE;
      btnOk.waitingDoubleClick = false;
      playDoubleClick();
    } else {
      // Start waiting for possible double click
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

  // Update last states
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
    handlePlaceholderMode(event);
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
    // Quick settings access
    currentState = SystemState::MODE_SETTINGS;
    settingsMenu = SettingsMenu::MAIN;
    settingsIndex = 0;
    drawSettingsMenu();
    break;

  default:
    break;
  }
}

void handlePlaceholderMode(ButtonEvent event) {
  if (event == ButtonEvent::OK_DOUBLE) {
    // Return to main menu
    currentState = SystemState::MAIN_MENU;
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
  case SettingsMenu::SLEEP_TIMER:
    handleSleepTimer(event);
    break;
  case SettingsMenu::MOTOR_TEST:
    handleMotorTest(event);
    break;
  case SettingsMenu::CALIBRATION:
    handleCalibration(event);
    break;
  case SettingsMenu::ABOUT:
    handleAbout(event);
    break;
  case SettingsMenu::SAVE_EXIT:
    // Handled in MAIN
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
    // Back to main menu without saving
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
      // Play preview sound
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
    ledMode = (ledMode + 3) % 4; // -1 with wrap
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

void handleSleepTimer(ButtonEvent event) {
  switch (event) {
  case ButtonEvent::UP_PRESSED:
    sleepTimer = (sleepTimer + 1) % 4;
    drawSleepTimerScreen();
    break;

  case ButtonEvent::DOWN_PRESSED:
    sleepTimer = (sleepTimer + 3) % 4;
    drawSleepTimerScreen();
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

void handleMotorTest(ButtonEvent event) {
  switch (event) {
  case ButtonEvent::UP_PRESSED:
    stopAllMotors();
    motorRunning = false;
    motorTestIndex = (motorTestIndex - 1 + 5) % 5; // 5 options: 4 motors + All
    drawMotorTestScreen();
    break;

  case ButtonEvent::DOWN_PRESSED:
    stopAllMotors();
    motorRunning = false;
    motorTestIndex = (motorTestIndex + 1) % 5; // 5 options: 4 motors + All
    drawMotorTestScreen();
    break;

  case ButtonEvent::OK_SINGLE:
    // Toggle motor(s)
    if (motorRunning) {
      stopAllMotors();
      motorRunning = false;
    } else {
      if (motorTestIndex == 4) {
        // Run all motors at their calibrated speeds
        runMotor(0, motorCalibration[0]);
        runMotor(1, motorCalibration[1]);
        runMotor(2, motorCalibration[2]);
        runMotor(3, motorCalibration[3]);
      } else {
        runMotor(motorTestIndex, motorCalibration[motorTestIndex]);
      }
      motorRunning = true;
    }
    drawMotorTestScreen();
    break;

  case ButtonEvent::OK_DOUBLE:
    stopAllMotors();
    motorRunning = false;
    settingsMenu = SettingsMenu::MAIN;
    drawSettingsMenu();
    break;

  default:
    break;
  }
}

void handleCalibration(ButtonEvent event) {
  switch (event) {
  case ButtonEvent::UP_PRESSED:
    motorCalibration[motorTestIndex] =
        min(255, motorCalibration[motorTestIndex] + 10);
    drawCalibrationScreen();
    break;

  case ButtonEvent::DOWN_PRESSED:
    motorCalibration[motorTestIndex] =
        max(10, motorCalibration[motorTestIndex] - 10);
    drawCalibrationScreen();
    break;

  case ButtonEvent::OK_SINGLE:
    // Next motor
    motorTestIndex = (motorTestIndex + 1) % 4;
    drawCalibrationScreen();
    break;

  case ButtonEvent::OK_DOUBLE:
    settingsMenu = SettingsMenu::MAIN;
    motorTestIndex = 0;
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
    drawModeScreen("MECANUM MODE", "4-wheel omnidirectional");
    break;
  case 1:
    currentState = SystemState::MODE_RC;
    drawModeScreen("RC MODE", "Remote Control");
    break;
  case 2:
    currentState = SystemState::MODE_VOICE;
    drawModeScreen("VOICE MODE", "Voice Commands");
    break;
  case 3:
    currentState = SystemState::MODE_SETTINGS;
    settingsMenu = SettingsMenu::MAIN;
    settingsIndex = 0;
    drawSettingsMenu();
    break;
  }
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
    settingsMenu = SettingsMenu::SLEEP_TIMER;
    drawSleepTimerScreen();
    break;
  case 4:
    settingsMenu = SettingsMenu::MOTOR_TEST;
    motorTestIndex = 0;
    motorRunning = false;
    drawMotorTestScreen();
    break;
  case 5:
    settingsMenu = SettingsMenu::CALIBRATION;
    motorTestIndex = 0;
    drawCalibrationScreen();
    break;
  case 6:
    settingsMenu = SettingsMenu::ABOUT;
    drawAboutScreen();
    break;
  case 7:
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

// Apply brightness to OLED using contrast command
void applyBrightness() {
  // SSD1306 contrast command: 0x81 followed by contrast value (0-255)
  display.ssd1306_command(0x81);       // SETCONTRAST command
  display.ssd1306_command(brightness); // Contrast value 0-255
}

void showBootScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20, 10);
  display.println("MINI OS");
  display.setTextSize(1);
  display.setCursor(25, 35);
  display.println("ESP32-S3 Master");
  display.setCursor(40, 50);
  display.println("v1.0.0");
  display.display();
}

void drawMainMenu() {
  display.clearDisplay();

  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SELECT MODE");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Show 3 items at a time with scroll
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

  // Footer hint
  display.setCursor(0, 56);
  display.print("OK:Sel  2xOK:Back");

  display.display();
}

void drawModeScreen(const char *title, const char *subtitle) {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(title);
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 25);
  display.println(subtitle);

  display.setCursor(0, 45);
  display.println("[PLACEHOLDER]");
  display.setCursor(0, 56);
  display.println("2x OK to go back");

  display.display();
}

void drawSettingsMenu() {
  display.clearDisplay();

  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SETTINGS");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Show 3 items at a time with scroll
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

  // Footer
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

  // Progress bar
  int barWidth = map(brightness, 0, 255, 0, 100);
  display.drawRect(14, 25, 100, 15, SSD1306_WHITE);
  display.fillRect(14, 25, barWidth, 15, SSD1306_WHITE);

  // Percentage
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

  // Progress bar
  int barWidth = volume;
  display.drawRect(14, 25, 100, 15, SSD1306_WHITE);
  display.fillRect(14, 25, barWidth, 15, SSD1306_WHITE);

  // Percentage
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

void drawSleepTimerScreen() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SLEEP TIMER");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(25, 25);
  display.println(sleepTimerNames[sleepTimer]);

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.println("UP/DN:Time OK:Done");

  display.display();
}

// Apply LED mode
void applyLedMode() {
  switch (ledMode) {
  case 0: // Off
    digitalWrite(PIN_LED_STATUS, LOW);
    break;
  case 1: // On
    digitalWrite(PIN_LED_STATUS, HIGH);
    break;
  case 2: // Blink - will be handled in loop
  case 3: // Breath - will be handled in loop
    digitalWrite(PIN_LED_STATUS, HIGH);
    break;
  }
}

void drawMotorTestScreen() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("MOTOR TEST");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 15);
  display.printf("Motor: %s", motorNames[motorTestIndex]);

  // Show speed info
  display.setCursor(0, 27);
  if (motorTestIndex == 4) {
    // All motors - show all speeds
    display.printf("FL:%d FR:%d", motorCalibration[0], motorCalibration[1]);
    display.setCursor(0, 37);
    display.printf("BL:%d BR:%d", motorCalibration[2], motorCalibration[3]);
  } else {
    display.printf("Speed: %d", motorCalibration[motorTestIndex]);
    display.setCursor(0, 37);
  }

  display.setCursor(0, 47);
  display.printf("Status: %s", motorRunning ? "RUNNING" : "STOPPED");

  display.setCursor(0, 57);
  display.println("OK:Run 2xOK:Back");

  display.display();
}

void drawCalibrationScreen() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("MOTOR SPEED");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Show 3 motors at a time with scroll
  int startIdx = (motorTestIndex > 1) ? motorTestIndex - 1 : 0;
  int endIdx = (startIdx + 3 < 4) ? startIdx + 3 : 4;

  int row = 0;
  for (int i = startIdx; i < endIdx; i++) {
    int y = 16 + row * 12;

    // Set cursor for arrow indicator
    display.setCursor(0, y);
    if (i == motorTestIndex) {
      display.print(">");
    }

    // Set cursor for motor label and value
    display.setCursor(10, y);
    display.printf("%s: %d",
                   i == 0   ? "FL"
                   : i == 1 ? "FR"
                   : i == 2 ? "BL"
                            : "BR",
                   motorCalibration[i]);
    row++;
  }

  display.setCursor(0, 56);
  display.println("UP/DN:Spd OK:Next");

  display.display();
}

void drawAboutScreen() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ABOUT");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 18);
  display.println("ESP32-S3 Mini OS");
  display.println("Version: 1.0.0");
  display.println("Build: Jan 2026");
  display.println("Built by :");
  display.println("mister rehan");
  display.println("ewbid sigma");

  // display.setCursor(0, 56);
  // display.println("OK to go back");

  display.display();
}

// ============================================================
// MOTOR FUNCTIONS
// ============================================================
void initMotors() {
  // FL
  pinMode(PIN_FL_IN1, OUTPUT);
  pinMode(PIN_FL_IN2, OUTPUT);
  ledcSetup(1, 20000, 8);
  ledcAttachPin(PIN_FL_ENA, 1);

  // FR
  pinMode(PIN_FR_IN1, OUTPUT);
  pinMode(PIN_FR_IN2, OUTPUT);
  ledcSetup(2, 20000, 8);
  ledcAttachPin(PIN_FR_ENA, 2);

  // BL
  pinMode(PIN_BL_IN1, OUTPUT);
  pinMode(PIN_BL_IN2, OUTPUT);
  ledcSetup(3, 20000, 8);
  ledcAttachPin(PIN_BL_ENA, 3);

  // BR
  pinMode(PIN_BR_IN1, OUTPUT);
  pinMode(PIN_BR_IN2, OUTPUT);
  ledcSetup(4, 20000, 8);
  ledcAttachPin(PIN_BR_ENA, 4);

  stopAllMotors();
}

void runMotor(int index, int speed) {
  int in1, in2, channel;

  switch (index) {
  case 0:
    in1 = PIN_FL_IN1;
    in2 = PIN_FL_IN2;
    channel = 1;
    break;
  case 1:
    in1 = PIN_FR_IN1;
    in2 = PIN_FR_IN2;
    channel = 2;
    break;
  case 2:
    in1 = PIN_BL_IN1;
    in2 = PIN_BL_IN2;
    channel = 3;
    break;
  case 3:
    in1 = PIN_BR_IN1;
    in2 = PIN_BR_IN2;
    channel = 4;
    break;
  default:
    return;
  }

  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  ledcWrite(channel, speed);
}

void stopAllMotors() {
  digitalWrite(PIN_FL_IN1, LOW);
  digitalWrite(PIN_FL_IN2, LOW);
  ledcWrite(1, 0);
  digitalWrite(PIN_FR_IN1, LOW);
  digitalWrite(PIN_FR_IN2, LOW);
  ledcWrite(2, 0);
  digitalWrite(PIN_BL_IN1, LOW);
  digitalWrite(PIN_BL_IN2, LOW);
  ledcWrite(3, 0);
  digitalWrite(PIN_BR_IN1, LOW);
  digitalWrite(PIN_BR_IN2, LOW);
  ledcWrite(4, 0);
}

// ============================================================
// SOUND FUNCTIONS
// ============================================================
void playTone(int freq, int duration) {
  if (volume == 0)
    return;
  // Volume controls duty cycle (not frequency-based volume, but simple on/off)
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
#define EEPROM_ADDR_VOLUME 2
#define EEPROM_ADDR_LED_MODE 7
#define EEPROM_ADDR_SLEEP_TIMER 8

void loadSettings() {
  if (EEPROM.read(EEPROM_ADDR_MAGIC) == EEPROM_MAGIC) {
    brightness = EEPROM.read(EEPROM_ADDR_BRIGHTNESS);
    volume = EEPROM.read(EEPROM_ADDR_VOLUME);
    for (int i = 0; i < 4; i++) {
      uint8_t val = EEPROM.read(EEPROM_ADDR_MOTOR_CAL + i);
      // Validate speed value (10-255), default to 255 if invalid
      motorCalibration[i] = (val >= 10 && val <= 255) ? val : 255;
    }
    ledMode = EEPROM.read(EEPROM_ADDR_LED_MODE);
    sleepTimer = EEPROM.read(EEPROM_ADDR_SLEEP_TIMER);
    Serial.println("[EEPROM] Settings loaded");
  } else {
    Serial.println("[EEPROM] No saved settings, using defaults");
  }
}

void saveSettings() {
  EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);
  EEPROM.write(EEPROM_ADDR_BRIGHTNESS, brightness);
  EEPROM.write(EEPROM_ADDR_VOLUME, volume);
  for (int i = 0; i < 4; i++) {
    EEPROM.write(EEPROM_ADDR_MOTOR_CAL + i, (uint8_t)motorCalibration[i]);
  }
  EEPROM.write(EEPROM_ADDR_LED_MODE, ledMode);
  EEPROM.write(EEPROM_ADDR_SLEEP_TIMER, sleepTimer);
  EEPROM.commit();
  Serial.println("[EEPROM] Settings saved");
}

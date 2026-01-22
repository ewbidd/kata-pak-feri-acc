#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* ===== OLED CONFIG ===== */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

/* ===== I2C PIN ===== */
#define SDA_PIN 8
#define SCL_PIN 9

/* ===== BUTTON PIN ===== */
#define BTN_UP     4    // kiri atas
#define BTN_DOWN   5    // kiri bawah
#define BTN_OK     6    // kanan

/* ===== BUZZER ===== */
#define BUZZER_PIN 7    // ACTIVE BUZZER

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/* ===== UI STATE ===== */
enum UIState {
  STATE_MENU,
  STATE_ACTIVE
};

UIState uiState = STATE_MENU;

/* ===== BUTTON STATE ===== */
bool lastUp   = HIGH;
bool lastDown = HIGH;
bool lastOk   = HIGH;

/* ===== MENU ===== */
int menuIndex = 0;
int currentMode = -1;

const int menuCount = 3;
const char* menuList[menuCount] = {
  "MODE RC",
  "MODE GYRO",
  "MODE VOICE"
};

/* ===== DOUBLE CLICK ===== */
unsigned long lastOkTime = 0;
const unsigned long doubleClickDelay = 400;

/* ===== BUZZER ===== */
void beepClick() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(30);
  digitalWrite(BUZZER_PIN, LOW);
}

void beepConfirm() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(80);
  digitalWrite(BUZZER_PIN, LOW);
  delay(40);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(80);
  digitalWrite(BUZZER_PIN, LOW);
}

void setup() {
  Serial.begin(115200);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED gagal terdeteksi");
    while (true);
  }

  display.clearDisplay();
  display.display();
}

void loop() {
  bool up   = digitalRead(BTN_UP);
  bool down = digitalRead(BTN_DOWN);
  bool ok   = digitalRead(BTN_OK);

  /* ===== NAVIGATION (MENU ONLY) ===== */
  if (uiState == STATE_MENU) {

    if (lastUp == HIGH && up == LOW) {
      menuIndex--;
      if (menuIndex < 0) menuIndex = menuCount - 1;
      beepClick();
    }

    if (lastDown == HIGH && down == LOW) {
      menuIndex++;
      if (menuIndex >= menuCount) menuIndex = 0;
      beepClick();
    }
  }

  /* ===== OK BUTTON ===== */
  if (lastOk == HIGH && ok == LOW) {
    unsigned long now = millis();

    if (uiState == STATE_MENU) {
      // pilih mode
      currentMode = menuIndex;
      uiState = STATE_ACTIVE;

      Serial.print("Mode aktif: ");
      Serial.println(menuList[currentMode]);

      beepConfirm();
    }
    else if (uiState == STATE_ACTIVE) {
      // double click untuk kembali
      if (now - lastOkTime < doubleClickDelay) {
        uiState = STATE_MENU;
        currentMode = -1;

        Serial.println("Kembali ke menu");
        beepConfirm();
      }
      lastOkTime = now;
    }
  }

  lastUp   = up;
  lastDown = down;
  lastOk   = ok;

  /* ===== OLED DISPLAY ===== */
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (uiState == STATE_MENU) {

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("ESP32-S3 MENU");

    for (int i = 0; i < menuCount; i++) {
      display.setCursor(0, 16 + i * 12);
      display.print(i == menuIndex ? "> " : "  ");
      display.println(menuList[i]);
    }

    display.setCursor(0, 56);
    display.print("OK = Select");

  } else if (uiState == STATE_ACTIVE) {

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Mode saat ini :");

    display.setTextSize(2);
    display.setCursor(0, 16);
    display.println(menuList[currentMode]);

    display.setTextSize(1);
    display.setCursor(0, 48);
    display.println("Klik 2x OK");
    display.setCursor(0, 56);
    display.println("untuk kembali");
  }

  display.display();
  delay(50);
}

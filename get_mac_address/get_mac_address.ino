/*
 * ===================================================
 * GET MAC ADDRESS - ESP32/ESP32-S3
 * ===================================================
 *
 * Upload kode ini ke ESP32 RECEIVER untuk mendapatkan
 * MAC Address-nya. Lihat di Serial Monitor.
 *
 * Setelah dapat MAC Address, copy dan paste ke file
 * web_transmitter.ino pada baris receiverMAC[]
 *
 * ===================================================
 */

#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Set WiFi mode
  WiFi.mode(WIFI_STA);

  Serial.println();
  Serial.println("================================================");
  Serial.println("         MAC ADDRESS DETECTOR");
  Serial.println("================================================");
  Serial.println();

  // Get MAC Address
  String mac = WiFi.macAddress();

  Serial.println("MAC Address ESP32 ini:");
  Serial.println();
  Serial.print("  String format : ");
  Serial.println(mac);
  Serial.println();

  // Parse MAC to bytes
  uint8_t macBytes[6];
  sscanf(mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &macBytes[0],
         &macBytes[1], &macBytes[2], &macBytes[3], &macBytes[4], &macBytes[5]);

  // Print in code format
  Serial.println("  Code format   : ");
  Serial.print("    uint8_t receiverMAC[] = {");
  for (int i = 0; i < 6; i++) {
    Serial.print("0x");
    if (macBytes[i] < 0x10)
      Serial.print("0");
    Serial.print(macBytes[i], HEX);
    if (i < 5)
      Serial.print(", ");
  }
  Serial.println("};");

  Serial.println();
  Serial.println("================================================");
  Serial.println("Copy baris 'uint8_t receiverMAC[]' di atas");
  Serial.println("ke file web_transmitter.ino");
  Serial.println("================================================");

  // Blink LED to indicate done
  pinMode(48, OUTPUT);
}

void loop() {
  // Blink LED
  digitalWrite(48, HIGH);
  delay(500);
  digitalWrite(48, LOW);
  delay(500);
}

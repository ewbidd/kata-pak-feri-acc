/*
 * ===================================================
 * ESP-NOW WEB TRANSMITTER - ESP32-S3
 * Web-Based Virtual Joystick Controller
 * ===================================================
 *
 * Controller berbasis web dengan virtual joystick
 * untuk mengontrol robot via ESP-NOW
 *
 * CARA PAKAI:
 * 1. Upload kode ini ke ESP32-S3
 * 2. Hubungkan HP/Laptop ke WiFi "RC_Controller"
 *    Password: "12345678"
 * 3. Buka browser ke http://192.168.4.1
 * 4. Gunakan virtual joystick untuk mengontrol
 *
 * ===================================================
 *
 * SERIAL MONITOR:
 * - Di Arduino IDE, pilih Tools -> USB CDC On Boot -> "Enabled"
 * - Upload kode, lalu buka Serial Monitor
 * - Atau gunakan USB-to-TTL di pin TX(43)/RX(44)
 *
 * ===================================================
 */

#include <WebServer.h>
#include <WiFi.h>
#include <esp_now.h>

// Gunakan Serial0 = Hardware UART (COM port biasa)
// Serial0 menggunakan TX=GPIO43, RX=GPIO44
#define SerialMon Serial0

// ===== KONFIGURASI WiFi ACCESS POINT =====
const char *ssid = "RC_Controller";
const char *password = "12345678";

// ===== MAC ADDRESS RECEIVER =====
uint8_t receiverMAC[] = {0xFC, 0x01, 0x2C, 0xD1, 0x76, 0x54};

// ===== RGB LED STATUS (NeoPixel di GPIO48) =====
#define RGB_LED_PIN 48

// Fungsi untuk set warna RGB LED
void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  neopixelWrite(RGB_LED_PIN, r, g, b);
}

// ===== STRUKTUR DATA YANG DIKIRIM =====
typedef struct {
  int16_t throttle; // -255 to 255 (maju/mundur)
  int16_t steering; // -255 to 255 (kiri/kanan)
  int16_t aux_x;    // -255 to 255 (auxiliary X)
  int16_t aux_y;    // -255 to 255 (auxiliary Y)
  bool btn1;        // Tombol 1
  bool btn2;        // Tombol 2
  uint8_t mode;     // Mode operasi (0=manual, 1=semi-auto, 2=auto)
} RemoteData;

RemoteData dataToSend;

// ===== WEB SERVER =====
WebServer server(80);

// ===== VARIABEL =====
esp_now_peer_info_t peerInfo;
bool peerConnected = false;
unsigned long lastSendTime = 0;
int sendFailCount = 0;
bool espNowReady = false;

// ===== HALAMAN HTML =====
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no, maximum-scale=1.0">
    <title>RC Controller</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            touch-action: none;
            -webkit-touch-callout: none;
            -webkit-user-select: none;
            user-select: none;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 50%, #0f3460 100%);
            min-height: 100vh;
            color: #fff;
            overflow: hidden;
        }
        
        .header {
            text-align: center;
            padding: 15px;
            background: rgba(0, 0, 0, 0.3);
            border-bottom: 2px solid #e94560;
        }
        
        .header h1 {
            font-size: 1.5em;
            color: #e94560;
            text-shadow: 0 0 10px rgba(233, 69, 96, 0.5);
        }
        
        .status-bar {
            display: flex;
            justify-content: space-around;
            padding: 10px;
            background: rgba(0, 0, 0, 0.2);
        }
        
        .status-item {
            text-align: center;
            padding: 5px 15px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 20px;
            font-size: 0.9em;
        }
        
        .status-item.connected {
            background: rgba(0, 255, 136, 0.2);
            color: #00ff88;
        }
        
        .status-item.disconnected {
            background: rgba(255, 68, 68, 0.2);
            color: #ff4444;
        }
        
        .controller-area {
            display: flex;
            justify-content: space-between;
            align-items: center;
            height: calc(100vh - 180px);
            padding: 20px;
        }
        
        .joystick-container {
            width: 45%;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        
        .joystick-label {
            font-size: 1.1em;
            margin-bottom: 10px;
            color: #94a3b8;
        }
        
        .joystick-base {
            width: 180px;
            height: 180px;
            background: radial-gradient(circle, rgba(255,255,255,0.1) 0%, rgba(255,255,255,0.05) 100%);
            border: 3px solid rgba(233, 69, 96, 0.5);
            border-radius: 50%;
            position: relative;
            box-shadow: 0 0 30px rgba(233, 69, 96, 0.2), inset 0 0 30px rgba(0, 0, 0, 0.3);
        }
        
        .joystick-stick {
            width: 70px;
            height: 70px;
            background: radial-gradient(circle at 30% 30%, #e94560 0%, #c73659 100%);
            border-radius: 50%;
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.5), 0 0 20px rgba(233, 69, 96, 0.3);
            cursor: pointer;
            transition: box-shadow 0.1s;
        }
        
        .joystick-stick:active {
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.5), 0 0 30px rgba(233, 69, 96, 0.5);
        }
        
        .joystick-values {
            margin-top: 15px;
            font-size: 0.9em;
            text-align: center;
            color: #64748b;
        }
        
        .center-panel {
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 15px;
        }
        
        .mode-btn {
            padding: 15px 25px;
            font-size: 1em;
            font-weight: bold;
            border: none;
            border-radius: 25px;
            cursor: pointer;
            transition: all 0.3s;
            text-transform: uppercase;
        }
        
        .mode-btn.manual {
            background: linear-gradient(135deg, #00b894, #00a878);
            color: white;
        }
        
        .mode-btn.semi-auto {
            background: linear-gradient(135deg, #fdcb6e, #f39c12);
            color: #1a1a2e;
        }
        
        .mode-btn.auto {
            background: linear-gradient(135deg, #e94560, #c73659);
            color: white;
        }
        
        .action-btn {
            width: 80px;
            height: 80px;
            border-radius: 50%;
            border: 3px solid;
            font-size: 1.5em;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.2s;
        }
        
        .btn1 {
            background: linear-gradient(135deg, #74b9ff, #0984e3);
            border-color: #74b9ff;
            color: white;
        }
        
        .btn2 {
            background: linear-gradient(135deg, #fd79a8, #e84393);
            border-color: #fd79a8;
            color: white;
        }
        
        .action-btn:active {
            transform: scale(0.9);
        }
        
        .action-btn.pressed {
            box-shadow: 0 0 20px currentColor;
        }
        
        .data-display {
            position: fixed;
            bottom: 0;
            left: 0;
            right: 0;
            background: rgba(0, 0, 0, 0.5);
            padding: 10px;
            display: flex;
            justify-content: space-around;
            font-family: monospace;
            font-size: 0.85em;
        }
        
        .data-item {
            text-align: center;
        }
        
        .data-value {
            color: #e94560;
            font-weight: bold;
        }
        
        @media (max-height: 500px) {
            .joystick-base {
                width: 140px;
                height: 140px;
            }
            .joystick-stick {
                width: 55px;
                height: 55px;
            }
            .header {
                padding: 8px;
            }
            .header h1 {
                font-size: 1.2em;
            }
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>🎮 RC Controller</h1>
    </div>
    
    <div class="status-bar">
        <div class="status-item" id="connection-status">Connecting...</div>
        <div class="status-item" id="mode-status">Mode: Manual</div>
    </div>
    
    <div class="controller-area">
        <div class="joystick-container">
            <div class="joystick-label">THROTTLE / STEERING</div>
            <div class="joystick-base" id="joystick1">
                <div class="joystick-stick" id="stick1"></div>
            </div>
            <div class="joystick-values" id="joy1-values">T: 0 | S: 0</div>
        </div>
        
        <div class="center-panel">
            <button class="mode-btn manual" id="mode-btn" onclick="toggleMode()">MANUAL</button>
            <button class="action-btn btn1" id="btn1">1</button>
            <button class="action-btn btn2" id="btn2">2</button>
        </div>
        
        <div class="joystick-container">
            <div class="joystick-label">AUXILIARY</div>
            <div class="joystick-base" id="joystick2">
                <div class="joystick-stick" id="stick2"></div>
            </div>
            <div class="joystick-values" id="joy2-values">X: 0 | Y: 0</div>
        </div>
    </div>
    
    <div class="data-display">
        <div class="data-item">THR: <span class="data-value" id="d-thr">0</span></div>
        <div class="data-item">STR: <span class="data-value" id="d-str">0</span></div>
        <div class="data-item">AX: <span class="data-value" id="d-ax">0</span></div>
        <div class="data-item">AY: <span class="data-value" id="d-ay">0</span></div>
    </div>

    <script>
        // State
        let state = {
            throttle: 0,
            steering: 0,
            aux_x: 0,
            aux_y: 0,
            btn1: false,
            btn2: false,
            mode: 0
        };
        
        let connected = false;
        let sendInterval;
        
        // Joystick class
        class Joystick {
            constructor(baseId, stickId, onMove) {
                this.base = document.getElementById(baseId);
                this.stick = document.getElementById(stickId);
                this.onMove = onMove;
                this.active = false;
                this.identifier = null;
                
                this.baseRect = null;
                this.maxDist = 0;
                
                this.init();
            }
            
            init() {
                this.base.addEventListener('touchstart', (e) => this.start(e), {passive: false});
                this.base.addEventListener('touchmove', (e) => this.move(e), {passive: false});
                this.base.addEventListener('touchend', (e) => this.end(e), {passive: false});
                this.base.addEventListener('touchcancel', (e) => this.end(e), {passive: false});
                
                this.base.addEventListener('mousedown', (e) => this.mouseStart(e));
                document.addEventListener('mousemove', (e) => this.mouseMove(e));
                document.addEventListener('mouseup', (e) => this.mouseEnd(e));
            }
            
            start(e) {
                e.preventDefault();
                if (this.active) return;
                
                const touch = e.changedTouches[0];
                this.identifier = touch.identifier;
                this.active = true;
                this.updateRect();
                this.processPosition(touch.clientX, touch.clientY);
            }
            
            move(e) {
                e.preventDefault();
                if (!this.active) return;
                
                for (let touch of e.changedTouches) {
                    if (touch.identifier === this.identifier) {
                        this.processPosition(touch.clientX, touch.clientY);
                        break;
                    }
                }
            }
            
            end(e) {
                e.preventDefault();
                for (let touch of e.changedTouches) {
                    if (touch.identifier === this.identifier) {
                        this.reset();
                        break;
                    }
                }
            }
            
            mouseStart(e) {
                e.preventDefault();
                this.active = true;
                this.updateRect();
                this.processPosition(e.clientX, e.clientY);
            }
            
            mouseMove(e) {
                if (!this.active) return;
                this.processPosition(e.clientX, e.clientY);
            }
            
            mouseEnd(e) {
                if (this.active) {
                    this.reset();
                }
            }
            
            updateRect() {
                this.baseRect = this.base.getBoundingClientRect();
                this.maxDist = (this.baseRect.width - parseInt(getComputedStyle(this.stick).width)) / 2;
            }
            
            processPosition(clientX, clientY) {
                const centerX = this.baseRect.left + this.baseRect.width / 2;
                const centerY = this.baseRect.top + this.baseRect.height / 2;
                
                let dx = clientX - centerX;
                let dy = clientY - centerY;
                
                const dist = Math.sqrt(dx * dx + dy * dy);
                if (dist > this.maxDist) {
                    dx = dx * this.maxDist / dist;
                    dy = dy * this.maxDist / dist;
                }
                
                this.stick.style.left = `calc(50% + ${dx}px)`;
                this.stick.style.top = `calc(50% + ${dy}px)`;
                
                // Normalize ke -255 to 255
                const normX = Math.round((dx / this.maxDist) * 255);
                const normY = Math.round((-dy / this.maxDist) * 255); // Invert Y
                
                this.onMove(normX, normY);
            }
            
            reset() {
                this.active = false;
                this.identifier = null;
                this.stick.style.left = '50%';
                this.stick.style.top = '50%';
                this.onMove(0, 0);
            }
        }
        
        // Initialize joysticks
        const joy1 = new Joystick('joystick1', 'stick1', (x, y) => {
            state.steering = x;
            state.throttle = y;
            document.getElementById('joy1-values').textContent = `T: ${y} | S: ${x}`;
            document.getElementById('d-thr').textContent = y;
            document.getElementById('d-str').textContent = x;
        });
        
        const joy2 = new Joystick('joystick2', 'stick2', (x, y) => {
            state.aux_x = x;
            state.aux_y = y;
            document.getElementById('joy2-values').textContent = `X: ${x} | Y: ${y}`;
            document.getElementById('d-ax').textContent = x;
            document.getElementById('d-ay').textContent = y;
        });
        
        // Button handlers
        const btn1Elem = document.getElementById('btn1');
        const btn2Elem = document.getElementById('btn2');
        
        ['touchstart', 'mousedown'].forEach(evt => {
            btn1Elem.addEventListener(evt, (e) => {
                e.preventDefault();
                state.btn1 = true;
                btn1Elem.classList.add('pressed');
            });
            btn2Elem.addEventListener(evt, (e) => {
                e.preventDefault();
                state.btn2 = true;
                btn2Elem.classList.add('pressed');
            });
        });
        
        ['touchend', 'touchcancel', 'mouseup', 'mouseleave'].forEach(evt => {
            btn1Elem.addEventListener(evt, (e) => {
                state.btn1 = false;
                btn1Elem.classList.remove('pressed');
            });
            btn2Elem.addEventListener(evt, (e) => {
                state.btn2 = false;
                btn2Elem.classList.remove('pressed');
            });
        });
        
        // Mode toggle
        const modeNames = ['MANUAL', 'SEMI-AUTO', 'AUTO'];
        const modeClasses = ['manual', 'semi-auto', 'auto'];
        
        function toggleMode() {
            state.mode = (state.mode + 1) % 3;
            updateModeDisplay();
        }
        
        function updateModeDisplay() {
            const btn = document.getElementById('mode-btn');
            btn.textContent = modeNames[state.mode];
            btn.className = 'mode-btn ' + modeClasses[state.mode];
            document.getElementById('mode-status').textContent = 'Mode: ' + modeNames[state.mode];
        }
        
        // Send data to ESP
        function sendData() {
            const params = `t=${state.throttle}&s=${state.steering}&ax=${state.aux_x}&ay=${state.aux_y}&b1=${state.btn1?1:0}&b2=${state.btn2?1:0}&m=${state.mode}`;
            
            fetch('/control?' + params)
                .then(response => response.text())
                .then(data => {
                    if (data === 'OK') {
                        if (!connected) {
                            connected = true;
                            updateConnectionStatus();
                        }
                    }
                })
                .catch(err => {
                    if (connected) {
                        connected = false;
                        updateConnectionStatus();
                    }
                });
        }
        
        function updateConnectionStatus() {
            const elem = document.getElementById('connection-status');
            if (connected) {
                elem.textContent = '🟢 Connected';
                elem.className = 'status-item connected';
            } else {
                elem.textContent = '🔴 Disconnected';
                elem.className = 'status-item disconnected';
            }
        }
        
        // Start sending data
        sendInterval = setInterval(sendData, 50); // 20Hz
        
        // Initial status
        updateModeDisplay();
        updateConnectionStatus();
        
        // Prevent context menu
        document.addEventListener('contextmenu', e => e.preventDefault());
    </script>
</body>
</html>
)rawliteral";

// ===== CALLBACK SAAT DATA TERKIRIM =====
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    setRGB(0, 50, 0); // HIJAU = terkirim OK ke receiver
    sendFailCount = 0;
  } else {
    sendFailCount++;
    if (sendFailCount > 20) {
      setRGB(50, 0, 0); // MERAH = receiver tidak ada (banyak kali gagal)
    } else {
      setRGB(0, 0, 50); // BIRU = mencoba koneksi ke receiver
    }
  }
}

// ===== HANDLER UNTUK HALAMAN UTAMA =====
void handleRoot() { server.send_P(200, "text/html", HTML_PAGE); }

// ===== HANDLER UNTUK KONTROL =====
void handleControl() {
  // Parse parameter dari request
  if (server.hasArg("t")) {
    dataToSend.throttle = server.arg("t").toInt();
  }
  if (server.hasArg("s")) {
    dataToSend.steering = server.arg("s").toInt();
  }
  if (server.hasArg("ax")) {
    dataToSend.aux_x = server.arg("ax").toInt();
  }
  if (server.hasArg("ay")) {
    dataToSend.aux_y = server.arg("ay").toInt();
  }
  if (server.hasArg("b1")) {
    dataToSend.btn1 = server.arg("b1").toInt() == 1;
  }
  if (server.hasArg("b2")) {
    dataToSend.btn2 = server.arg("b2").toInt() == 1;
  }
  if (server.hasArg("m")) {
    dataToSend.mode = server.arg("m").toInt();
  }

  // Kirim data via ESP-NOW
  if (espNowReady) {
    esp_now_send(receiverMAC, (uint8_t *)&dataToSend, sizeof(dataToSend));
  }

  server.send(200, "text/plain", "OK");
}

// ===== HANDLER STATUS =====
void handleStatus() {
  String json = "{";
  json += "\"connected\":" + String(sendFailCount < 5 ? "true" : "false") + ",";
  json += "\"throttle\":" + String(dataToSend.throttle) + ",";
  json += "\"steering\":" + String(dataToSend.steering) + ",";
  json += "\"aux_x\":" + String(dataToSend.aux_x) + ",";
  json += "\"aux_y\":" + String(dataToSend.aux_y) + ",";
  json += "\"mode\":" + String(dataToSend.mode);
  json += "}";

  server.send(200, "application/json", json);
}

// ===== SETUP =====
void setup() {
  // RGB LED: MERAH = mulai booting
  setRGB(50, 0, 0);

  SerialMon.begin(115200);
  delay(2000); // Tunggu power stabil (penting untuk battery)

  SerialMon.println("=====================================");
  SerialMon.println("  ESP-NOW WEB TRANSMITTER");
  SerialMon.println("  Virtual Joystick Controller");
  SerialMon.println("=====================================");

  // Blink KUNING 2x = Serial OK
  for (int i = 0; i < 2; i++) {
    setRGB(50, 50, 0); // KUNING
    delay(100);
    setRGB(0, 0, 0); // OFF
    delay(100);
  }

  // Inisialisasi data
  dataToSend.throttle = 0;
  dataToSend.steering = 0;
  dataToSend.aux_x = 0;
  dataToSend.aux_y = 0;
  dataToSend.btn1 = false;
  dataToSend.btn2 = false;
  dataToSend.mode = 0;

  // Setup WiFi sebagai Access Point + Station
  SerialMon.println("Memulai WiFi AP...");
  WiFi.mode(WIFI_AP_STA);
  delay(500); // Tunggu WiFi mode switch

  // Disconnect dari WiFi client jika ada
  WiFi.disconnect();
  delay(100);

  // Konfigurasi Access Point dengan parameter lengkap
  // Channel 1, tidak hidden, max 4 connections
  bool apStarted = WiFi.softAP(ssid, password, 1, 0, 4);

  if (apStarted) {
    IPAddress IP = WiFi.softAPIP();
    SerialMon.println();
    SerialMon.println("**********************************");
    SerialMon.println("*** ACCESS POINT AKTIF! ***");
    SerialMon.println("**********************************");
    SerialMon.print("  SSID     : ");
    SerialMon.println(ssid);
    SerialMon.print("  Password : ");
    SerialMon.println(password);
    SerialMon.print("  IP       : ");
    SerialMon.println(IP);
    SerialMon.println();
    SerialMon.println("----------------------------------");
    SerialMon.println("MAC ADDRESS INFO:");
    SerialMon.print("  Transmitter (ini): ");
    SerialMon.println(WiFi.macAddress());
    SerialMon.print("  Receiver (target): ");
    // Print receiver MAC in readable format
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", receiverMAC[0],
            receiverMAC[1], receiverMAC[2], receiverMAC[3], receiverMAC[4],
            receiverMAC[5]);
    SerialMon.println(macStr);
    SerialMon.println("----------------------------------");
    SerialMon.println();

    // Inisialisasi ESP-NOW
    if (esp_now_init() != ESP_OK) {
      SerialMon.println("ERROR: Gagal inisialisasi ESP-NOW!");
    } else {
      SerialMon.println("ESP-NOW berhasil diinisialisasi");
      espNowReady = true;

      // Daftarkan callback
      esp_now_register_send_cb(OnDataSent);

      // Tambahkan peer (receiver)
      memcpy(peerInfo.peer_addr, receiverMAC, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;

      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        SerialMon.println("ERROR: Gagal menambahkan peer!");
      } else {
        SerialMon.println("Peer receiver berhasil ditambahkan");
        peerConnected = true;
      }
    }

    // Setup Web Server
    server.on("/", handleRoot);
    server.on("/control", handleControl);
    server.on("/status", handleStatus);

    server.begin();
    SerialMon.println("Web Server aktif!");
    SerialMon.println();
    SerialMon.println("=====================================");
    SerialMon.println("Buka browser dan akses:");
    SerialMon.print("  http://");
    SerialMon.println(IP);
    SerialMon.println("=====================================");

    // LED blink HIJAU 3x = WiFi AP OK
    for (int i = 0; i < 3; i++) {
      setRGB(0, 50, 0); // HIJAU
      delay(100);
      setRGB(0, 0, 0); // OFF
      delay(100);
    }
    // Tetap HIJAU redup = siap
    setRGB(0, 20, 0);
  } else {
    // GAGAL membuat AP - blink MERAH cepat terus menerus
    SerialMon.println("!!! ERROR: GAGAL MEMBUAT ACCESS POINT !!!");
    SerialMon.println("Cek power supply atau reset ESP32");
    while (1) {
      setRGB(50, 0, 0); // MERAH
      delay(50);
      setRGB(0, 0, 0); // OFF
      delay(50);
    }
  }
}

// ===== LOOP =====
void loop() {
  server.handleClient();

  // Debug output setiap 1 detik
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 1000) {
    SerialMon.print("T:");
    SerialMon.print(dataToSend.throttle);
    SerialMon.print(" S:");
    SerialMon.print(dataToSend.steering);
    SerialMon.print(" AX:");
    SerialMon.print(dataToSend.aux_x);
    SerialMon.print(" AY:");
    SerialMon.print(dataToSend.aux_y);
    SerialMon.print(" B1:");
    SerialMon.print(dataToSend.btn1);
    SerialMon.print(" B2:");
    SerialMon.print(dataToSend.btn2);
    SerialMon.print(" M:");
    SerialMon.println(dataToSend.mode);

    lastDebug = millis();
  }

  delay(1);
}

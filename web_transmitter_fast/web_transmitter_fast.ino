/*
 * ===================================================
 * ESP-NOW WEB TRANSMITTER - FAST VERSION
 * WebSocket untuk Low Latency Control
 * ===================================================
 *
 * Versi cepat dengan WebSocket (bukan HTTP polling)
 * Latency jauh lebih rendah!
 *
 * ===================================================
 */

#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include <esp_now.h>


#define SerialMon Serial0

// ===== KONFIGURASI =====
const char *ssid = "RC_Controller";
const char *password = "12345678";

// ===== MAC ADDRESS RECEIVER - GANTI DENGAN MAC RECEIVER KAMU! =====
uint8_t receiverMAC[] = {0xFC, 0x01, 0x2C, 0xD1, 0x76, 0x54};

// ===== RGB LED =====
#define RGB_LED_PIN 48
void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  neopixelWrite(RGB_LED_PIN, r, g, b);
}

// ===== STRUKTUR DATA =====
typedef struct {
  int16_t throttle;
  int16_t steering;
  int16_t aux_x;
  int16_t aux_y;
  bool btn1;
  bool btn2;
  uint8_t mode;
} RemoteData;

RemoteData dataToSend;

// ===== SERVER =====
WebServer server(80);
WebSocketsServer webSocket(81);

// ===== VARIABEL =====
esp_now_peer_info_t peerInfo;
bool espNowReady = false;
int sendFailCount = 0;
unsigned long lastSend = 0;

// ===== HTML PAGE =====
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no, maximum-scale=1.0">
    <title>RC Controller</title>
    <style>
        * { margin:0; padding:0; box-sizing:border-box; touch-action:none; user-select:none; }
        body { font-family:Arial; background:linear-gradient(135deg,#1a1a2e,#16213e); min-height:100vh; color:#fff; overflow:hidden; }
        .header { text-align:center; padding:10px; background:rgba(0,0,0,0.3); }
        .header h1 { font-size:1.3em; color:#e94560; }
        .status { display:flex; justify-content:center; gap:20px; padding:8px; font-size:0.9em; }
        .status span { padding:5px 15px; background:rgba(255,255,255,0.1); border-radius:15px; }
        .status .on { background:rgba(0,255,136,0.3); color:#0f8; }
        .status .off { background:rgba(255,68,68,0.3); color:#f44; }
        .ctrl { display:flex; justify-content:space-between; align-items:center; height:calc(100vh - 140px); padding:15px; }
        .joy-wrap { width:42%; text-align:center; }
        .joy-label { font-size:0.9em; color:#94a3b8; margin-bottom:8px; }
        .joy-base { width:160px; height:160px; background:rgba(255,255,255,0.08); border:3px solid rgba(233,69,96,0.5); border-radius:50%; position:relative; margin:0 auto; }
        .joy-stick { width:60px; height:60px; background:radial-gradient(circle at 30% 30%,#e94560,#c73659); border-radius:50%; position:absolute; top:50%; left:50%; transform:translate(-50%,-50%); cursor:pointer; }
        .joy-val { margin-top:10px; font-size:0.8em; color:#64748b; }
        .center { display:flex; flex-direction:column; align-items:center; gap:12px; }
        .mode-btn { padding:12px 20px; font-weight:bold; border:none; border-radius:20px; cursor:pointer; }
        .mode-btn.m0 { background:linear-gradient(135deg,#00b894,#00a878); color:#fff; }
        .mode-btn.m1 { background:linear-gradient(135deg,#fdcb6e,#f39c12); color:#1a1a2e; }
        .mode-btn.m2 { background:linear-gradient(135deg,#e94560,#c73659); color:#fff; }
        .btn { width:60px; height:60px; border-radius:50%; border:2px solid; font-size:1.2em; font-weight:bold; cursor:pointer; }
        .btn1 { background:linear-gradient(135deg,#74b9ff,#0984e3); border-color:#74b9ff; color:#fff; }
        .btn2 { background:linear-gradient(135deg,#fd79a8,#e84393); border-color:#fd79a8; color:#fff; }
        .btn.pressed { transform:scale(0.9); box-shadow:0 0 15px currentColor; }
        .data { position:fixed; bottom:0; left:0; right:0; background:rgba(0,0,0,0.5); padding:8px; display:flex; justify-content:space-around; font-family:monospace; font-size:0.8em; }
        .data .v { color:#e94560; font-weight:bold; }
        @media(max-height:500px) { .joy-base{width:120px;height:120px;} .joy-stick{width:50px;height:50px;} }
    </style>
</head>
<body>
    <div class="header"><h1>🎮 RC Fast</h1></div>
    <div class="status">
        <span id="ws-status" class="off">WS: Connecting</span>
        <span id="mode-status">Mode: MANUAL</span>
    </div>
    <div class="ctrl">
        <div class="joy-wrap">
            <div class="joy-label">THROTTLE / STEERING</div>
            <div class="joy-base" id="j1"><div class="joy-stick" id="s1"></div></div>
            <div class="joy-val" id="v1">T:0 S:0</div>
        </div>
        <div class="center">
            <button class="mode-btn m0" id="mbtn" onclick="toggleMode()">MANUAL</button>
            <button class="btn btn1" id="b1">1</button>
            <button class="btn btn2" id="b2">2</button>
        </div>
        <div class="joy-wrap">
            <div class="joy-label">AUXILIARY</div>
            <div class="joy-base" id="j2"><div class="joy-stick" id="s2"></div></div>
            <div class="joy-val" id="v2">X:0 Y:0</div>
        </div>
    </div>
    <div class="data">
        <div>THR:<span class="v" id="dt">0</span></div>
        <div>STR:<span class="v" id="ds">0</span></div>
        <div>AX:<span class="v" id="dax">0</span></div>
        <div>AY:<span class="v" id="day">0</span></div>
        <div>MS:<span class="v" id="ms">0</span></div>
    </div>
<script>
let ws, state={t:0,s:0,ax:0,ay:0,b1:0,b2:0,m:0}, lastSend=0;
const modeNames=['MANUAL','SEMI-AUTO','AUTO'];

function connect() {
    ws = new WebSocket('ws://'+location.hostname+':81/');
    ws.onopen = () => { 
        document.getElementById('ws-status').textContent='WS: Connected';
        document.getElementById('ws-status').className='on';
    };
    ws.onclose = () => { 
        document.getElementById('ws-status').textContent='WS: Disconnected';
        document.getElementById('ws-status').className='off';
        setTimeout(connect, 1000); 
    };
    ws.onerror = () => ws.close();
}
connect();

function send() {
    if(ws && ws.readyState===1) {
        let now = performance.now();
        ws.send(`${state.t},${state.s},${state.ax},${state.ay},${state.b1},${state.b2},${state.m}`);
        document.getElementById('ms').textContent = Math.round(now - lastSend);
        lastSend = now;
    }
}

class Joy {
    constructor(bid, sid, cb) {
        this.base=document.getElementById(bid);
        this.stick=document.getElementById(sid);
        this.cb=cb; this.active=false; this.tid=null;
        this.base.ontouchstart=e=>{e.preventDefault();this.tid=e.changedTouches[0].identifier;this.active=true;this.upd();this.proc(e.changedTouches[0]);};
        this.base.ontouchmove=e=>{e.preventDefault();for(let t of e.changedTouches)if(t.identifier===this.tid)this.proc(t);};
        this.base.ontouchend=e=>{for(let t of e.changedTouches)if(t.identifier===this.tid)this.reset();};
        this.base.onmousedown=e=>{e.preventDefault();this.active=true;this.upd();this.proc(e);};
        document.onmousemove=e=>{if(this.active)this.proc(e);};
        document.onmouseup=e=>{if(this.active)this.reset();};
    }
    upd(){this.rect=this.base.getBoundingClientRect();this.max=(this.rect.width-60)/2;}
    proc(e){
        let x=(e.clientX||e.pageX)-this.rect.left-this.rect.width/2;
        let y=(e.clientY||e.pageY)-this.rect.top-this.rect.height/2;
        let d=Math.sqrt(x*x+y*y);
        if(d>this.max){x=x*this.max/d;y=y*this.max/d;}
        this.stick.style.left=`calc(50% + ${x}px)`;
        this.stick.style.top=`calc(50% + ${y}px)`;
        this.cb(Math.round(x/this.max*255),Math.round(-y/this.max*255));
        send();
    }
    reset(){this.active=false;this.stick.style.left='50%';this.stick.style.top='50%';this.cb(0,0);send();}
}

new Joy('j1','s1',(x,y)=>{state.s=x;state.t=y;document.getElementById('v1').textContent=`T:${y} S:${x}`;document.getElementById('dt').textContent=y;document.getElementById('ds').textContent=x;});
new Joy('j2','s2',(x,y)=>{state.ax=x;state.ay=y;document.getElementById('v2').textContent=`X:${x} Y:${y}`;document.getElementById('dax').textContent=x;document.getElementById('day').textContent=y;});

['b1','b2'].forEach(id=>{
    let el=document.getElementById(id);
    let down=()=>{state[id]=1;el.classList.add('pressed');send();};
    let up=()=>{state[id]=0;el.classList.remove('pressed');send();};
    el.ontouchstart=e=>{e.preventDefault();down();};
    el.ontouchend=up;
    el.onmousedown=down;
    el.onmouseup=up;
    el.onmouseleave=up;
});

function toggleMode(){
    state.m=(state.m+1)%3;
    let b=document.getElementById('mbtn');
    b.textContent=modeNames[state.m];
    b.className='mode-btn m'+state.m;
    document.getElementById('mode-status').textContent='Mode: '+modeNames[state.m];
    send();
}
document.addEventListener('contextmenu',e=>e.preventDefault());
</script>
</body>
</html>
)rawliteral";

// ===== ESP-NOW CALLBACK =====
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    setRGB(0, 50, 0);
    sendFailCount = 0;
  } else {
    sendFailCount++;
    setRGB(sendFailCount > 20 ? 50 : 0, 0, sendFailCount > 20 ? 0 : 50);
  }
}

// ===== WEBSOCKET EVENT =====
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t length) {
  if (type == WStype_TEXT) {
    // Parse: "throttle,steering,aux_x,aux_y,btn1,btn2,mode"
    char *token = strtok((char *)payload, ",");
    if (token)
      dataToSend.throttle = atoi(token);
    token = strtok(NULL, ",");
    if (token)
      dataToSend.steering = atoi(token);
    token = strtok(NULL, ",");
    if (token)
      dataToSend.aux_x = atoi(token);
    token = strtok(NULL, ",");
    if (token)
      dataToSend.aux_y = atoi(token);
    token = strtok(NULL, ",");
    if (token)
      dataToSend.btn1 = atoi(token);
    token = strtok(NULL, ",");
    if (token)
      dataToSend.btn2 = atoi(token);
    token = strtok(NULL, ",");
    if (token)
      dataToSend.mode = atoi(token);

    // Kirim via ESP-NOW segera
    if (espNowReady) {
      esp_now_send(receiverMAC, (uint8_t *)&dataToSend, sizeof(dataToSend));
    }
  }
}

void handleRoot() { server.send_P(200, "text/html", HTML_PAGE); }

// ===== SETUP =====
void setup() {
  setRGB(50, 0, 0);
  SerialMon.begin(115200);
  delay(1000);

  SerialMon.println("=== WEB TRANSMITTER FAST ===");

  // WiFi AP
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password, 1, 0, 4);

  SerialMon.print("AP IP: ");
  SerialMon.println(WiFi.softAPIP());
  SerialMon.print("MAC: ");
  SerialMon.println(WiFi.macAddress());

  // ESP-NOW
  if (esp_now_init() == ESP_OK) {
    espNowReady = true;
    esp_now_register_send_cb(OnDataSent);
    memcpy(peerInfo.peer_addr, receiverMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
    SerialMon.println("ESP-NOW OK");
  }

  // Web Server
  server.on("/", handleRoot);
  server.begin();

  // WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  setRGB(0, 20, 0);
  SerialMon.println("Ready! Connect to RC_Controller WiFi");
  SerialMon.println("Open http://192.168.4.1");
}

// ===== LOOP =====
void loop() {
  webSocket.loop();
  server.handleClient();
}

/*
  Inky Pinky  v2.0 (lean)  -  pen lift for the Two Trees TTS-55 pen plotter
  -------------------------------------------------------------------------
  One rule:  laser ON  -> pen DOWN,   laser OFF -> pen UP.

  Board : ESP32 Dev Module (30-pin DevKit), Arduino IDE 2.x, esp32 core 3.x
  Library: ESP32Servo (Library Manager)
  Files : InkyPinky.ino + page.h in a folder named InkyPinky

  Wiring:
    Laser PWM (wire C) -> BAT85 (band toward PWM) -> D32
    SG90 signal -> D18
    MP1584EN 5.0 V OUT+ -> VIN + servo red
    MP1584EN OUT-       -> ESP GND (the pin that tested good) + servo brown
    1000 uF capacitor across servo red / brown (stripe to brown)

  How "laser on" is decided:
    An interrupt adds up how long D32 is HIGH. Every 5 ms the last 20 ms are
    checked: HIGH for 0.4 ms or more (2 %) = laser on. Any power from about
    2 % to 100 %, pulsing or steady, counts as on. Motor interference makes
    spikes of a few microseconds: HIGH pulses under 5 us are not counted.

  WiFi: hotspot PenPlotter-XXXX (password inkypinky) -> http://192.168.4.1
        optional home network, set on the page. Also http://inkypinky.local
  Update: Firmware page -> upload a .bin (Arduino IDE: Sketch > Export compiled binary)
  Rescue: hold BOOT 5 s -> factory reset (WiFi + angles back to defaults)
  LED (GPIO2): on = pen down, slow blink = pen up + WiFi in use,
               off = pen up, fast blink = BOOT held
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <ESP32Servo.h>
#include "soc/gpio_reg.h"

#define FW_VERSION  "2.0"
#define PIN_SIGNAL  32            // laser PWM in
#define PIN_SERVO   18            // servo signal
#define PIN_BOOT    0             // BOOT button
#define PIN_LED     2             // blue LED
#define HOSTNAME    "inkypinky"
#define AP_PASS_DEF "inkypinky"
#define TICK_MS     5             // check every 5 ms
#define WIN_TICKS   4             // look at the last 4 checks = 20 ms
#define ON_US       400           // HIGH this long in 20 ms = laser on
#define MIN_PULSE   5             // HIGH pulses shorter than 5 us are interference, not laser
#define MANUAL_MS   10000         // test buttons / sliders hold for 10 s
#define ANG_MIN     15            // SG90 safe range
#define ANG_MAX     165

// ---------- settings (kept in flash) ----------
volatile int upAng = 90, downAng = 119;
String apName, apPass, staSsid, staPass;
Preferences prefs;
WebServer server(80);
Servo servo;

// ---------- laser signal: time spent HIGH, measured by the interrupt ----------
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t lastT = 0;      // time of the last edge / check (us)
volatile uint32_t highUs = 0;     // HIGH time since the last check (us)
volatile uint32_t edges = 0;      // edges since the last check
volatile bool     level = false;  // current level of D32

void IRAM_ATTR onEdge() {
  uint32_t now = (uint32_t)esp_timer_get_time();
  bool l = REG_READ(GPIO_IN1_REG) & 1;          // GPIO32 = bit 0 of IN1
  portENTER_CRITICAL_ISR(&mux);
  uint32_t dt = now - lastT;
  if (level && dt >= MIN_PULSE) highUs = highUs + dt;   // count only real pulses
  lastT = now;
  level = l;
  edges = edges + 1;
  portEXIT_CRITICAL_ISR(&mux);
}

// ---------- pen state ----------
volatile bool laserOn = false;    // what the signal says
volatile bool penDown = false;    // what the servo does
volatile int  manualReq = 0;      // from the page: 1 = up, 2 = down
volatile int  manualLeft = 0;     // seconds left of a manual test (for the page)
volatile bool updating = false;   // firmware update running -> pen up
volatile bool netUp = false;      // someone uses WiFi (for the LED)
volatile bool bootHeld = false;   // BOOT button held (for the LED)

void penTask(void*) {
  uint32_t win[WIN_TICKS] = {0};
  int wi = 0;
  bool manual = false, manualDown = false, manualRef = false;
  uint32_t manualT = 0;
  int lastAngle = -1;

  for (;;) {
    // 1. collect HIGH time since the last check
    uint32_t now = (uint32_t)esp_timer_get_time();
    portENTER_CRITICAL(&mux);
    if (level) highUs = highUs + (now - lastT);
    lastT = now;
    uint32_t hi = highUs;
    if (edges == 0) level = REG_READ(GPIO_IN1_REG) & 1;   // no edge seen: re-read the pin, so a missed edge can't stick
    highUs = 0; edges = 0;
    portEXIT_CRITICAL(&mux);

    // 2. laser on = HIGH for ON_US or more in the last 20 ms
    win[wi] = hi; wi = (wi + 1) % WIN_TICKS;
    uint32_t sum = 0;
    for (int i = 0; i < WIN_TICKS; i++) sum += win[i];
    laserOn = (sum >= ON_US);

    // 3. test buttons / sliders: hold for 10 s, or until the laser changes
    uint32_t ms = millis();
    int rq = manualReq;
    if (rq) { manualReq = 0; manual = true; manualDown = (rq == 2); manualRef = laserOn; manualT = ms; }
    if (manual && (laserOn != manualRef || ms - manualT >= MANUAL_MS)) manual = false;
    manualLeft = manual ? (int)((MANUAL_MS - (ms - manualT) + 999) / 1000) : 0;

    // 4. pen: up during updates and the first second after start
    penDown = !updating && ms > 1000 && (manual ? manualDown : laserOn);
    int a = penDown ? downAng : upAng;
    if (a != lastAngle) { servo.write(a); lastAngle = a; }

    // 5. LED
    bool led;
    if (bootHeld)     led = (ms / 100) % 2;
    else if (penDown) led = true;
    else if (netUp)   led = (ms / 1000) % 2;
    else              led = false;
    digitalWrite(PIN_LED, led);

    vTaskDelay(pdMS_TO_TICKS(TICK_MS));
  }
}

// ---------- settings ----------
void loadSettings() {
  prefs.begin("inky", true);
  upAng   = constrain(prefs.getInt("ua", 90),  ANG_MIN, ANG_MAX);
  downAng = constrain(prefs.getInt("da", 119), ANG_MIN, ANG_MAX);
  apName  = prefs.getString("apn", "");
  apPass  = prefs.getString("app", AP_PASS_DEF);
  staSsid = prefs.getString("ss", "");
  staPass = prefs.getString("spw", "");
  prefs.end();
  if (apName.length() == 0) {
    uint64_t mac = ESP.getEfuseMac();
    char id[5];
    snprintf(id, sizeof(id), "%02X%02X", (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
    apName = String("PenPlotter-") + id;
  }
  if (apPass.length() < 8) apPass = AP_PASS_DEF;
}

void factoryReset() {
  updating = true;
  delay(300);                                    // let the pen go up
  prefs.begin("inky", false); prefs.clear(); prefs.end();
  ESP.restart();
}

// ---------- web page (page.h, same folder) ----------
#include "page.h"

String jsonEsc(const String& s) {
  String o;
  for (char c : s) {
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c >= 0x20) o += c;
  }
  return o;
}

void handleState() {
  String j = "{\"pen\":\"";  j += penDown ? "down" : "up";
  j += "\",\"laser\":";      j += laserOn ? 1 : 0;
  j += ",\"man\":";          j += manualLeft;
  j += ",\"upd\":";          j += updating ? 1 : 0;
  j += ",\"ua\":";           j += upAng;
  j += ",\"da\":";           j += downAng;
  j += ",\"ap\":\"";         j += jsonEsc(apName);
  j += "\",\"sta\":\"";      j += jsonEsc(staSsid);
  j += "\",\"staOk\":";      j += (WiFi.status() == WL_CONNECTED) ? 1 : 0;
  j += ",\"staip\":\"";      j += WiFi.localIP().toString();
  j += "\",\"ver\":\"" FW_VERSION "\"}";
  server.send(200, "application/json", j);
}

// angle slider: moves the servo live (shows that position) - "save" stores it
void handleAngle() {
  if (server.hasArg("ua")) { upAng   = constrain(server.arg("ua").toInt(), ANG_MIN, ANG_MAX); manualReq = 1; }
  if (server.hasArg("da")) { downAng = constrain(server.arg("da").toInt(), ANG_MIN, ANG_MAX); manualReq = 2; }
  server.send(200, "text/plain", "OK");
}

void handleSave() {
  prefs.begin("inky", false);
  prefs.putInt("ua", upAng); prefs.putInt("da", downAng);
  prefs.end();
  server.send(200, "text/plain", "OK");
}

void handlePen() {
  manualReq = (server.arg("s") == "down") ? 2 : 1;
  server.send(200, "text/plain", "OK");
}

void handleWifi() {
  String apn = server.arg("apn"); apn.trim();
  String app = server.arg("app");
  String ss  = server.arg("ss");  ss.trim();
  String spw = server.arg("spw");
  if (app.length() && app.length() < 8) { server.send(400, "text/plain", "Hotspot password needs 8 or more characters."); return; }
  if (apn.length() > 31 || ss.length() > 32 || app.length() > 63 || spw.length() > 63) { server.send(400, "text/plain", "Name or password too long."); return; }
  prefs.begin("inky", false);
  prefs.putString("apn", apn);                       // empty = PenPlotter-XXXX
  if (app.length()) prefs.putString("app", app);     // empty = keep
  if (ss != staSsid || spw.length()) prefs.putString("spw", spw);
  prefs.putString("ss", ss);
  prefs.end();
  server.send(200, "text/plain", "OK");
  delay(500);
  ESP.restart();
}

void handleScan() {
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_FAILED) WiFi.scanNetworks(true);
  if (n < 0) { server.send(200, "application/json", "{\"running\":1}"); return; }
  String j = "{\"running\":0,\"nets\":[";
  bool first = true;
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i).length() == 0) continue;
    if (!first) j += ',';
    j += "\"" + jsonEsc(WiFi.SSID(i)) + "\"";
    first = false;
  }
  j += "]}";
  WiFi.scanDelete();
  server.send(200, "application/json", j);
}

void setupServer() {
  server.on("/",            HTTP_GET,  []() { server.send(200, "text/html", PAGE); });
  server.on("/api/state",   HTTP_GET,  handleState);
  server.on("/api/angle",   HTTP_GET,  handleAngle);
  server.on("/api/save",    HTTP_POST, handleSave);
  server.on("/api/pen",     HTTP_GET,  handlePen);
  server.on("/api/wifi",    HTTP_POST, handleWifi);
  server.on("/api/scan",    HTTP_GET,  handleScan);
  server.on("/api/restart", HTTP_POST, []() { server.send(200, "text/plain", "OK"); updating = true; delay(500); ESP.restart(); });
  server.on("/api/reset",   HTTP_POST, []() { server.send(200, "text/plain", "OK"); factoryReset(); });

  // firmware upload from the browser
  server.on("/update", HTTP_POST,
    []() {
      bool ok = !Update.hasError();
      server.sendHeader("Connection", "close");
      server.send(ok ? 200 : 500, "text/plain", ok ? "OK" : Update.errorString());
      if (ok) { delay(500); ESP.restart(); }
      updating = false;
    },
    []() {
      HTTPUpload& up = server.upload();
      if (up.status == UPLOAD_FILE_START)        { updating = true; Update.begin(UPDATE_SIZE_UNKNOWN); }
      else if (up.status == UPLOAD_FILE_WRITE)   { Update.write(up.buf, up.currentSize); }
      else if (up.status == UPLOAD_FILE_END)     { Update.end(true); }
      else if (up.status == UPLOAD_FILE_ABORTED) { Update.abort(); updating = false; }
    });

  server.onNotFound([]() { server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); });
  server.begin();
}

// ---------- setup ----------
void setup() {
  Serial.begin(115200);
  loadSettings();

  servo.setPeriodHertz(50);
  servo.attach(PIN_SERVO, 500, 2400);
  servo.write(upAng);                            // start with the pen up

  pinMode(PIN_SIGNAL, INPUT_PULLUP);
  pinMode(PIN_BOOT, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  level = digitalRead(PIN_SIGNAL);
  lastT = (uint32_t)esp_timer_get_time();
  attachInterrupt(digitalPinToInterrupt(PIN_SIGNAL), onEdge, CHANGE);

  xTaskCreatePinnedToCore(penTask, "pen", 4096, NULL, 1, NULL, 1);

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName.c_str(), apPass.c_str());
  if (staSsid.length()) WiFi.begin(staSsid.c_str(), staPass.c_str());
  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);

  setupServer();
  Serial.printf("\nInky Pinky %s  ->  hotspot %s  http://192.168.4.1\n", FW_VERSION, apName.c_str());
}

// ---------- loop ----------
void loop() {
  server.handleClient();

  // home network: give up after 30 s, retry every 5 min (constant retrying makes the hotspot unstable)
  static uint32_t staT = 0;
  static bool gaveUp = false;
  if (staSsid.length()) {
    if (WiFi.status() == WL_CONNECTED)                 { staT = millis(); gaveUp = false; }
    else if (!gaveUp && millis() - staT > 30000)       { WiFi.disconnect(false, false); gaveUp = true; staT = millis(); }
    else if (gaveUp && millis() - staT > 300000)       { WiFi.begin(staSsid.c_str(), staPass.c_str()); gaveUp = false; staT = millis(); }
  }
  netUp = (WiFi.status() == WL_CONNECTED) || (WiFi.softAPgetStationNum() > 0);

  // BOOT held 5 s -> factory reset
  static uint32_t bootT = 0;
  if (digitalRead(PIN_BOOT) == LOW) {
    if (!bootT) bootT = millis();
    bootHeld = millis() - bootT > 300;
    if (millis() - bootT > 5000) factoryReset();
  } else { bootT = 0; bootHeld = false; }

  delay(2);
}

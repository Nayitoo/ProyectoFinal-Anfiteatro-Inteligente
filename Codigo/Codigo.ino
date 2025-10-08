/********************* INCLUDES BÁSICOS *********************/
#include <WiFi.h>
#include <WebServer.h>
#include "Principal.h"
#include "Admin.h"

// === CREDENCIALES WIFI ===
const char* ssid     = "LABO";
const char* password = "";

WebServer server(80);

/* ===== Librerías de hardware ===== */
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WebSocketsServer.h>   // WebSocket en puerto 81

/********** Pines / Hardware **********/
#define CAPACITY_MAX 40
LiquidCrystal_I2C lcd(0x27, 16, 2);

// RFID RC522 (VSPI)
#define RST_PIN 4
#define SS_PIN  5
MFRC522 rfid(SS_PIN, RST_PIN);

// DHT11
#define DHT_PIN   14
#define DHT_TYPE  DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// IR (egreso) — OJO: GPIO34 es solo-entrada (sin pull-ups internos)
#define IR_PIN 34
#define IR_ACTIVE_LOW 1

// Stepper 28BYJ-48 + ULN2003
#define M_IN1 25
#define M_IN2 26
#define M_IN3 27
#define M_IN4 32

// Ventiladores
#define FAN1_PIN 12
#define FAN2_PIN 33

#define TEMP_ON_FAN   26.0
#define TEMP_OFF_FAN  25.0

const uint32_t T_ABIERTO_MS = 30000;
const uint32_t T_FUNCION_MS = 35000;
const uint32_t T_ESPERA_MS  = 30000;

/* ===== Estado general ===== */
enum Estado { EST_ABIERTO, EST_EN_FUNCION, EST_CERRADO };
enum Phase  { PH_ABIERTO,  PH_FUNCION,    PH_ESPERA   };
Estado estado = EST_ABIERTO;
Phase  phase  = PH_ABIERTO;
uint32_t phaseStart = 0;

volatile uint16_t ingresos = 0;
volatile uint16_t egresos  = 0;
float temperaturaC = 0.0;
float humedadRel   = 0.0;
bool  fansOn = false;

uint32_t lcdMsgUntilMs = 0;
uint32_t egresoBlockUntilMs = 0;
const uint16_t BLOCK_EGRESO_MS = 3000;

/* ===== Stepper ===== */
const uint8_t halfStep[8][4] = {
  {1,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,1,0},
  {0,0,1,0},{0,0,1,1},{0,0,0,1},{1,0,0,1}
};
const uint16_t OPEN_STEPS = 2048;
const uint16_t STEP_INTERVAL_MS = 1;

enum CurtainState { CURT_IDLE, CURT_OPENING, CURT_CLOSING };
CurtainState curtState = CURT_IDLE;
uint16_t curtStepsRemaining = 0;
uint8_t  stepIndex = 0;
uint32_t lastStepMs = 0;

/* ===== Robustez RFID ===== */
uint32_t lastRFIDSeenMs = 0;
uint32_t lastRFIDInitMs = 0;
const uint32_t RFID_REINIT_MS = 10000;
const uint32_t RFID_ANTIREP_MS = 800;
byte lastUid[10]; uint8_t lastUidLen = 0; uint32_t lastUidMs = 0;

/* ===== WebSocket ===== */
WebSocketsServer ws(81);           // WS en puerto 81
void wsBroadcastStatus();          // fwd decl

/* ========================== UTILS ========================== */
static inline bool isInputOnlyGPIO(int pin){
  return (pin>=34 && pin<=39); // ESP32: 34–39 son solo-entrada (sin PUs)
}

void motorWriteStep(uint8_t idx){
  digitalWrite(M_IN1, halfStep[idx][0]);
  digitalWrite(M_IN2, halfStep[idx][1]);
  digitalWrite(M_IN3, halfStep[idx][2]);
  digitalWrite(M_IN4, halfStep[idx][3]);
}
void curtainOpenStart(){  curtState = CURT_OPENING;  curtStepsRemaining = OPEN_STEPS; }
void curtainCloseStart(){ curtState = CURT_CLOSING;  curtStepsRemaining = OPEN_STEPS; }
void curtainTask(){
  if(curtState==CURT_IDLE) return;
  if(millis()-lastStepMs < STEP_INTERVAL_MS) return;
  lastStepMs = millis();
  stepIndex = (curtState==CURT_OPENING) ? ((stepIndex + 1) & 0x07) : ((stepIndex + 7) & 0x07);
  motorWriteStep(stepIndex);
  if(curtStepsRemaining>0) curtStepsRemaining--;
  if(curtStepsRemaining==0){
    curtState = CURT_IDLE;
    digitalWrite(M_IN1, LOW); digitalWrite(M_IN2, LOW);
    digitalWrite(M_IN3, LOW); digitalWrite(M_IN4, LOW);
  }
}

/* ===== IR – egresos ===== */
bool irLast = (IR_ACTIVE_LOW ? HIGH : LOW);
uint32_t irLastChange = 0;
const uint16_t IR_DEBOUNCE_MS = 50;
void readIR(){
  int v = digitalRead(IR_PIN);
  uint32_t now = millis();
  if(v != irLast && (now - irLastChange) > IR_DEBOUNCE_MS){
    irLastChange = now;
    irLast = v;
    bool active = IR_ACTIVE_LOW ? (v==LOW) : (v==HIGH);
    if(active){
      if (now > egresoBlockUntilMs) {
        if (egresos < ingresos) {
          egresos++;
          wsBroadcastStatus();                 // PUSH inmediato
        }
      }
    }
  }
}

/* ===== RFID helpers ===== */
bool rfidAlive(){
  byte ver = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  return (ver != 0x00 && ver != 0xFF);
}
void rfidBeginHard(){
  SPI.end();
  SPI.begin(18, 19, 23, SS_PIN);  // SCK,MISO,MOSI,SS
  pinMode(SS_PIN, OUTPUT); digitalWrite(SS_PIN, HIGH);
  pinMode(RST_PIN, OUTPUT); digitalWrite(RST_PIN, HIGH);
  delay(5);
  rfid.PCD_Init(SS_PIN, RST_PIN);
  delay(50);
  rfid.PCD_AntennaOn();
  lastRFIDInitMs = millis();
  Serial.print("[RFID] VersionReg=0x");
  Serial.println(rfid.PCD_ReadRegister(MFRC522::VersionReg), HEX);
}
bool sameUid(const byte *uid, byte len){
  if(len!=lastUidLen) return false;
  for(uint8_t i=0;i<len;i++) if(uid[i]!=lastUid[i]) return false;
  return true;
}
void memorizeUid(const byte *uid, byte len){
  lastUidLen = len; for(uint8_t i=0;i<len;i++) lastUid[i]=uid[i]; lastUidMs = millis();
}

/* ===== RFID – ingresos ===== */
uint32_t lastRFIDMs = 0;
const uint16_t RFID_PERIOD_MS = 30;
void rfidTask(){
  uint32_t now = millis();
  if((now - lastRFIDInitMs) > RFID_REINIT_MS){
    if(!rfidAlive()){
      Serial.println("[RFID] No responde; re-inicializando...");
      rfidBeginHard();
    }
    lastRFIDInitMs = now;
  }
  if(now - lastRFIDMs < RFID_PERIOD_MS) return;
  lastRFIDMs = now;

  if(!rfid.PICC_IsNewCardPresent()) return;
  if(!rfid.PICC_ReadCardSerial())   return;

  if(sameUid(rfid.uid.uidByte, rfid.uid.size) && (now - lastUidMs) < RFID_ANTIREP_MS){
    rfid.PICC_HaltA(); rfid.PCD_StopCrypto1();
    return;
  }
  memorizeUid(rfid.uid.uidByte, rfid.uid.size);

  bool entradasHabilitadas = (phase == PH_ABIERTO) || (phase == PH_FUNCION);
  if(entradasHabilitadas && ingresos < CAPACITY_MAX){
    egresoBlockUntilMs = now + BLOCK_EGRESO_MS;
    lcdMsgUntilMs = now + 3000;
    ingresos++;
    lastRFIDSeenMs = now;
    Serial.print("[RFID] UID:");
    for(byte i=0;i<rfid.uid.size;i++){ Serial.print(rfid.uid.uidByte[i], HEX); Serial.print(i<rfid.uid.size-1?":":""); }
    Serial.println();
    wsBroadcastStatus();                 // PUSH inmediato
  }
  rfid.PICC_HaltA(); rfid.PCD_StopCrypto1();
}

/* ===== DHT + ventiladores ===== */
uint32_t lastDHT = 0;
const uint32_t DHT_PERIOD_MS = 2000;
void tempTask(){
  if(millis()-lastDHT < DHT_PERIOD_MS) return;
  lastDHT = millis();

  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if(!isnan(t)) temperaturaC = t;
  if(!isnan(h)) humedadRel   = h;

  bool changed = false;
  if(temperaturaC >= TEMP_ON_FAN && !fansOn){
    fansOn = true;  changed = true;
    digitalWrite(FAN1_PIN, HIGH); digitalWrite(FAN2_PIN, HIGH);
  } else if(temperaturaC < TEMP_OFF_FAN && fansOn){
    fansOn = false; changed = true;
    digitalWrite(FAN1_PIN, LOW);  digitalWrite(FAN2_PIN, LOW);
  }
  if(changed) wsBroadcastStatus();      // PUSH si cambian fans
}

/* ===== Fases ===== */
uint32_t phaseDurationMs(){
  switch(phase){
    case PH_ABIERTO: return T_ABIERTO_MS;
    case PH_FUNCION: return T_FUNCION_MS;
    case PH_ESPERA:  return T_ESPERA_MS;
  } return 0;
}
uint32_t phaseRemainingMs(){
  uint32_t elapsed = millis() - phaseStart;
  uint32_t dur = phaseDurationMs();
  return (elapsed >= dur) ? 0 : (dur - elapsed);
}
const char* phaseName(){
  switch(phase){
    case PH_ABIERTO: return "abierto";
    case PH_FUNCION: return "funcion";
    case PH_ESPERA:  return "espera";
  } return "desconocida";
}

/* ===== LCD ===== */
uint32_t lastLCD = 0;
void lcdTask(){
  if(millis()-lastLCD < 250) return;
  lastLCD = millis();

  if(millis() < lcdMsgUntilMs){
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Pase,");
    lcd.setCursor(0,1); lcd.print("disfrute la funcion");
    return;
  }

  lcd.setCursor(0,0);
  char l0[17];
  if(phase==PH_FUNCION)      snprintf(l0, sizeof(l0), "EN FUNCION     ");
  else if(phase==PH_ESPERA)  snprintf(l0, sizeof(l0), "EN ESPERA      ");
  else                       snprintf(l0, sizeof(l0), "ABIERTO        ");
  lcd.print(l0);

  lcd.setCursor(0,1);
  char l1[17];
  if (phase == PH_FUNCION || phase == PH_ESPERA) {
    uint32_t rem = phaseRemainingMs();
    uint16_t sec = rem / 1000;
    uint8_t mm = sec / 60, ss = sec % 60;
    snprintf(l1, sizeof(l1), "Queda %02u:%02u     ", mm, ss);
  } else {
    snprintf(l1, sizeof(l1), "T:%2.0fC H:%2.0f%%   ", temperaturaC, humedadRel);
  }
  lcd.print(l1);
}

/* ===== Ocupación actual ===== */
uint16_t ocupacionActual(){
  int val = (int)ingresos - (int)egresos;
  if(val < 0) val = 0;
  if(val > CAPACITY_MAX) val = CAPACITY_MAX;
  return (uint16_t)val;
}

/* ===== Ciclo del telón ===== */
void cycleInit(){
  phase = PH_ABIERTO;
  phaseStart = millis();
  estado = EST_ABIERTO;
  curtState = CURT_IDLE; curtStepsRemaining = 0;
  digitalWrite(M_IN1, LOW); digitalWrite(M_IN2, LOW);
  digitalWrite(M_IN3, LOW); digitalWrite(M_IN4, LOW);
}

void cycleTask(){
  uint32_t now = millis();
  switch(phase){
    case PH_ABIERTO:{
      estado = EST_ABIERTO;
      if(now - phaseStart >= T_ABIERTO_MS){
        phase = PH_FUNCION; phaseStart = now; estado = EST_EN_FUNCION;
        curtainOpenStart();
        wsBroadcastStatus();           // PUSH cambio de fase
      }
    }break;
    case PH_FUNCION:{
      estado = EST_EN_FUNCION;
      if(now - phaseStart >= T_FUNCION_MS){
        phase = PH_ESPERA; phaseStart = now; estado = EST_ABIERTO;
        curtainCloseStart();
        wsBroadcastStatus();           // PUSH cambio de fase
      }
    }break;
    case PH_ESPERA:{
      estado = EST_ABIERTO;
      if(now - phaseStart >= T_ESPERA_MS){
        phase = PH_ABIERTO; phaseStart = now; estado = EST_ABIERTO;
        wsBroadcastStatus();           // PUSH cambio de fase
      }
    }break;
  }
}

/* ===== JSON ===== */
String jsonStatus(){
  uint16_t ocup = ocupacionActual();
  uint8_t pct = (uint8_t)round((ocup * 100.0) / CAPACITY_MAX);

  String s = "{";
  s += "\"estado\":\"" + String((phase==PH_FUNCION)?"en-funcion":"abierto") + "\",";
  s += "\"fase\":\"" + String(phaseName()) + "\",";
  s += "\"t_restante_ms\":" + String(phaseRemainingMs()) + ",";
  s += "\"temp\":" + String(temperaturaC, 1) + ",";
  s += "\"hum\":"  + String(humedadRel, 0) + ",";
  s += "\"ingresos\":" + String(ingresos) + ",";
  s += "\"egresos\":" + String(egresos) + ",";
  s += "\"aforo_actual\":" + String(ocup) + ",";
  s += "\"aforo_max\":" + String(CAPACITY_MAX) + ",";
  s += "\"aforo_pct\":" + String(pct) + ",";
  s += "\"fansOn\":" + String(fansOn ? "true" : "false") + ",";
  s += "\"rfid_alive\":" + String(rfidAlive() ? "true" : "false");
  s += "}";
  return s;
}

/* ===== WebSocket helpers ===== */
void wsBroadcastStatus(){
  String s = jsonStatus();
  ws.broadcastTXT(s);
}
void onWsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  if(type == WStype_CONNECTED){
    IPAddress ip = ws.remoteIP(num);
    Serial.printf("[WS] Cliente %u conectado desde %s\n", num, ip.toString().c_str());

    // >>> FIX: no pasar rvalue; guardamos primero en una variable
    String s = jsonStatus();
    ws.sendTXT(num, s);   // estado inicial

  } else if(type == WStype_DISCONNECTED){
    Serial.printf("[WS] Cliente %u desconectado\n", num);
  }
}


/* ===== HTTP Handlers ===== */
void handleRoot(){  server.send(200, "text/html", MAIN_page); }
void handleAdmin(){ server.send(200, "text/html", ADMIN_page); }
void handleStatus(){
  String body = jsonStatus();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", body);
}
void handleReset(){
  ingresos = 0; egresos = 0;
  Serial.println("[ADMIN] Reset de aforo");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"ok\":true,\"action\":\"reset\"}");
  wsBroadcastStatus();                       // PUSH
}
void handleOptions(){
  server.sendHeader("Access-Control-Allow-Origin","*");
  server.sendHeader("Access-Control-Allow-Methods","GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers","Content-Type");
  server.send(204);
}
void handleNotFound(){ server.send(404, "text/plain", "404"); }

/* ========================== SETUP ========================== */
void setup() {
  Serial.begin(115200); delay(100);

  // ==== IR PIN SAFE CONFIG ====
  if (IR_ACTIVE_LOW) {
    if (isInputOnlyGPIO(IR_PIN)) {
      // GPIO34–39: sin pull-ups internos
      pinMode(IR_PIN, INPUT);
      Serial.println("[IR] GPIO es solo-entrada (34–39). Usando INPUT sin PULLUP. Si tu sensor no trae pull-up, agregá 10k a 3.3V.");
    } else {
      pinMode(IR_PIN, INPUT_PULLUP);
    }
  } else {
    pinMode(IR_PIN, INPUT);
  }

  // Stepper
  pinMode(M_IN1, OUTPUT); pinMode(M_IN2, OUTPUT);
  pinMode(M_IN3, OUTPUT); pinMode(M_IN4, OUTPUT);
  digitalWrite(M_IN1, LOW); digitalWrite(M_IN2, LOW);
  digitalWrite(M_IN3, LOW); digitalWrite(M_IN4, LOW);

  // Fans
  pinMode(FAN1_PIN, OUTPUT); pinMode(FAN2_PIN, OUTPUT);
  digitalWrite(FAN1_PIN, LOW); digitalWrite(FAN2_PIN, LOW);

  // LCD
  Wire.begin(21,22); Wire.setClock(100000);
  lcd.init(); lcd.backlight(); lcd.clear();
  lcd.setCursor(0,0); lcd.print("Inicializando");
  lcd.setCursor(0,1); lcd.print("WiFi...");

  // DHT
  dht.begin();

  // RFID
  rfidBeginHard();

  // WiFi
  WiFi.begin(ssid, password);
  uint32_t wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && (millis()-wifiStart) < 15000) {
    delay(500); Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WiFi] No conectado (timeout)");
  }

  lcd.clear(); lcd.setCursor(0,0); lcd.print("IP:");
  lcd.setCursor(0,1); lcd.print(WiFi.localIP().toString());

  // HTTP
  server.on("/",         HTTP_GET,  handleRoot);
  server.on("/admin",    HTTP_GET,  handleAdmin);
  server.on("/status",   HTTP_GET,  handleStatus);
  server.on("/reset",    HTTP_POST, handleReset);
  server.on("/reset",    HTTP_GET,  handleReset);
  server.on("/reset",    HTTP_OPTIONS, handleOptions);
  server.onNotFound(handleNotFound);
  server.begin();

  // WebSocket
  ws.begin();
  ws.onEvent(onWsEvent);

  // Ciclo
  cycleInit();
}

/* ========================== LOOP ========================== */
void loop() {
  server.handleClient();
  ws.loop();

  curtainTask();
  rfidTask();
  readIR();
  tempTask();
  lcdTask();
  cycleTask();
}

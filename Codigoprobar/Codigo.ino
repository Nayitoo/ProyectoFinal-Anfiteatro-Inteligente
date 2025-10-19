/********************* INCLUDES B??SICOS *********************/
#include <WiFi.h>          // WiFi
#include <WebServer.h>     // Servidor HTTP
#include <WebSocketsServer.h> // WebSocket broadcast
#include "Principal.h"     // P??gina de USUARIO (ruta "/")
#include "Admin.h"         // P??gina de ADMIN    (ruta "/admin")

const char* ssid     = "LABO";
const char* password = "";
WebServer server(80);
WebSocketsServer ws(81);
void wsBroadcastStatus();
/************************************************************/

/* ===== Librer??as de hardware ===== */
#include <SPI.h>
#include <MFRC522.h>            // RFID RC522
#include <Wire.h>
#include <LiquidCrystal_I2C.h>  // LCD I2C
#include <DHT.h>                // DHT11

/********** Pines / Hardware **********
 LCD I2C: VCC=5V/3V3, GND, SDA=GPIO21, SCL=GPIO22
 RFID RC522: VCC=3.3V, GND, RST=GPIO4, SDA=GPIO5, SCK=18, MOSI=23, MISO=19
 Sensor IR (egreso): OUT=GPIO34
 DHT11: DATA=GPIO14
 Motor 28BYJ-48 + ULN2003: IN1=GPIO25, IN2=GPIO26, IN3=GPIO27, IN4=GPIO32
 Ventiladores (virtuales por ahora): FAN1=GPIO12, FAN2=GPIO33
****************************************/

const uint16_t CAPACITY_MAX_DEFAULT = 40;
uint16_t capacityMax = CAPACITY_MAX_DEFAULT;

// LCD I2C (cambia 0x27 por 0x3F si tu m??dulo usa esa direcci??n)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// RFID RC522 (SPI)
#define RST_PIN   4
#define SS_PIN    5
MFRC522 rfid(SS_PIN, RST_PIN);

// DHT11
#define DHT_PIN   14
#define DHT_TYPE  DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// Sensor IR (egresos)
#define IR_PIN    34

// Motor paso a paso (28BYJ-48 + ULN2003) ??? secuencia half-step
#define M_IN1 25
#define M_IN2 26
#define M_IN3 27
#define M_IN4 32

// Ventiladores (virtuales)
#define FAN1_PIN 12
#define FAN2_PIN 33

// LED de estado (RGB)
#define STATUS_LED_R_PIN 15
#define STATUS_LED_G_PIN 2
#define STATUS_LED_B_PIN 13

// Rel?? de alimentaci??n
#define RELAY_PIN 17

/* ===== Ventiladores con umbrales de test =====
   ON  a 26.0 ??C
   OFF a 25.0 ??C
   (histeresis 1 ??C)
*/
#define TEMP_ON_FAN   26.0
#define TEMP_OFF_FAN  25.0

/* ===== Timings del ciclo =====
   ABIERTO  : 30 s  (tel??n cerrado)
   EN FUNCI??N: 35 s (tel??n abre y queda abierto)
   ESPERA   : 30 s  (tel??n se cierra; salen)
*/
const uint32_t T_ABIERTO_MS_DEFAULT  = 30000;
const uint32_t T_FUNCION_MS_DEFAULT  = 35000;
const uint32_t T_ESPERA_MS_DEFAULT   = 30000;
uint32_t durAbiertoMs = T_ABIERTO_MS_DEFAULT;
uint32_t durFuncionMs = T_FUNCION_MS_DEFAULT;
uint32_t durEsperaMs  = T_ESPERA_MS_DEFAULT;

/* ===== Estado general ===== */
enum Estado { EST_ABIERTO, EST_EN_FUNCION, EST_CERRADO };
Estado estado = EST_ABIERTO;

/* ===== Fases del tel??n (orden ABIERTO ??? FUNCI??N ??? ESPERA) ===== */
enum Phase { PH_ABIERTO, PH_FUNCION, PH_ESPERA };
Phase phase = PH_ABIERTO;
uint32_t phaseStart = 0;

volatile uint16_t ingresos = 0;  // suma RFID
volatile uint16_t egresos = 0;   // suma IR
float  temperaturaC = 0.0;
float  humedadRel   = 0.0;
bool   fansOn = false;
bool   relayOn = false;

// Mensaje temporal al pasar tarjeta
uint32_t lcdMsgUntilMs = 0;

// Bloqueo de egreso tras RFID (evita que la misma mano dispare el IR)
uint32_t egresoBlockUntilMs = 0;
const uint16_t BLOCK_EGRESO_MS = 3000;

/* ===== Stepper (28BYJ-48) ===== */
const uint8_t halfStep[8][4] = {
  {1,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,1,0},
  {0,0,1,0},{0,0,1,1},{0,0,0,1},{1,0,0,1}
};
const uint16_t OPEN_STEPS = 2048;    // Ajustar a tu carrera real
const uint16_t STEP_INTERVAL_MS = 3; // Velocidad

enum CurtainState { CURT_IDLE, CURT_OPENING, CURT_CLOSING };
CurtainState curtState = CURT_IDLE;
uint16_t curtStepsRemaining = 0;
uint8_t  stepIndex = 0;
uint32_t lastStepMs = 0;

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

  if(curtState==CURT_OPENING){ stepIndex = (stepIndex + 1) & 0x07; }
  else                        { stepIndex = (stepIndex + 7) & 0x07; }
  motorWriteStep(stepIndex);

  if(curtStepsRemaining>0) curtStepsRemaining--;
  if(curtStepsRemaining==0){
    curtState = CURT_IDLE;
    // desenergizar bobinas
    digitalWrite(M_IN1, LOW); digitalWrite(M_IN2, LOW);
    digitalWrite(M_IN3, LOW); digitalWrite(M_IN4, LOW);
  }
}

void setStatusLed(bool redOn, bool greenOn, bool blueOn){
  digitalWrite(STATUS_LED_R_PIN, redOn ? HIGH : LOW);
  digitalWrite(STATUS_LED_G_PIN, greenOn ? HIGH : LOW);
  digitalWrite(STATUS_LED_B_PIN, blueOn ? HIGH : LOW);
}

void refreshStatusLed(){
  switch(phase){
    case PH_ABIERTO:
      setStatusLed(false, true, false);   // Verde
      break;
    case PH_FUNCION:
      setStatusLed(true, true, false);    // Amarillo
      break;
    case PH_ESPERA:
    default:
      setStatusLed(true, false, false);   // Rojo
      break;
  }
}

/* ===== IR ??? egresos (antirrebote + bloqueo post-RFID) ===== */
bool irLast = HIGH;         // seg??n m??dulo puede ser activo-bajo
uint32_t irLastChange = 0;
const uint16_t IR_DEBOUNCE_MS = 50;
void readIR(){
  int v = digitalRead(IR_PIN);
  uint32_t now = millis();
  if(v != irLast && (now - irLastChange) > IR_DEBOUNCE_MS){
    irLastChange = now;
    irLast = v;
    if(v == LOW){ // flanco activo (corte del haz)
      // Salidas permitidas siempre (respetando bloqueo e integridad)
      if (now > egresoBlockUntilMs) {
        if (egresos < ingresos) {
          egresos++;
          wsBroadcastStatus();
          // Serial.println("[IR] Egreso contabilizado");
        }
      } else {
        // Serial.println("[IR] Ignorado por bloqueo post-RFID");
      }
    }
  }
}

/* ===== RFID ??? ingresos (sin anti-repetici??n, pero condicionado por fase) ===== */
uint32_t lastRFIDMs = 0;
const uint16_t RFID_PERIOD_MS = 50;
void rfidTask(){
  if(millis()-lastRFIDMs < RFID_PERIOD_MS) return;
  lastRFIDMs = millis();
  if(!rfid.PICC_IsNewCardPresent()) return;
  if(!rfid.PICC_ReadCardSerial()) return;

  // Entradas habilitadas solo en ABIERTO y EN FUNCI??N
  bool entradasHabilitadas = (phase == PH_ABIERTO) || (phase == PH_FUNCION);

  if(entradasHabilitadas && ingresos < capacityMax){
    egresoBlockUntilMs = millis() + BLOCK_EGRESO_MS;   // bloqueo egreso 3s
    lcdMsgUntilMs = millis() + 3000;                   // mensaje 3s
    Serial.println("[RFID] Tarjeta detectada");
    ingresos++;
    wsBroadcastStatus();
  } else {
    // Serial.println("[RFID] Entrada bloqueada (ESPERA/AFORO)");
  }

  rfid.PICC_HaltA(); rfid.PCD_StopCrypto1();
}
bool rfidAlive(){
  byte ver = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  return (ver != 0x00 && ver != 0xFF);
}


/* ===== DHT ??? lectura + ventiladores (virtuales) ===== */
uint32_t lastDHT = 0;
const uint32_t DHT_PERIOD_MS = 2000;
void tempTask(){
  if(millis()-lastDHT < DHT_PERIOD_MS) return;
  lastDHT = millis();

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if(!isnan(t)) temperaturaC = t;
  if(!isnan(h)) humedadRel   = h;

  // Log a serie (??til para ver thresholds)
  if(!isnan(t)){
    Serial.print("[TEMP] "); Serial.print(temperaturaC, 1);
    Serial.print(" C  [HUM] "); Serial.print(humedadRel, 0); Serial.println(" %");
  }

  bool statusChanged = false;

  // Ventiladores con nuevos umbrales de test (reflejado en /status)
  if(temperaturaC >= TEMP_ON_FAN && !fansOn){
    fansOn = true;
    digitalWrite(FAN1_PIN, HIGH);
    digitalWrite(FAN2_PIN, HIGH);
    statusChanged = true;
  } else if(temperaturaC < TEMP_OFF_FAN && fansOn){
    fansOn = false;
    digitalWrite(FAN1_PIN, LOW);
    digitalWrite(FAN2_PIN, LOW);
    statusChanged = true;
  }

  // Rel?? gobernado por temperatura (usa misma hist??resis que ventiladores)
  if(temperaturaC >= TEMP_ON_FAN && !relayOn){
    relayOn = true;
    digitalWrite(RELAY_PIN, HIGH);
    statusChanged = true;
  } else if(temperaturaC < TEMP_OFF_FAN && relayOn){
    relayOn = false;
    digitalWrite(RELAY_PIN, LOW);
    statusChanged = true;
  }

  if(statusChanged){
    wsBroadcastStatus();
  }
}

/* ===== Utilidades de tiempo de fase ===== */
uint32_t phaseDurationMs(){
  switch(phase){
    case PH_ABIERTO: return durAbiertoMs;
    case PH_FUNCION: return durFuncionMs;
    case PH_ESPERA:  return durEsperaMs;
  }
  return 0;
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
  }
  return "desconocida";
}

/* ===== LCD ===== */
uint32_t lastLCD = 0;
void lcdTask(){
  if(millis()-lastLCD < 250) return;
  lastLCD = millis();

  // Mensaje temporal al pasar tarjeta
  if(millis() < lcdMsgUntilMs){
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Pase,");
    lcd.setCursor(0,1); lcd.print("disfrute la funcion");
    return;
  }

  // L??nea 1: estado
  lcd.setCursor(0,0);
  char l0[17];
  if(phase==PH_FUNCION)      snprintf(l0, sizeof(l0), "EN FUNCION     ");
  else if(phase==PH_ESPERA)  snprintf(l0, sizeof(l0), "EN ESPERA      ");
  else /* PH_ABIERTO */      snprintf(l0, sizeof(l0), "ABIERTO        ");
  lcd.print(l0);

  // L??nea 2: contador en FUNCION/ESPERA; T/H en ABIERTO
  lcd.setCursor(0,1);
  char l1[17];
  if (phase == PH_FUNCION || phase == PH_ESPERA) {
    uint32_t rem = phaseRemainingMs();
    uint16_t sec = rem / 1000;
    uint8_t mm = sec / 60;
    uint8_t ss = sec % 60;
    snprintf(l1, sizeof(l1), "Queda %02u:%02u     ", mm, ss);
  } else { // ABIERTO
    snprintf(l1, sizeof(l1), "T:%2.0fC H:%2.0f%%   ", temperaturaC, humedadRel);
  }
  lcd.print(l1);
}

/* ===== Ocupaci??n actual ===== */
uint16_t ocupacionActual(){
  int val = (int)ingresos - (int)egresos;
  if(val < 0) val = 0;
  if(val > capacityMax) val = capacityMax;
  return (uint16_t)val;
}

/* ===== Ciclo del tel??n (solo mover en cambios de fase) ===== */
void cycleInit(){
  phase = PH_ABIERTO;
  phaseStart = millis();
  estado = EST_ABIERTO;
  refreshStatusLed();

  // Asegurar bobinas en reposo; NO mover el tel??n aqu??.
  curtState = CURT_IDLE; curtStepsRemaining = 0;
  digitalWrite(M_IN1, LOW); digitalWrite(M_IN2, LOW);
  digitalWrite(M_IN3, LOW); digitalWrite(M_IN4, LOW);
}

void cycleTask(){
  uint32_t now = millis();
  switch(phase){
    case PH_ABIERTO:{
      estado = EST_ABIERTO;
      if(now - phaseStart >= durAbiertoMs){
        phase = PH_FUNCION;
        phaseStart = now;
        estado = EST_EN_FUNCION;
        curtainOpenStart();
        wsBroadcastStatus(); // *** SOLO aqu?? abrimos el tel??n ***
      }
    }break;

    case PH_FUNCION:{
      estado = EST_EN_FUNCION;
      if(now - phaseStart >= durFuncionMs){
        phase = PH_ESPERA;
        phaseStart = now;
        estado = EST_ABIERTO; // (legacy) la fase indica ESPERA
        curtainCloseStart();  // *** SOLO aqu?? cerramos el tel??n ***
      }
    }break;

    case PH_ESPERA:{
      estado = EST_ABIERTO;   // compat: estado visible ???abierto???; fase=espera
      if(now - phaseStart >= durEsperaMs){
        // Volvemos a ABIERTO; el tel??n ya qued?? cerrado en el cambio anterior
        phase = PH_ABIERTO;
        phaseStart = now;
        estado = EST_ABIERTO;
        wsBroadcastStatus();
        // NO mover tel??n aqu??
      }
    }break;
  }

  refreshStatusLed();
}

/* ===== JSON de estado ===== */
String jsonStatus(){
  uint16_t ocup = ocupacionActual();
  uint8_t pct = (uint8_t)round((ocup * 100.0) / capacityMax);

  String s = "{";
  s += "\"estado\":\"" + String((phase==PH_FUNCION)?"en-funcion":"abierto") + "\",";
  s += "\"fase\":\"" + String(phaseName()) + "\",";
  s += "\"t_restante_ms\":" + String(phaseRemainingMs()) + ",";
  s += "\"temp\":" + String(temperaturaC, 1) + ",";
  s += "\"hum\":"  + String(humedadRel, 0) + ",";
  s += "\"ingresos\":" + String(ingresos) + ",";
  s += "\"egresos\":" + String(egresos) + ",";
  s += "\"aforo_actual\":" + String(ocup) + ",";
  s += "\"aforo_max\":" + String(capacityMax) + ",";
  s += "\"aforo_pct\":" + String(pct) + ",";
  s += "\"dur_abierto_ms\":" + String(durAbiertoMs) + ",";
  s += "\"dur_funcion_ms\":" + String(durFuncionMs) + ",";
  s += "\"dur_espera_ms\":" + String(durEsperaMs) + ",";
  s += "\"fansOn\":" + String(fansOn ? "true" : "false") + ",";
  s += "\"relayOn\":" + String(relayOn ? "true" : "false") + ",";
  s += "\"rfid_alive\":" + String(rfidAlive() ? "true" : "false");
  s += "}";
  return s;
}

void wsBroadcastStatus(){
  String payload = jsonStatus();
  ws.broadcastTXT(payload);
}

void onWsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  if(type == WStype_CONNECTED){
    String msg = jsonStatus();
    ws.sendTXT(num, msg);
  }
}


static String jsonExtractString(const String& src, const char* key){
  String pattern = "\"" + String(key) + "\"";
  int idx = src.indexOf(pattern);
  if(idx < 0) return "";
  idx = src.indexOf(':', idx + pattern.length());
  if(idx < 0) return "";
  idx++;
  while(idx < (int)src.length() && (src[idx]==' ' || src[idx]=='\r' || src[idx]=='\n')) idx++;
  if(idx >= (int)src.length() || src[idx] != '\"') return "";
  int start = idx + 1;
  int end = src.indexOf('\"', start);
  while(end > start && end != -1 && src[end-1] == '\\'){
    end = src.indexOf('\"', end + 1);
  }
  if(end < 0) return "";
  return src.substring(start, end);
}

static bool jsonExtractUint(const String& src, const char* key, uint32_t& value){
  String pattern = "\"" + String(key) + "\"";
  int idx = src.indexOf(pattern);
  if(idx < 0) return false;
  idx = src.indexOf(':', idx + pattern.length());
  if(idx < 0) return false;
  idx++;
  while(idx < (int)src.length() && (src[idx]==' ' || src[idx]=='\r' || src[idx]=='\n' || src[idx]=='\"')) idx++;
  if(idx >= (int)src.length()) return false;
  int end = idx;
  bool found = false;
  while(end < (int)src.length()){
    char c = src[end];
    if(c >= '0' && c <= '9'){
      found = true;
      end++;
      continue;
    }
    break;
  }
  if(!found) return false;
  String num = src.substring(idx, end);
  value = (uint32_t)num.toInt();
  return true;
}

static void manualAdvancePhase(){
  uint32_t now = millis();
  switch(phase){
    case PH_ABIERTO:
      phase = PH_FUNCION;
      estado = EST_EN_FUNCION;
      phaseStart = now;
      curtainOpenStart();
      break;
    case PH_FUNCION:
      phase = PH_ESPERA;
      estado = EST_ABIERTO;
      phaseStart = now;
      curtainCloseStart();
      break;
    case PH_ESPERA:
    default:
      phase = PH_ABIERTO;
      estado = EST_ABIERTO;
      phaseStart = now;
      break;
  }
  refreshStatusLed();
}

static void manualOpenCurtain(){
  curtainOpenStart();
  estado = EST_ABIERTO;
}

static void manualCloseCurtain(){
  curtainCloseStart();
  estado = EST_CERRADO;
}

static void manualToggleFans(){
  fansOn = !fansOn;
  digitalWrite(FAN1_PIN, fansOn ? HIGH : LOW);
  digitalWrite(FAN2_PIN, fansOn ? HIGH : LOW);
}
/* ===== Handlers HTTP ===== */
void handleRoot(){  server.send(200, "text/html", MAIN_page); }   // Usuario (Principal.h)
void handleAdmin(){ server.send(200, "text/html", ADMIN_page); }  // Admin   (Admin.h)
void handleStatus(){
  String body = jsonStatus();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", body);
}
void handleReset(){
  ingresos = 0;
  egresos  = 0;
  Serial.println("[ADMIN] Reset de aforo solicitado");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"ok\":true,\"action\":\"reset\"}");
  wsBroadcastStatus();
}

void handleControl(){
  server.sendHeader("Access-Control-Allow-Origin", "*");
  String body = server.hasArg("plain") ? server.arg("plain") : "";
  String action = jsonExtractString(body, "action");
  action.trim();

  if(action.length() == 0){
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"accion\"}");
    return;
  }

  bool handled = false;
  if(action == "avanzar"){
    manualAdvancePhase();
    handled = true;
  } else if(action == "abrir"){
    manualOpenCurtain();
    handled = true;
  } else if(action == "cerrar"){
    manualCloseCurtain();
    handled = true;
  } else if(action == "toggle-fans"){
    manualToggleFans();
    handled = true;
  }

  if(!handled){
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"accion_desconocida\"}");
    return;
  }

  wsBroadcastStatus();
  String resp = String("{\"ok\":true,\"action\":\"") + action + "\"}";
  server.send(200, "application/json", resp);
}

void handleConfig(){
  server.sendHeader("Access-Control-Allow-Origin", "*");
  String body = server.hasArg("plain") ? server.arg("plain") : "";
  bool updated = false;
  uint32_t value = 0;

  if(jsonExtractUint(body, "aforo_max", value) && value > 0){
    if(value > 1000) value = 1000;
    capacityMax = (uint16_t)value;
    updated = true;
  }

  if(jsonExtractUint(body, "dur_abierto_ms", value) && value >= 1000){
    if(value > 600000) value = 600000;
    durAbiertoMs = value;
    updated = true;
  }

  if(jsonExtractUint(body, "dur_funcion_ms", value) && value >= 1000){
    if(value > 600000) value = 600000;
    durFuncionMs = value;
    updated = true;
  }

  if(jsonExtractUint(body, "dur_espera_ms", value) && value >= 1000){
    if(value > 600000) value = 600000;
    durEsperaMs = value;
    updated = true;
  }

  if(!updated){
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"sin_parametros\"}");
    return;
  }

  wsBroadcastStatus();
  server.send(200, "application/json", "{\"ok\":true,\"action\":\"config\"}");
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
  Serial.begin(115200);

  // GPIO b??sicos
  pinMode(IR_PIN, INPUT);
  pinMode(M_IN1, OUTPUT); pinMode(M_IN2, OUTPUT);
  pinMode(M_IN3, OUTPUT); pinMode(M_IN4, OUTPUT);
  digitalWrite(M_IN1, LOW); digitalWrite(M_IN2, LOW);
  digitalWrite(M_IN3, LOW); digitalWrite(M_IN4, LOW);

  pinMode(FAN1_PIN, OUTPUT);
  pinMode(FAN2_PIN, OUTPUT);
  digitalWrite(FAN1_PIN, LOW);
  digitalWrite(FAN2_PIN, LOW);

  pinMode(STATUS_LED_R_PIN, OUTPUT);
  pinMode(STATUS_LED_G_PIN, OUTPUT);
  pinMode(STATUS_LED_B_PIN, OUTPUT);
  setStatusLed(false, false, false);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // LCD
  Wire.begin(21,22);
  Wire.setClock(100000);
  lcd.init();             // si tu lib no tiene init(), usar lcd.begin(16,2);
  // lcd.begin(16,2);
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Inicializando");
  lcd.setCursor(0,1); lcd.print("WiFi...");

  // DHT
  dht.begin();

  // RFID
  SPI.begin();                 // SCK=18, MISO=19, MOSI=23
  rfid.PCD_Init(SS_PIN, RST_PIN); // SDA=5, RST=4
  delay(50);

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a la red WiFi.");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0,0); lcd.print("IP:");
  lcd.setCursor(0,1); lcd.print(WiFi.localIP().toString());

  // Rutas HTTP
  server.on("/",         HTTP_GET,  handleRoot);    // Usuario
  server.on("/admin",    HTTP_GET,  handleAdmin);   // Admin
  server.on("/status",   HTTP_GET,  handleStatus);
  server.on("/reset",    HTTP_POST, handleReset);   // Reset aforo (principal)
  server.on("/reset",    HTTP_GET,  handleReset);   // Reset aforo (fallback)
  server.on("/reset",    HTTP_OPTIONS, handleOptions);
  server.on("/control", HTTP_POST, handleControl);
  server.on("/control", HTTP_OPTIONS, handleOptions);
  server.on("/config",  HTTP_POST, handleConfig);
  server.on("/config",  HTTP_OPTIONS, handleOptions);
  server.onNotFound(handleNotFound);

  server.begin();

  ws.begin();
  ws.onEvent(onWsEvent);

  // Iniciar ciclo
  cycleInit();
}

/* ========================== LOOP ========================== */
void loop() {
  server.handleClient();
  ws.loop();
  curtainTask();  // mueve tel??n SOLO en transiciones
  rfidTask();     // entradas (seg??n fase)
  readIR();       // salidas (siempre)
  tempTask();     // DHT + ventiladores (virtual) + log
  lcdTask();      // LCD con contador o Temp/Hum seg??n fase
  cycleTask();    // l??gica de fases
}

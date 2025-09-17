/********************* INCLUDES BÁSICOS *********************/
#include <WiFi.h>          // WiFi
#include <WebServer.h>     // Servidor HTTP
#include "Principal.h"     // Página de USUARIO (ruta "/")
#include "Admin.h"         // Página de ADMIN    (ruta "/admin")

const char* ssid     = "LABO";
const char* password = "";
WebServer server(80);
/************************************************************/

/* ===== Librerías de hardware ===== */
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

#define CAPACITY_MAX     40

// LCD I2C (cambia 0x27 por 0x3F si tu módulo usa esa dirección)
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

// Motor paso a paso (28BYJ-48 + ULN2003) – secuencia half-step
#define M_IN1 25
#define M_IN2 26
#define M_IN3 27
#define M_IN4 32

// Ventiladores (virtuales)
#define FAN1_PIN 12
#define FAN2_PIN 33

/* ===== Ventiladores con umbrales de test =====
   ON  a 26.0 °C
   OFF a 25.0 °C
   (histeresis 1 °C)
*/
#define TEMP_ON_FAN   26.0
#define TEMP_OFF_FAN  25.0

/* ===== Timings del ciclo =====
   ABIERTO  : 30 s  (telón cerrado)
   EN FUNCIÓN: 35 s (telón abre y queda abierto)
   ESPERA   : 30 s  (telón se cierra; salen)
*/
const uint32_t T_ABIERTO_MS  = 30000;
const uint32_t T_FUNCION_MS  = 35000;
const uint32_t T_ESPERA_MS   = 30000;

/* ===== Estado general ===== */
enum Estado { EST_ABIERTO, EST_EN_FUNCION, EST_CERRADO };
Estado estado = EST_ABIERTO;

/* ===== Fases del telón (orden ABIERTO → FUNCIÓN → ESPERA) ===== */
enum Phase { PH_ABIERTO, PH_FUNCION, PH_ESPERA };
Phase phase = PH_ABIERTO;
uint32_t phaseStart = 0;

volatile uint16_t ingresos = 0;  // suma RFID
volatile uint16_t egresos = 0;   // suma IR
float  temperaturaC = 0.0;
float  humedadRel   = 0.0;
bool   fansOn = false;

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

/* ===== IR – egresos (antirrebote + bloqueo post-RFID) ===== */
bool irLast = HIGH;         // según módulo puede ser activo-bajo
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
          // Serial.println("[IR] Egreso contabilizado");
        }
      } else {
        // Serial.println("[IR] Ignorado por bloqueo post-RFID");
      }
    }
  }
}

/* ===== RFID – ingresos (sin anti-repetición, pero condicionado por fase) ===== */
uint32_t lastRFIDMs = 0;
const uint16_t RFID_PERIOD_MS = 50;
void rfidTask(){
  if(millis()-lastRFIDMs < RFID_PERIOD_MS) return;
  lastRFIDMs = millis();
  if(!rfid.PICC_IsNewCardPresent()) return;
  if(!rfid.PICC_ReadCardSerial()) return;

  // Entradas habilitadas solo en ABIERTO y EN FUNCIÓN
  bool entradasHabilitadas = (phase == PH_ABIERTO) || (phase == PH_FUNCION);

  if(entradasHabilitadas && ingresos < CAPACITY_MAX){
    egresoBlockUntilMs = millis() + BLOCK_EGRESO_MS;   // bloqueo egreso 3s
    lcdMsgUntilMs = millis() + 3000;                   // mensaje 3s
    Serial.println("[RFID] Tarjeta detectada");
    ingresos++;
  } else {
    // Serial.println("[RFID] Entrada bloqueada (ESPERA/AFORO)");
  }

  rfid.PICC_HaltA(); rfid.PCD_StopCrypto1();
}

/* ===== DHT – lectura + ventiladores (virtuales) ===== */
uint32_t lastDHT = 0;
const uint32_t DHT_PERIOD_MS = 2000;
void tempTask(){
  if(millis()-lastDHT < DHT_PERIOD_MS) return;
  lastDHT = millis();

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if(!isnan(t)) temperaturaC = t;
  if(!isnan(h)) humedadRel   = h;

  // Log a serie (útil para ver thresholds)
  if(!isnan(t)){
    Serial.print("[TEMP] "); Serial.print(temperaturaC, 1);
    Serial.print(" C  [HUM] "); Serial.print(humedadRel, 0); Serial.println(" %");
  }

  // Ventiladores con nuevos umbrales de test (reflejado en /status)
  if(temperaturaC >= TEMP_ON_FAN && !fansOn){
    fansOn = true;
    digitalWrite(FAN1_PIN, HIGH); digitalWrite(FAN2_PIN, HIGH);
  } else if(temperaturaC < TEMP_OFF_FAN && fansOn){
    fansOn = false;
    digitalWrite(FAN1_PIN, LOW);  digitalWrite(FAN2_PIN, LOW);
  }
}

/* ===== Utilidades de tiempo de fase ===== */
uint32_t phaseDurationMs(){
  switch(phase){
    case PH_ABIERTO: return T_ABIERTO_MS;
    case PH_FUNCION: return T_FUNCION_MS;
    case PH_ESPERA:  return T_ESPERA_MS;
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

  // Línea 1: estado
  lcd.setCursor(0,0);
  char l0[17];
  if(phase==PH_FUNCION)      snprintf(l0, sizeof(l0), "EN FUNCION     ");
  else if(phase==PH_ESPERA)  snprintf(l0, sizeof(l0), "EN ESPERA      ");
  else /* PH_ABIERTO */      snprintf(l0, sizeof(l0), "ABIERTO        ");
  lcd.print(l0);

  // Línea 2: contador en FUNCION/ESPERA; T/H en ABIERTO
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

/* ===== Ocupación actual ===== */
uint16_t ocupacionActual(){
  int val = (int)ingresos - (int)egresos;
  if(val < 0) val = 0;
  if(val > CAPACITY_MAX) val = CAPACITY_MAX;
  return (uint16_t)val;
}

/* ===== Ciclo del telón (solo mover en cambios de fase) ===== */
void cycleInit(){
  phase = PH_ABIERTO;
  phaseStart = millis();
  estado = EST_ABIERTO;

  // Asegurar bobinas en reposo; NO mover el telón aquí.
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
        phase = PH_FUNCION;
        phaseStart = now;
        estado = EST_EN_FUNCION;
        curtainOpenStart(); // *** SOLO aquí abrimos el telón ***
      }
    }break;

    case PH_FUNCION:{
      estado = EST_EN_FUNCION;
      if(now - phaseStart >= T_FUNCION_MS){
        phase = PH_ESPERA;
        phaseStart = now;
        estado = EST_ABIERTO; // (legacy) la fase indica ESPERA
        curtainCloseStart();  // *** SOLO aquí cerramos el telón ***
      }
    }break;

    case PH_ESPERA:{
      estado = EST_ABIERTO;   // compat: estado visible “abierto”; fase=espera
      if(now - phaseStart >= T_ESPERA_MS){
        // Volvemos a ABIERTO; el telón ya quedó cerrado en el cambio anterior
        phase = PH_ABIERTO;
        phaseStart = now;
        estado = EST_ABIERTO;
        // NO mover telón aquí
      }
    }break;
  }
}

/* ===== JSON de estado ===== */
String jsonStatus(){
  uint16_t ocup = ocupacionActual();
  uint8_t pct = (uint8_t)round((ocup * 100.0) / CAPACITY_MAX);

  String s = "{";
  s += "\"estado\":\"" + String((phase==PH_FUNCION)?"en-funcion":"abierto") + "\","; // compat legacy
  s += "\"fase\":\"" + String(phaseName()) + "\",";         // "abierto" | "funcion" | "espera"
  s += "\"t_restante_ms\":" + String(phaseRemainingMs()) + ",";
  s += "\"temp\":" + String(temperaturaC, 1) + ",";
  s += "\"hum\":"  + String(humedadRel, 0) + ",";
  s += "\"ingresos\":" + String(ingresos) + ",";
  s += "\"egresos\":" + String(egresos) + ",";
  s += "\"aforo_actual\":" + String(ocup) + ",";
  s += "\"aforo_max\":" + String(CAPACITY_MAX) + ",";
  s += "\"aforo_pct\":" + String(pct) + ",";
  s += "\"fansOn\":" + String(fansOn ? "true" : "false");
  s += "}";
  return s;
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
}
void handleOptions(){ // CORS simple por si lo necesitás
  server.sendHeader("Access-Control-Allow-Origin","*");
  server.sendHeader("Access-Control-Allow-Methods","GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers","Content-Type");
  server.send(204);
}
void handleNotFound(){ server.send(404, "text/plain", "404"); }

/* ========================== SETUP ========================== */
void setup() {
  Serial.begin(115200);

  // GPIO básicos
  pinMode(IR_PIN, INPUT);
  pinMode(M_IN1, OUTPUT); pinMode(M_IN2, OUTPUT);
  pinMode(M_IN3, OUTPUT); pinMode(M_IN4, OUTPUT);
  digitalWrite(M_IN1, LOW); digitalWrite(M_IN2, LOW);
  digitalWrite(M_IN3, LOW); digitalWrite(M_IN4, LOW);

  pinMode(FAN1_PIN, OUTPUT);
  pinMode(FAN2_PIN, OUTPUT);
  digitalWrite(FAN1_PIN, LOW);
  digitalWrite(FAN2_PIN, LOW);

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
  server.onNotFound(handleNotFound);

  server.begin();

  // Iniciar ciclo
  cycleInit();
}

/* ========================== LOOP ========================== */
void loop() {
  server.handleClient();
  curtainTask();  // mueve telón SOLO en transiciones
  rfidTask();     // entradas (según fase)
  readIR();       // salidas (siempre)
  tempTask();     // DHT + ventiladores (virtual) + log
  lcdTask();      // LCD con contador o Temp/Hum según fase
  cycleTask();    // lógica de fases
}

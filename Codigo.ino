/********************* INCLUDES BÁSICOS (tu bloque requerido) *********************/
#include <WiFi.h>          // Incluye la biblioteca WiFi para manejar la conexión a la red.
#include <WebServer.h>     // Incluye la biblioteca WebServer para crear un servidor web.
#include "Principal.h"     // Incluye el archivo que contiene el código HTML de la página principal. 

const char* ssid     = "LABO";  // Define el nombre de la red WiFi a la que se conectará.
const char* password = "";      // Define la contraseña de la red WiFi.
WebServer server(80);           // Crea un objeto para el servidor web en el puerto 80.
/*******************************************************************************/

/* ===== Resto de librerías de hardware ===== */
#include <SPI.h>
#include <MFRC522.h>            // RFID RC522
#include <Wire.h>
#include <LiquidCrystal_I2C.h>  // LCD I2C
#include <DHT.h>                // DHT11

/********** CONSTANTES DE HARDWARE **********
 LCD I2C: VCC=5V, GND, SDA=GPIO21, SCL=GPIO22
 RFID RC522: VCC=3.3V, GND, RST=GPIO4, SDA=GPIO5, SCK=18, MOSI=23, MISO=19
 Sensor IR (egreso): OUT=GPIO34
 DHT11: DATA=GPIO14
 Motor paso a paso (28BYJ-48 + ULN2003): IN1=GPIO25, IN2=GPIO26, IN3=GPIO27, IN4=GPIO32
 Ventiladores (relevador o MOSFET): FAN1=GPIO12, FAN2=GPIO33
 ********************************************/

#define CAPACITY_MAX     40

// LCD I2C (dirección típica 0x27; cambiar a 0x3F si tu módulo usa esa)
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

// ======== LED RGB (DESHABILITADO) ========
// Dejo TODO comentado para que no influya hasta que lo montes.
// #define PIN_R     15
// #define PIN_G      2
// #define PIN_B     13
// #define CH_R       0
// #define CH_G       1
// #define CH_B       2
// #define PWM_FREQ 5000
// #define PWM_RES     8
// void setRGB(uint8_t r, uint8_t g, uint8_t b) {
//   ledcWrite(CH_R, r);
//   ledcWrite(CH_G, g);
//   ledcWrite(CH_B, b);
// }
// void updateRGB(){
//   // Cambiar color según estado (verde/amarillo/rojo)
// }
// =========================================

// Motor paso a paso (cortina) – 28BYJ-48 con ULN2003 (half-step)
#define M_IN1 25
#define M_IN2 26
#define M_IN3 27
#define M_IN4 32

// Ventiladores
#define FAN1_PIN 12
#define FAN2_PIN 33

// Umbrales temperatura para ventiladores (histeresis)
#define TEMP_ON_FAN   33.0
#define TEMP_OFF_FAN  31.0

// Timings del ciclo automático
const uint32_t T_INGRESO_MS = 5000;
const uint32_t T_FUNCION_MS = 10000;
const uint32_t T_RETIRO_MS  = 5000;

/* ===== Estado del sistema ===== */
enum Estado { EST_ABIERTO, EST_EN_FUNCION, EST_CERRADO };
Estado estado = EST_ABIERTO;

volatile uint16_t ingresos = 0;  // suma RFID
volatile uint16_t egresos = 0;   // suma IR
float temperaturaC = 0.0;
bool fansOn = false;
String lcdLine = "Bienvenidos";
String lcdLine2 = "Anfiteatro";

/* ===== Cortina (stepper) ===== */
const uint8_t halfStep[8][4] = {
  {1,0,0,0},
  {1,1,0,0},
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1},
  {1,0,0,1}
};
const uint16_t OPEN_STEPS = 2048;      // ajustar a tu carrera real
const uint16_t STEP_INTERVAL_MS = 3;   // velocidad de paso

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
void curtainOpenStart(){
  curtState = CURT_OPENING;
  curtStepsRemaining = OPEN_STEPS;
}
void curtainCloseStart(){
  curtState = CURT_CLOSING;
  curtStepsRemaining = OPEN_STEPS;
}
void curtainTask(){
  if(curtState==CURT_IDLE) return;
  if(millis()-lastStepMs < STEP_INTERVAL_MS) return;
  lastStepMs = millis();

  if(curtState==CURT_OPENING){
    stepIndex = (stepIndex + 1) & 0x07;
    motorWriteStep(stepIndex);
  } else { // CLOSING
    stepIndex = (stepIndex + 7) & 0x07;
    motorWriteStep(stepIndex);
  }
  if(curtStepsRemaining>0) curtStepsRemaining--;
  if(curtStepsRemaining==0){
    curtState = CURT_IDLE;
    // desenergizar bobinas
    digitalWrite(M_IN1, LOW);
    digitalWrite(M_IN2, LOW);
    digitalWrite(M_IN3, LOW);
    digitalWrite(M_IN4, LOW);
  }
}

/* ===== IR – egresos (antirrebote) ===== */
bool irLast = HIGH;         // segun módulo puede ser activo-bajo
uint32_t irLastChange = 0;
const uint16_t IR_DEBOUNCE_MS = 50;
void readIR(){
  int v = digitalRead(IR_PIN);
  uint32_t now = millis();
  if(v != irLast && (now - irLastChange) > IR_DEBOUNCE_MS){
    irLastChange = now;
    irLast = v;
    // si es activo-bajo, contar en flanco a LOW:
    if(v == LOW){
      if(egresos < ingresos) egresos++;
    }
  }
}

/* ===== RFID – ingresos (antirepetición) ===== */
uint32_t lastRFIDMs = 0;
const uint16_t RFID_PERIOD_MS = 50;
uint8_t lastUID[10] = {0};
byte lastUIDLen = 0;
uint32_t lastUIDTime = 0;

bool sameUID(MFRC522::Uid &uid){
  if(uid.size != lastUIDLen) return false;
  for(byte i=0;i<uid.size;i++) if(uid.uidByte[i] != lastUID[i]) return false;
  return true;
}
void rememberUID(MFRC522::Uid &uid){
  lastUIDLen = uid.size;
  for(byte i=0;i<uid.size;i++) lastUID[i] = uid.uidByte[i];
  lastUIDTime = millis();
}
void rfidTask(){
  if(millis()-lastRFIDMs < RFID_PERIOD_MS) return;
  lastRFIDMs = millis();
  if(!rfid.PICC_IsNewCardPresent()) return;
  if(!rfid.PICC_ReadCardSerial()) return;

  if(sameUID(rfid.uid) && millis()-lastUIDTime < 2000){
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }
  rememberUID(rfid.uid);
  if(ingresos < CAPACITY_MAX) ingresos++;

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

/* ===== DHT – temperatura + ventiladores ===== */
uint32_t lastDHT = 0;
const uint32_t DHT_PERIOD_MS = 2000;
void tempTask(){
  if(millis()-lastDHT < DHT_PERIOD_MS) return;
  lastDHT = millis();
  float t = dht.readTemperature();
  if(!isnan(t)) temperaturaC = t;

  if(temperaturaC >= TEMP_ON_FAN && !fansOn){
    fansOn = true;
    digitalWrite(FAN1_PIN, HIGH);
    digitalWrite(FAN2_PIN, HIGH);
  } else if(temperaturaC < TEMP_OFF_FAN && fansOn){
    fansOn = false;
    digitalWrite(FAN1_PIN, LOW);
    digitalWrite(FAN2_PIN, LOW);
  }
}

/* ===== LCD ===== */
uint32_t lastLCD = 0;
void lcdTask(){
  if(millis()-lastLCD < 250) return;
  lastLCD = millis();
  lcd.setCursor(0,0);
  char l0[17];
  if(estado==EST_EN_FUNCION) snprintf(l0, sizeof(l0), "EN FUNCION     ");
  else if(estado==EST_ABIERTO) snprintf(l0, sizeof(l0), "ABIERTO        ");
  else snprintf(l0, sizeof(l0), "CERRADO        ");
  lcd.print(l0);

  lcd.setCursor(0,1);
  char l1[17];
  uint16_t ocup = (uint16_t)max(0, min((int)CAPACITY_MAX, (int)ingresos - (int)egresos));
  snprintf(l1, sizeof(l1), "T:%2.0fC IN:%02u", temperaturaC, (unsigned)ocup);
  lcd.print(l1);
}

/* ===== Ciclo automático 5-10-5 ===== */
enum Phase { PH_INGRESO, PH_FUNCION, PH_RETIRO };
Phase phase = PH_INGRESO;
uint32_t phaseStart = 0;
uint16_t targetOcup = 0;

uint16_t ocupacionActual(){
  int val = (int)ingresos - (int)egresos;
  if(val < 0) val = 0;
  if(val > CAPACITY_MAX) val = CAPACITY_MAX;
  return (uint16_t)val;
}
void pickTarget(){
  // objetivo entre 10 y 40 personas
  targetOcup = 10 + (uint16_t)random(31); // 10..40 (31 valores: 0..30)
  if(targetOcup > CAPACITY_MAX) targetOcup = CAPACITY_MAX;
}
void cycleInit(){
  phase = PH_INGRESO;
  phaseStart = millis();
  ingresos = 0; egresos = 0;
  pickTarget();
  estado = EST_ABIERTO;
  curtainCloseStart(); // cerrada al inicio
}
void cycleTask(){
  uint32_t now = millis();
  switch(phase){
    case PH_INGRESO:{
      estado = EST_ABIERTO;
      // Llenar hacia target durante T_INGRESO_MS
      static uint32_t lastIn = 0;
      uint16_t ocup = ocupacionActual();
      if(ocup < targetOcup){
        if(now - lastIn > 200 && ingresos < CAPACITY_MAX){
          ingresos++;
          lastIn = now;
        }
      }
      if(now - phaseStart >= T_INGRESO_MS){
        phase = PH_FUNCION; phaseStart = now;
        estado = EST_EN_FUNCION;
        curtainOpenStart(); // abrir cortina
      }
    }break;

    case PH_FUNCION:{
      estado = EST_EN_FUNCION;
      if(now - phaseStart >= T_FUNCION_MS){
        phase = PH_RETIRO; phaseStart = now;
        estado = EST_ABIERTO;
        curtainCloseStart(); // cerrar cortina
      }
    }break;

    case PH_RETIRO:{
      estado = EST_ABIERTO;
      // Vaciar a cero en T_RETIRO_MS
      static uint32_t lastOut = 0;
      if(now - lastOut > 150 && egresos < ingresos){
        egresos++;
        lastOut = now;
      }
      if(now - phaseStart >= T_RETIRO_MS){
        cycleInit(); // reiniciar ciclo
      }
    }break;
  }
}

/* ===== HTTP – Handlers ===== */
String jsonStatus(){
  uint16_t ocup = ocupacionActual();
  uint8_t pct = (uint8_t)round((ocup * 100.0) / CAPACITY_MAX);
  String st = (estado==EST_EN_FUNCION) ? "en-funcion" : (estado==EST_ABIERTO ? "abierto" : "cerrado");

  // Si querés, podés incluir horarios aquí como arreglo JSON.
  // Ejemplo: "horarios":[{"titulo":"Apertura","hora":"18:00"}, ...]
  String s = "{";
  s += "\"estado\":\"" + st + "\",";
  s += "\"temp\":" + String(temperaturaC, 1) + ",";
  s += "\"ingresos\":" + String(ingresos) + ",";
  s += "\"egresos\":" + String(egresos) + ",";
  s += "\"aforo_actual\":" + String(ocup) + ",";
  s += "\"aforo_max\":" + String(CAPACITY_MAX) + ",";
  s += "\"aforo_pct\":" + String(pct) + ",";
  s += "\"fansOn\":" + String(fansOn ? "true" : "false") + ",";
  s += "\"lcd\":\"" + lcdLine + "\"";
  s += "}";
  return s;
}

void handleRoot(){
  server.send(200, "text/html", MAIN_page); // MAIN_page viene de Principal.h
}
void handleStatus(){
  String body = jsonStatus();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", body);
}
void handleOpen(){
  curtainOpenStart();
  server.send(200, "application/json", "{\"ok\":true,\"action\":\"open\"}");
}
void handleClose(){
  curtainCloseStart();
  server.send(200, "application/json", "{\"ok\":true,\"action\":\"close\"}");
}
void handleLCD(){
  if(server.hasArg("text")){
    lcdLine = server.arg("text");
  }
  server.send(200, "application/json", "{\"ok\":true}");
}
void handleReset(){
  ingresos = egresos = 0;
  server.send(200, "application/json", "{\"ok\":true,\"action\":\"reset\"}");
}
void handleNotFound(){
  server.send(404, "text/plain", "404");
}

/* ========================== SETUP ========================== */
void setup() {
  Serial.begin(115200);

  // GPIO básicos
  pinMode(IR_PIN, INPUT);
  pinMode(M_IN1, OUTPUT);
  pinMode(M_IN2, OUTPUT);
  pinMode(M_IN3, OUTPUT);
  pinMode(M_IN4, OUTPUT);
  digitalWrite(M_IN1, LOW); digitalWrite(M_IN2, LOW);
  digitalWrite(M_IN3, LOW); digitalWrite(M_IN4, LOW);

  pinMode(FAN1_PIN, OUTPUT);
  pinMode(FAN2_PIN, OUTPUT);
  digitalWrite(FAN1_PIN, LOW);
  digitalWrite(FAN2_PIN, LOW);

  // ===== RGB deshabilitado =====
  // // PWM RGB (LEDC)
  // ledcSetup(CH_R, PWM_FREQ, PWM_RES); ledcAttachPin(PIN_R, CH_R);
  // ledcSetup(CH_G, PWM_FREQ, PWM_RES); ledcAttachPin(PIN_G, CH_G);
  // ledcSetup(CH_B, PWM_FREQ, PWM_RES); ledcAttachPin(PIN_B, CH_B);
  // updateRGB();

  // LCD
  Wire.begin(); // SDA=21, SCL=22 por defecto ESP32
  lcd.init();
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

  // Conexión a la red WiFi
  WiFi.begin(ssid, password); // Inicia la conexión a la red WiFi con el SSID y la contraseña especificados.
  while (WiFi.status() != WL_CONNECTED) { // Espera a que se establezca la conexión WiFi.
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a la red WiFi.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0,0); lcd.print("IP:");
  lcd.setCursor(0,1); lcd.print(WiFi.localIP().toString());

  // Rutas HTTP
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/curtain/open", HTTP_POST, handleOpen);
  server.on("/curtain/close", HTTP_POST, handleClose);
  server.on("/lcd", HTTP_POST, handleLCD);
  server.on("/reset", HTTP_POST, handleReset);
  server.onNotFound(handleNotFound);

  // Inicia el servidor web
  server.begin();

  // Semilla para random (por si usás A0 libre; si no, comentar)
  // randomSeed(analogRead(0));

  // Inicia ciclo automático
  cycleInit();
}

/* ========================== LOOP ========================== */
void loop() {
  server.handleClient();
  curtainTask();
  rfidTask();
  readIR();
  tempTask();
  lcdTask();
  cycleTask();

  // ===== RGB deshabilitado =====
  // updateRGB();
}

#include <WiFi.h>       // Para manejar conexión WiFi
#include <WebServer.h>  // Crear servidor web
#include "GENERAL.h"    // Página principal
#include "ADMIN.h"      // Página admin

// Librerías de hardware
#include <Wire.h>              // I2C (LCD)
#include <LiquidCrystal_I2C.h> // LCD 16x2
#include <SPI.h>               // SPI (RFID)
#include <MFRC522.h>           // RFID
#include <DHT.h>               // Sensor de temperatura

// Pines
#define DHTPIN 15
#define DHTTYPE DHT11

#define RFID_SS 5
#define RFID_RST 27

#define FC51_PIN 4

#define LED_R 12
#define LED_G 13
#define LED_B 14

#define STEPPER_IN1 32
#define STEPPER_IN2 33
#define STEPPER_IN3 25
#define STEPPER_IN4 26

// Objetos de librerías
DHT dht(DHTPIN, DHTTYPE);
MFRC522 rfid(RFID_SS, RFID_RST);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// WiFi
const char* ssid = "LABO"; 
const char* password = ""; 
WebServer server(80); 

// --- FUNCIONES DE INICIALIZACIÓN ---
void initDHT11() {
  dht.begin();
  Serial.println("DHT11 inicializado.");
}

void initRFID() {
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("RFID inicializado.");
}

void initFC51() {
  pinMode(FC51_PIN, INPUT);
  Serial.println("Sensor infrarrojo FC-51 inicializado.");
}

void initLCD() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LCD inicializado");
  Serial.println("LCD 16x2 inicializado.");
}

void initStepper() {
  pinMode(STEPPER_IN1, OUTPUT);
  pinMode(STEPPER_IN2, OUTPUT);
  pinMode(STEPPER_IN3, OUTPUT);
  pinMode(STEPPER_IN4, OUTPUT);
  Serial.println("Motor inicializado.");
}

void initLEDs() {
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);
  Serial.println("RGB inicializado.");
}

// --- FUNCIONES VACÍAS ---
void checkDHT11() {}
void checkRFID() {}
void checkFC51() {}
void updateLCD() {}
void moveStepper() {}
void setLEDState(int state) {}

// --- MANEJO DE RUTAS WEB ---
void handleRoot() {
  server.send(200, "text/html", GENERAL);
}

void handleAdmin() {
  server.send(200, "text/html", ADMIN);
}

void handleAbrir() {
  Serial.println("Accion: Abrir Telón");
  server.send(200, "text/plain", "Telón abierto (simulado)");
}

void handleCerrar() {
  Serial.println("Accion: Cerrar Telón");
  server.send(200, "text/plain", "Telón cerrado (simulado)");
}

// --- CONFIGURACIÓN INICIAL ---
void setup() {
  Serial.begin(115200);

  // Conexión WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a la red WiFi.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Iniciar módulos
  initDHT11();
  initRFID();
  initFC51();
  initLCD();
  initStepper();
  initLEDs();

  // Rutas del servidor
  server.on("/", HTTP_GET, handleRoot);
  server.on("/admin", HTTP_GET, handleAdmin);
  server.on("/abrir", HTTP_GET, handleAbrir);
  server.on("/cerrar", HTTP_GET, handleCerrar);

  server.begin();
}

// --- BUCLE PRINCIPAL ---
void loop() {
  server.handleClient();

  checkDHT11();
  checkRFID();
  checkFC51();
  updateLCD();
  delay(200);
}

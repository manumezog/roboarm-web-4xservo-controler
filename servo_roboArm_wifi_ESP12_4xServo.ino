#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// --- CONFIGURACIÓN DE RED ---
const char* ssid = "";
const char* password = "";

ESP8266WebServer server(80);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// --- CALIBRACIÓN DE SERVOS ---
// MG996R (Puertos 0, 1, 2)
#define MG996_MIN  102 
#define MG996_MAX  512 
// SG90 (Puerto 3)
#define SG90_MIN   120 
#define SG90_MAX   480 

// --- VARIABLES DE ESTADO ---
float currentAngles[4] = {90, 90, 90, 90};
int targetAngles[4] = {90, 90, 90, 90};
unsigned long lastMoveTime = 0;
int globalStepDelay = 15; // Delay entre pasos (ms)
bool emergencyStop = false;

// --- LÓGICA DE MOVIMIENTO SUAVE (ASÍNCRONA) ---
void updateServos() {
  if (emergencyStop) return;

  if (millis() - lastMoveTime >= (unsigned long)globalStepDelay) {
    lastMoveTime = millis();
    bool moving = false;

    for (int i = 0; i < 4; i++) {
      if (abs(currentAngles[i] - targetAngles[i]) > 0.5) {
        moving = true;
        if (currentAngles[i] < targetAngles[i]) currentAngles[i] += 1.0;
        else currentAngles[i] -= 1.0;

        int pulse = (i == 3) ? 
                    map((int)currentAngles[i], 0, 180, SG90_MIN, SG90_MAX) : 
                    map((int)currentAngles[i], 0, 180, MG996_MIN, MG996_MAX);
        
        pwm.setPWM(i, 0, pulse);
      }
    }
  }
}

// --- MANEJADORES DEL SERVIDOR WEB ---
void handleRoot() {
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1' charset='utf-8'>";
  html += "<style>body{font-family:'Segoe UI',sans-serif; text-align:center; background:#eceff1; padding:20px; color:#333;}";
  html += ".card{background:white; padding:15px; border-radius:15px; margin:10px auto; max-width:500px; box-shadow:0 4px 6px rgba(0,0,0,0.1);}";
  html += ".control-group{display:flex; align-items:center; gap:10px; margin-top:10px;}";
  html += ".slider{flex-grow:1; height:15px; border-radius:5px; background:#d7ccc8; outline:none;}";
  html += ".btn-small{padding:10px; border:none; border-radius:8px; background:#455a64; color:white; font-weight:bold; cursor:pointer; min-width:55px;}";
  html += ".btn-panic{background:#d32f2f; width:100%; padding:15px; color:white; border:none; border-radius:10px; font-weight:bold; margin-bottom:10px;}";
  html += ".btn-action{background:#1976d2; width:48%; padding:10px; color:white; border:none; border-radius:10px; font-weight:bold;}";
  html += "h3{margin:5px 0; font-size:16px; color:#455a64;}</style></head><body>";
  
  html += "<h1>Control Brazo PETG</h1>";

  // Panel de Seguridad
  html += "<div class='card'>";
  html += "<button class='btn-panic' onclick='fetch(\"/panic\")'>PARADA DE EMERGENCIA</button>";
  html += "<div style='display:flex; justify-content:space-between;'>";
  html += "<button class='btn-action' style='background:#388e3c' onclick='fetch(\"/reset_safety\")'>ACTIVAR MOTORES</button>";
  html += "<button class='btn-action' onclick='centerJS()'>CENTRAR TODO</button></div></div>";

  // Slider de Velocidad (Invertido lógicamente: derecha = más rápido)
  int speedVal = 101 - globalStepDelay;
  html += "<div class='card' style='background:#37474f; color:white;'><h3>Velocidad de Movimiento: <span id='vSpd'>" + String(speedVal) + "</span></h3>";
  html += "<input type='range' min='1' max='100' value='" + String(speedVal) + "' class='slider' onchange='updateSpeed(this.value)'></div>";

  // Sliders de los 4 Servos
  const char* names[] = {"Base (P0)", "Codo (P1)", "Hombro (P2)", "Pinza (P3)"};
  for(int i=0; i<4; i++) {
    html += "<div class='card'><h3>" + String(names[i]) + ": <span id='v" + String(i) + "'>" + String((int)currentAngles[i]) + "</span>&deg;</h3>";
    html += "<div class='control-group'>";
    html += "<button class='btn-small' onclick='moveServo(" + String(i) + ", 0)'>0&deg;</button>";
    html += "<input type='range' min='0' max='180' value='" + String((int)currentAngles[i]) + "' class='slider' id='s" + String(i) + "' oninput='moveServo(" + String(i) + ", this.value)'>";
    html += "<button class='btn-small' onclick='moveServo(" + String(i) + ", 180)'>180&deg;</button>";
    html += "</div></div>";
  }

  html += "<script>";
  html += "let lastCall = 0;";
  html += "function moveServo(port, val) {";
  html += "  document.getElementById('v' + port).innerHTML = val;";
  html += "  document.getElementById('s' + port).value = val;";
  html += "  let now = Date.now(); if (now - lastCall > 40) {";
  html += "    fetch('/setAngle?port=' + port + '&deg=' + val); lastCall = now; } }";
  html += "function updateSpeed(val) { document.getElementById('vSpd').innerHTML = val; fetch('/setSpeed?spd=' + val); }";
  html += "function centerJS() { for(let i=0; i<4; i++){ moveServo(i, 90); } fetch('/center'); }";
  html += "</script></body></html>";
  
  server.send(200, "text/html", html);
}

void handleAngle() {
  if (!emergencyStop && server.hasArg("port") && server.hasArg("deg")) {
    int p = server.arg("port").toInt();
    int d = server.arg("deg").toInt();
    if (p >= 0 && p < 4) {
      targetAngles[p] = d;
      Serial.printf("WEB: Puerto %d -> Objetivo %d°\n", p, d);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("spd")) {
    int s = server.arg("spd").toInt();
    globalStepDelay = 101 - s; // Inversión lógica
    Serial.printf("WEB: Velocidad ajustada. Delay: %d ms\n", globalStepDelay);
    server.send(200, "text/plain", "OK");
  }
}

void handlePanic() {
  emergencyStop = true;
  for(int i=0; i<16; i++) pwm.setPWM(i, 0, 4096); // Corta señal PWM
  Serial.println("!!! EMERGENCIA: MOTORES DESACTIVADOS !!!");
  server.send(200, "text/plain", "STOP");
}

void handleResetSafety() {
  emergencyStop = false;
  Serial.println("SEGURIDAD: Motores re-activados.");
  server.send(200, "text/plain", "OK");
}

void handleCenter() {
  if (!emergencyStop) {
    for(int i=0; i<4; i++) targetAngles[i] = 90;
    Serial.println("WEB: Centrando todos los ejes.");
  }
  server.send(200, "text/plain", "OK");
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n--- CONTROL BRAZO PETG v2.0 ---");

  pwm.begin();
  pwm.setPWMFreq(50);
  Serial.println("Driver PCA9685 OK");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi CONECTADO");
  Serial.print("IP del Brazo: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/setAngle", handleAngle);
  server.on("/setSpeed", handleSpeed);
  server.on("/panic", handlePanic);
  server.on("/reset_safety", handleResetSafety);
  server.on("/center", handleCenter);
  
  server.begin();
  Serial.println("Servidor Web listo.");
}

// --- LOOP ---
void loop() {
  server.handleClient();
  updateServos();
}

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

const char* ssid = "DIGIFIBRA-86AC";
const char* password = "yN4+rX8pKQ";

ESP8266WebServer server(80);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Calibración MG996R (Puertos 0, 1, 2)
#define MG996_MIN  102 
#define MG996_MAX  512 

// Calibración SG90 (Puerto 3) 
// El SG90 suele ser feliz entre 120 y 480 para evitar forzar el plástico
#define SG90_MIN   120 
#define SG90_MAX   480 

int currentAngles[4] = {90, 90, 90, 90};
int globalStepDelay = 15; 
bool emergencyStop = false;

void moveServoSlow(int port, int targetAngle) {
  if (emergencyStop) return;
  int startAngle = currentAngles[port];
  if (targetAngle == startAngle) return;
  int step = (targetAngle > startAngle) ? 1 : -1;
  
  for (int pos = startAngle; pos != targetAngle + step; pos += step) {
    if (emergencyStop) break;
    
    // Mapeo diferenciado: Puertos 0-2 usan límites MG996, Puerto 3 usa SG90
    int pulse = (port == 3) ? map(pos, 0, 180, SG90_MIN, SG90_MAX) : map(pos, 0, 180, MG996_MIN, MG996_MAX);
    
    pwm.setPWM(port, 0, pulse);
    delay(globalStepDelay); 
    server.handleClient();
  }
  currentAngles[port] = targetAngle;
}

void handleRoot() {
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:'Segoe UI',sans-serif; text-align:center; background:#eceff1; padding:20px; color:#333;}";
  html += ".card{background:white; padding:20px; border-radius:15px; margin:15px auto; max-width:450px; box-shadow:0 4px 6px rgba(0,0,0,0.1);}";
  html += ".slider{width:100%; height:15px; border-radius:5px; background:#d7ccc8; outline:none; margin:15px 0;}";
  html += ".btn-container{display:flex; flex-direction:column; gap:10px; align-items:center;}";
  html += ".btn{width:100%; max-width:400px; padding:15px; border:none; border-radius:10px; color:white; font-size:16px; font-weight:bold; cursor:pointer;}";
  html += ".btn-panic{background:#d32f2f;} .btn-center{background:#1976d2;} .btn-on{background:#388e3c;}";
  html += "h3{margin:5px 0; font-size:18px; color:#455a64;}</style></head><body>";
  
  html += "<h1>PETG Arm Controller</h1>";

  // Panel de Botones (Fijos, sin recarga de página)
  html += "<div class='card btn-container'>";
  html += "<button class='btn btn-panic' onclick='fetch(\"/panic\")'>PARADA DE EMERGENCIA</button>";
  html += "<button class='btn btn-on' onclick='fetch(\"/reset_safety\")'>ACTIVAR MOTORES</button>";
  html += "<button class='btn btn-center' onclick='centerJS()'>CENTRAR TODO (90&deg;)</button>";
  html += "</div>";

  // Control de Velocidad
  html += "<div class='card' style='background:#37474f; color:white;'><h3>Velocidad: <span id='vSpd'>" + String(globalStepDelay) + "</span>ms</h3>";
  html += "<input type='range' min='1' max='100' value='" + String(globalStepDelay) + "' class='slider' onchange='updateSpeed(this.value)'></div>";

  // Sliders de Servos
  for(int i=0; i<4; i++) {
    String label = (i==0)?"Base (P0)":(i==1)?"Codo (P1)":(i==2)?"Hombro (P2)":"Pinza SG90 (P3)";
    html += "<div class='card'><h3>" + label + ": <span id='v" + String(i) + "'>" + String(currentAngles[i]) + "</span>&deg;</h3>";
    html += "<input type='range' min='0' max='180' value='" + String(currentAngles[i]) + "' class='slider' id='s" + String(i) + "' onchange='moveServo(" + String(i) + ", this.value)'></div>";
  }

  html += "<script>";
  html += "function moveServo(port, val) { document.getElementById('v' + port).innerHTML = val; fetch('/setAngle?port=' + port + '&deg=' + val); }";
  html += "function updateSpeed(val) { document.getElementById('vSpd').innerHTML = val; fetch('/setSpeed?ms=' + val); }";
  html += "function centerJS() { for(let i=0; i<4; i++){ document.getElementById('v'+i).innerHTML=90; document.getElementById('s'+i).value=90; } fetch('/center'); }";
  html += "</script></body></html>";
  
  server.send(200, "text/html", html);
}

void handlePanic() {
  emergencyStop = true;
  for(int i=0; i<16; i++) pwm.setPWM(i, 0, 4096); 
  server.send(200, "text/plain", "STOPPED");
}

void handleResetSafety() {
  emergencyStop = false;
  for(int i=0; i<4; i++) {
    int pulse = (i==3) ? map(currentAngles[i], 0, 180, SG90_MIN, SG90_MAX) : map(currentAngles[i], 0, 180, MG996_MIN, MG996_MAX);
    pwm.setPWM(i, 0, pulse);
  }
  server.send(200, "text/plain", "MOTORS_ON");
}

void handleAngle() {
  if (!emergencyStop && server.hasArg("port") && server.hasArg("deg")) {
    moveServoSlow(server.arg("port").toInt(), server.arg("deg").toInt());
  }
  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("ms")) {
    globalStepDelay = server.arg("ms").toInt();
    server.send(200, "text/plain", "OK");
  }
}

void handleCenter() {
  if (!emergencyStop) {
    for(int i=0; i<4; i++) moveServoSlow(i, 90);
  }
  server.send(200, "text/plain", "CENTERED");
}

void setup() {
  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(50);
  
  // Posición inicial (90 grados para todos)
  for(int i=0; i<4; i++) {
    int pulse = (i==3) ? map(90, 0, 180, SG90_MIN, SG90_MAX) : map(90, 0, 180, MG996_MIN, MG996_MAX);
    pwm.setPWM(i, 0, pulse);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  
  server.on("/", handleRoot);
  server.on("/setAngle", handleAngle);
  server.on("/setSpeed", handleSpeed);
  server.on("/panic", handlePanic);
  server.on("/reset_safety", handleResetSafety);
  server.on("/center", handleCenter);
  server.begin();
}

void loop() {
  server.handleClient();
}
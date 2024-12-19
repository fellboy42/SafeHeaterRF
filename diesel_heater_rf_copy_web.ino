#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "DieselHeaterRF.h"

// WiFi Credentials
const char *ssid = "stevennet4g";
const char *password = "12222111";

// Web server and WebSocket server
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Diesel Heater
#define HEATER_POLL_INTERVAL 4000
#define HEATER_CMD_START  0x01  // Replace with actual command
#define HEATER_CMD_STOP   0x02  // Replace with actual command

uint32_t heaterAddr;
DieselHeaterRF heater;
heater_state_t state;

// Function Prototypes
void handleRoot();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
String buildStatusJSON();
void setTemperature(int temp);

void setup() {
  Serial.begin(115200);

  // Start WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Start Web server
  server.on("/", handleRoot);
  server.begin();

  // Initialize heater
  heater.begin();
  Serial.println("Started pairing, press and hold the pairing button on the heater's LCD panel...");
  heaterAddr = heater.findAddress(60000UL);

  if (heaterAddr) {
    Serial.print("Got address: ");
    Serial.println(heaterAddr, HEX);
    heater.setAddress(heaterAddr);
  } else {
    Serial.println("Failed to find a heater");
    while (1) {}  // Stop here
  }
}

void loop() {
  server.handleClient();
  webSocket.loop();

  // Poll heater state and send updates
  if (heater.getState(&state)) {
    String json = buildStatusJSON();
    webSocket.broadcastTXT(json);
  } else {
    Serial.println("Failed to get the state");
  }
  
  delay(HEATER_POLL_INTERVAL);
}

void handleRoot() {
  server.send(200, "text/html", 
    "<html>"
    "<head>"
    "<script>"
    "let socket = new WebSocket('ws://' + location.hostname + ':81');"
    "socket.onmessage = function(event) { console.log(JSON.parse(event.data)); };"
    "function sendCommand(cmd, value = 0) {"
    "  let message = JSON.stringify({ command: cmd, value: value });"
    "  socket.send(message);"
    "}"
    "</script>"
    "</head>"
    "<body>"
    "<h1>Diesel Heater Control</h1>"
    "<button onclick=\"sendCommand('start')\">Start Heater</button>"
    "<button onclick=\"sendCommand('stop')\">Stop Heater</button>"
    "<input type=\"range\" min=\"10\" max=\"30\" step=\"1\" onchange=\"sendCommand('setTemp', this.value)\">"
    "</body>"
    "</html>"
  );
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_TEXT) {
    String message = String((char *)payload);
    DynamicJsonDocument doc(256);

    DeserializationError error = deserializeJson(doc, message);
    if (error) {
      Serial.print("JSON Deserialization failed: ");
      Serial.println(error.c_str());
      return;
    }

    String command = doc["command"];
    if (command == "start") {
      heater.sendCommand(HEATER_CMD_START);
    } else if (command == "stop") {
      heater.sendCommand(HEATER_CMD_STOP);
    } else if (command == "setTemp") {
      int temperature = doc["value"];
      setTemperature(temperature);
    }
  }
}

String buildStatusJSON() {
  String json = "{";
  json += "\"state\":" + String(state.state) + ",";
  json += "\"power\":" + String(state.power) + ",";
  json += "\"voltage\":" + String(state.voltage) + ",";
  json += "\"ambientTemp\":" + String(state.ambientTemp) + ",";
  json += "\"caseTemp\":" + String(state.caseTemp) + ",";
  json += "\"setpoint\":" + String(state.setpoint) + ",";
  json += "\"pumpFreq\":" + String(state.pumpFreq) + ",";
  json += "\"autoMode\":" + String(state.autoMode) + ",";
  json += "\"rssi\":" + String(state.rssi);
  json += "}";
  return json;
}

void setTemperature(int temp) {
  uint32_t command = 0x10000000 | (temp & 0xFF); // Replace with actual encoding logic
  heater.sendCommand(command);
}

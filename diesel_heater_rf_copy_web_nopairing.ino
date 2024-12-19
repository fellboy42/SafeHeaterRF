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

  // Initialize heater with a predefined address
  uint32_t heaterAddr = 0x123456; // Replace with the actual address of your heater
  heater.begin();
  heater.setAddress(heaterAddr);
  Serial.print("Using preconfigured heater address: ");
  Serial.println(heaterAddr, HEX);
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
    "<!DOCTYPE html>"
    "<html lang='en'>"
    "<head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>Diesel Heater Control</title>"
    "<style>"
    "body { font-family: Arial, sans-serif; background-color: #f4f4f4; margin: 0; padding: 0; text-align: center; }"
    "h1 { background-color: #333; color: white; padding: 20px; margin: 0; }"
    ".container { max-width: 600px; margin: 20px auto; padding: 20px; background: white; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); border-radius: 10px; }"
    "button { padding: 10px 20px; margin: 10px; font-size: 16px; border: none; border-radius: 5px; cursor: pointer; }"
    ".start { background-color: #28a745; color: white; }"
    ".stop { background-color: #dc3545; color: white; }"
    ".range-container { margin: 20px 0; }"
    ".range-label { font-size: 16px; margin-bottom: 10px; }"
    "input[type='range'] { width: 100%; }"
    "</style>"
    "<script>"
    "let socket = new WebSocket('ws://' + location.hostname + ':81');"
    "socket.onmessage = function(event) {"
    "  const data = JSON.parse(event.data);"
    "  console.log(data);"
    "  document.getElementById('status').innerText = 'Heater Status: ' + (data.state ? 'On' : 'Off');"
    "};"
    "function sendCommand(cmd, value = 0) {"
    "  const message = JSON.stringify({ command: cmd, value: value });"
    "  socket.send(message);"
    "}"
    "</script>"
    "</head>"
    "<body>"
    "<h1>Diesel Heater Control</h1>"
    "<div class='container'>"
    "<div id='status'>Heater Status: Unknown</div>"
    "<button class='start' onclick=\"sendCommand('start')\">Start Heater</button>"
    "<button class='stop' onclick=\"sendCommand('stop')\">Stop Heater</button>"
    "<div class='range-container'>"
    "<div class='range-label'>Set Temperature: <span id='tempValue'>20</span>°C</div>"
    "<input type='range' min='10' max='30' step='1' value='20' onchange=\"document.getElementById('tempValue').innerText=this.value; sendCommand('setTemp', this.value);\">"
    "</div>"
    "</div>"
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

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <WiFiManager.h>
#include "DieselHeaterRF.h"

// WiFi credentials
const char *ssid = "stevennet4g";
const char *password = "12222111";

// Web server and WebSocket server
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Diesel Heater
#define HEATER_POLL_INTERVAL 4000
#define HEATER_CMD_START  0x01
#define HEATER_CMD_STOP   0x02
#define HEATER_PAIRING_CMD 0x03

DieselHeaterRF heater;
heater_state_t state;

// Function Prototypes
void handleRoot();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
String buildStatusJSON();
void setTemperature(int temp);
void setupWiFi();
bool pairWithHeater();

// WiFi setup
void setupWiFi() {
  WiFiManager wifiManager;

  // Attempt to connect to saved WiFi credentials
  if (!WiFi.begin(ssid, password)) {
    Serial.println("Failed to connect to saved WiFi. Starting captive portal...");
  }

  // Start a WiFi configuration portal if needed
  if (!wifiManager.autoConnect("DieselHeaterAP")) {
    Serial.println("Failed to connect to WiFi and setup timed out. Restarting...");
    ESP.restart();
  }

  Serial.println("WiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);

  // Initialize EEPROM
  EEPROM.begin(512);

  // Initialize WiFi
  setupWiFi();

  // Start WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Start Web server
  server.on("/", handleRoot);
  server.begin();

  // Initialize heater with address from EEPROM
  uint32_t heaterAddr = EEPROM.read(0) | (EEPROM.read(1) << 8) | (EEPROM.read(2) << 16) | (EEPROM.read(3) << 24);
  if (heaterAddr == 0xFFFFFFFF) {
    heaterAddr = 0x123456; // Default address
  }
  heater.begin();
  heater.setAddress(heaterAddr);

  Serial.print("Using heater address: 0x");
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
    ".pair { background-color: #007bff; color: white; }"
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
    "  document.getElementById('power').innerText = 'Power: ' + data.power + ' W';"
    "  document.getElementById('voltage').innerText = 'Voltage: ' + data.voltage + ' V';"
    "  document.getElementById('ambientTemp').innerText = 'Ambient Temp: ' + data.ambientTemp + '°C';"
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
    "<div id='power'>Power: Unknown</div>"
    "<div id='voltage'>Voltage: Unknown</div>"
    "<div id='ambientTemp'>Ambient Temp: Unknown</div>"
    "<button class='start' onclick=\"sendCommand('start')\">Start Heater</button>"
    "<button class='stop' onclick=\"sendCommand('stop')\">Stop Heater</button>"
    "<button class='pair' onclick=\"sendCommand('pair')\">Pair Heater</button>"
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
    } else if (command == "pair") {
      if (pairWithHeater()) {
        Serial.println("Pairing successful!");
      } else {
        Serial.println("Pairing failed.");
      }
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
  uint32_t command = 0x10000000 | (temp & 0xFF);
  heater.sendCommand(command);
}

bool pairWithHeater() {
  uint32_t pairedAddress = 0; // Replace with real logic to retrieve the address
  heater.sendCommand(HEATER_PAIRING_CMD);

  // Simulated logic to retrieve paired address
  delay(5000); // Simulate pairing process
  pairedAddress = 0x123456; // Replace with actual paired address logic

  if (pairedAddress != 0) {
    EEPROM.write(0, pairedAddress & 0xFF);
    EEPROM.write(1, (pairedAddress >> 8) & 0xFF);
    EEPROM.write(2, (pairedAddress >> 16) & 0xFF);
    EEPROM.write(3, (pairedAddress >> 24) & 0xFF);
    EEPROM.commit();

    heater.setAddress(pairedAddress);
    Serial.print("Paired with heater. Address: 0x");
    Serial.println(pairedAddress, HEX);
    return true;
  }

  Serial.println("Pairing failed.");
  return false;
}

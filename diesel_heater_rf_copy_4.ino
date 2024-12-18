/*
 * 
 *    ESP32         CC1101
 *    -----         ------
 *    4   <-------> GDO2
 *    18  <-------> SCK
 *    3v3 <-------> VCC
 *    23  <-------> MOSI
 *    19  <-------> MISO
 *    5   <-------> CSn
 *    GND <-------> GND
 *
 *    Test tx 433.920 send every 10s check on  SDR 
 */

#include "DieselHeaterRF.h"

#define HEATER_POLL_INTERVAL  4000

uint32_t heaterAddr = 0x12345678; // Replace this with your heater's known address.

DieselHeaterRF heater;
heater_state_t state;

void setup() {
  Serial.begin(115200);

  heater.begin();
  heater.setAddress(heaterAddr); // Set the known address of the heater.

  Serial.println("Heater initialized with fixed address.");
}

void loop() {
  // Replace HEATER_CMD_WAKEUP with the desired hex value
  uint32_t customCommand = 0x1234567;

  heater.sendCommand(customCommand); // Transmit the custom hex command

  if (heater.getState(&state)) {
    Serial.printf("State: %d, Power: %d, Voltage: %f, Ambient: %d, Case: %d, Setpoint: %d, PumpFreq: %f, Auto: %d, RSSI: %d\n", 
                  state.state, state.power, state.voltage, state.ambientTemp, state.caseTemp, state.setpoint, state.pumpFreq, state.autoMode, state.rssi); 
  } else {
    Serial.println("Failed to get the state");
  }

  delay(HEATER_POLL_INTERVAL);
}

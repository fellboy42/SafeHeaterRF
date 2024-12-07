#ifndef SafeHeaterRF_h
#define SafeHeaterRF_h

#include <Arduino.h>

// Pin Definitions
#define SAFE_HEATER_SCK_PIN   18
#define SAFE_HEATER_MISO_PIN  19
#define SAFE_HEATER_MOSI_PIN  23
#define SAFE_HEATER_SS_PIN    5
#define SAFE_HEATER_GDO2_PIN  4

// Command Definitions
#define SAFE_HEATER_CMD_WAKEUP  0x23
#define SAFE_HEATER_CMD_MODE    0x24
#define SAFE_HEATER_CMD_POWER   0x2B
#define SAFE_HEATER_CMD_UP      0x3C
#define SAFE_HEATER_CMD_DOWN    0x3E

// Heater State Definitions
#define SAFE_HEATER_STATE_OFF            0x00
#define SAFE_HEATER_STATE_STARTUP        0x01
#define SAFE_HEATER_STATE_RUNNING        0x05
#define SAFE_HEATER_STATE_SHUTDOWN       0x06
#define SAFE_HEATER_STATE_ERROR          0xFF // New state to indicate errors

// Safety Parameters
#define SAFE_HEATER_MAX_TEMP       120  // Maximum allowed temperature in Celsius
#define SAFE_HEATER_MIN_VOLTAGE    10.0 // Minimum voltage in volts
#define SAFE_HEATER_TX_RETRIES     10   // Retransmission count
#define SAFE_HEATER_RX_TIMEOUT_MS  5000 // Response timeout in milliseconds

// Heater State Structure
typedef struct {
  uint8_t state       = SAFE_HEATER_STATE_OFF;
  uint8_t power       = 0;
  float voltage       = 0.0;
  int8_t ambientTemp  = 0;
  uint8_t caseTemp    = 0;
  int8_t setpoint     = 0;
  float pumpFreq      = 0.0;
  uint8_t autoMode    = 0;
  int16_t rssi        = 0;
} safe_heater_state_t;

// Class Definition
class SafeHeaterRF
{
  public:
    // Constructors
    SafeHeaterRF(); 
    SafeHeaterRF(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t ss, uint8_t gdo2);

    // Destructor
    ~SafeHeaterRF();

    // Initialization
    void begin(); 
    void begin(uint32_t heaterAddr);

    // Address Management
    void setAddress(uint32_t heaterAddr);

    // Heater Control
    bool getState(safe_heater_state_t *state, uint32_t timeout = SAFE_HEATER_RX_TIMEOUT_MS); 
    void sendCommand(uint8_t cmd, uint32_t addr, uint8_t retries = SAFE_HEATER_TX_RETRIES);

    // Safety Management
    void enableSafetyChecks(bool enable); 
    bool isSafeState(const safe_heater_state_t &state); 

  private:
    // Pin Assignments
    uint8_t _pinSck;
    uint8_t _pinMiso;
    uint8_t _pinMosi;
    uint8_t _pinSs;
    uint8_t _pinGdo2;

    // Internal Variables
    uint32_t _heaterAddr = 0;
    bool _safetyChecksEnabled = true;

    // Internal Methods
    void initRadio(); 
    void txPacket(uint8_t len, char *data); 
    bool rxPacket(uint8_t len, char *data, uint16_t timeout);

    // SPI Communication
    uint8_t writeReg(uint8_t addr, uint8_t value);
    void writeBurst(uint8_t addr, uint8_t len, char *data);
    void spiStart(); 
    void spiEnd(); 

    // Validation
    bool validateState(const safe_heater_state_t &state); 
};

#endif // SafeHeaterRF_h

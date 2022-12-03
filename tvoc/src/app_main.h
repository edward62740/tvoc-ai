#ifndef APP_MAIN_H
#define APP_MAIN_H

void mainTask(void *pvParameters);
void displaySleep(void);

// #define ENABLE_FACTORY_CLEAN_SENSOR
#ifdef DEBUG
#define DEBUG_BEGIN(...) DebugSerial.begin(__VA_ARGS__)
#define DEBUG_PRINT(...) DebugSerial.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) DebugSerial.println(__VA_ARGS__)
#else
#define DEBUG_BEGIN(...)
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#endif
static PGM_P self_name = "VOC-Sensor";
static const uint8_t self_mac[6] = {0x02, 0xF0, 0x0D, 0xBE, 0xEF, 0x01};

static PGM_P db_ip = "<redacted>";
static const uint16_t db_port = 8086;
static PGM_P db_name = "voc";
static PGM_P db_token = "<redacted>";

const uint8_t LED_LINK = PG12;
const uint8_t LED_ACT = PG13;
const uint8_t LED_ERR = PG14;
const uint8_t LED_MEAS = PG15;
const uint8_t LCD_BL = PC9;
const uint8_t PIR_INT = PA8;

const uint8_t UART_TX = PD5;
const uint8_t UART_RX = PD6;

const uint8_t ETH_MOSI = PB5;
const uint8_t ETH_MISO = PB4;
const uint8_t ETH_SCK = PB3;
const uint8_t ETH_CS = PB6;
const uint8_t ETH_RST = PB8;

const uint32_t DISPLAY_SLEEP_TIMEOUT_MS = 15000;
const bool PIR_SENSOR_ENABLED = true;

#endif // APP_MAIN_H
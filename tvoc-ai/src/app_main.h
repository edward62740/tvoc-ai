#ifndef APP_MAIN_H
#define APP_MAIN_H

void mainTask(void *pvParameters);

static PGM_P self_name = "VOC-Sensor";
static const uint8_t self_mac[6] = {0x02, 0xF0, 0x0D, 0xBE, 0xEF, 0x01};

static PGM_P db_ip = "<redacted>";
static const uint16_t db_port = 8086;
static PGM_P db_name = "tmp";
static PGM_P db_token = "<redacted>";


#endif // APP_MAIN_H
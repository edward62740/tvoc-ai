#ifndef APP_COMMS_H
#define APP_COMMS_H

#include <Arduino.h>
#include "SPI.h"
#include "Ethernet3.h"
#include "FreeRTOS.h"
#include "STM32FreeRTOS.h"
#include <map>
class AppComms
{
public:
    AppComms(SPIClass *spi, const uint8_t *mac, const uint8_t csPin, const uint8_t rstPin, const char *hostname, HardwareSerial *log = &Serial);

    ~AppComms();

    void begin(uint8_t no_link_tol_cnt = 5);

    bool isReady();

    int connect(const char *host, uint16_t port);

    template <typename T>
    bool send(T data, char *tag, const char *db_id, const char *token, size_t size)
    {
        this->db.db_id = (char *)db_id;
        this->db.token = (char *)token;
        char resp[255] = {0};
        String in_data = String(data);
        String in_tag = String(tag);
        String POSTData = in_tag + " value=" + in_data;
        client.print("POST /write?db=");
        client.print(this->db.db_id);
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(this->db.host);
        client.print("Authorization: Token ");
        client.println(this->db.token);
        client.println("User-Agent: STM32F746ZGT6");
        // client.println("Connection: close");
        client.println("Content-Type: application/x-www-form-urlencoded;");
        client.print("Content-Length: ");
        client.println(POSTData.length());
        client.println();
        client.println(POSTData);
        vTaskDelay(30);
        if (client.available())
        {
            client.readBytes(resp, client.available());
            this->ser_log->println(resp);
            return true;
        }
        return false;
    }
    
    bool sendAll(std::map<char *, float>, const char *db_id, const char *token);

private:
    EthernetClient client;
    HardwareSerial *ser_log;
    struct
    {
        uint8_t *mac;
        uint8_t csPin;
        uint8_t rstPin;
        SPIClass *spi;
        char *hostname; // self hostname
        uint8_t prev_link_state = 0; // 0 = link up, 1 = link down
        uint8_t no_link_tol_cnt = 5; // Number of times to check for link before resetting
    } eth_if; // Ethernet interface
    struct
    {
        char *host; // db server ip
        uint16_t port; // db server port
        char *db_id; // db id
        char *token; // db token
    } db; // Database info
};
#endif /* APP_COMMS_H */
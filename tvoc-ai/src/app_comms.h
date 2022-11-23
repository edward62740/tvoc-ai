#ifndef APP_COMMS_H
#define APP_COMMS_H

#include <Arduino.h>
#include "SPI.h"
#include "Ethernet3.h"
class AppComms
{
public:
    AppComms(SPIClass *spi, uint8_t *mac, uint8_t csPin, uint8_t rstPin, const char *hostname, HardwareSerial *log = &Serial1);

    ~AppComms();

    void begin();

    bool isReady();

    int connect(const char *host, uint16_t port);

    template <typename T>
    bool send(T data, const char *db_id, const char *token, size_t size)
    {
        this->db.db_id = db_id;
        this->db.token = token;
        char resp[255] = {0};
        String in_data = String(data);
        String POSTData = "temp value=" + in_data;
        client.print("POST /write?db=");
        client.print(this->db.db_id);
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(this->db.host);
        client.print("Authorization: Token ");
        client.println(this->db.token);
        client.println("User-Agent: Arduino/1.6");
        // client.println("Connection: close");
        client.println("Content-Type: application/x-www-form-urlencoded;");
        client.print("Content-Length: ");
        client.println(POSTData.length());
        client.println();
        client.println(POSTData);
        delay(30);
        if (client.available())
        {
            client.readBytes(resp, client.available());
            this->ser_log->println(resp);
            return true;
        }
        else
            return false;
    }

private:
    SPIClass *spi;
    EthernetClient client;
    HardwareSerial *ser_log;
    struct
    {
        uint8_t *mac;
        uint8_t csPin;
        uint8_t rstPin;
        const char *hostname;
    } eth_if;
    struct
    {
        const char *host;
        uint16_t port;
        const char *db_id;
        const char *token;
    } db;
};
#endif /* APP_COMMS_H */
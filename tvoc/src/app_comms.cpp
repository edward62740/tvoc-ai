#include "app_comms.h"
#include "app_main.h"
AppComms::AppComms(SPIClass *spi, const uint8_t *mac, const uint8_t csPin, const uint8_t rstPin, const char *hostname)
{
    this->eth_if.spi = spi;
    this->eth_if.mac = (uint8_t *)mac;
    this->eth_if.csPin = csPin;
    this->eth_if.rstPin = rstPin;
    this->eth_if.hostname = (char *)hostname;
}

AppComms::~AppComms()
{
    delete this;
}

void AppComms::begin(uint8_t no_link_tol_cnt)
{
    this->eth_if.no_link_tol_cnt = no_link_tol_cnt;
    Ethernet.setCsPin(this->eth_if.csPin);
    Ethernet.setRstPin(this->eth_if.rstPin);
    Ethernet.setHostname(this->eth_if.hostname);
    Ethernet.phyMode(FULL_DUPLEX_10);
    Ethernet.begin(this->eth_if.mac, this->eth_if.spi);
}

bool AppComms::isReady()
{
    if (this->eth_if.prev_link_state > 5 && Ethernet.link() == 1)
    {
        NVIC_SystemReset();
    }
    Ethernet.link() == 1 ? this->eth_if.prev_link_state = 0 : this->eth_if.prev_link_state++;
    Ethernet.maintain();
    return Ethernet.link() == 1 ? true : false;
}

int AppComms::connect(const char *host, uint16_t port)
{
    client.setTimeout(1000);
    this->eth_if.prev_link_state = Ethernet.link();
    return client.connect(host, port);
}

bool AppComms::sendAll(std::map<const char *, float> SensorResultMap, const char *db_id, const char *token)
{
    digitalWrite(LED_ERR, HIGH);
    if (Ethernet.link() == 1)
    {
        this->db.db_id = (char *)db_id;
        this->db.token = (char *)token;
        char resp[255] = {0};
        uint16_t payload_size = 0;

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
        String POSTData = "voc1 test=1,";
        for (auto &x : SensorResultMap)
        {
            String in_data = String(x.second);
            String in_tag = String(x.first);
            POSTData.concat(in_tag + "=" + in_data + ",");
        }
        POSTData.remove(POSTData.length() - 1);

        client.println(POSTData.length());
        client.println();
        DEBUG_PRINTLN(POSTData);
        client.println(POSTData);
        vTaskDelay(30);
        if (client.available())
        {
            client.readBytes(resp, client.available());
            DEBUG_PRINTLN(resp);
            client.flush();
            client.clearWriteError();
            digitalWrite(LED_ERR, LOW);
            return true;
        }
        DEBUG_PRINTLN("No response from server");
        client.flush();
        client.clearWriteError();
        return false;
    }
    else
        return false;
}

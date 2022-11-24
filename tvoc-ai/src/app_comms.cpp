#include "app_comms.h"

AppComms::AppComms(SPIClass *spi, const uint8_t *mac, const uint8_t csPin, const uint8_t rstPin, const char *hostname, HardwareSerial *log)
{
    this->eth_if.spi = spi;
    this->ser_log = log;
    this->eth_if.mac = (uint8_t *)mac;
    this->eth_if.csPin = csPin;
    this->eth_if.rstPin = rstPin;
    this->eth_if.hostname = (char *)hostname;
}

AppComms::~AppComms()
{
}

void AppComms::begin()
{
    Ethernet.setCsPin(this->eth_if.csPin);
    Ethernet.setRstPin(this->eth_if.rstPin);
    Ethernet.setHostname(this->eth_if.hostname);
    Ethernet.begin(this->eth_if.mac, this->eth_if.spi);
}

bool AppComms::isReady()
{
    this->ser_log->println(Ethernet.localIP());
    return Ethernet.link() == 1 ? true : false;
}

int AppComms::connect(const char *host, uint16_t port)
{
    return client.connect(host, port);
}

bool AppComms::sendAll(std::map<char *, float> SensorResultMap, const char *db_id, const char *token)
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
    String POSTData = "voc test=1,";
    for (auto &x : SensorResultMap)
    {
        String in_data = String(x.second);
        String in_tag = String(x.first);
        POSTData.concat(in_tag + "=" + in_data + ",");
    }
    POSTData.remove(POSTData.length() - 1);

    client.println(POSTData.length());
    client.println();
    this->ser_log->println(POSTData);
    client.println(POSTData);
    vTaskDelay(30);
    if (client.available())
    {
        client.readBytes(resp, client.available());
        this->ser_log->println(resp);
        return true;
    }
    return false;
    return true;
}

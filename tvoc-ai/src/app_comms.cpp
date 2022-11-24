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
    for (auto const &x : SensorResultMap)
    {
        this->send<float>(x.second, x.first, db_id, token, sizeof(x.second));
    }
    return true;
}

#include "app_comms.h"

AppComms::AppComms(SPIClass *spi, uint8_t *mac, uint8_t csPin, uint8_t rstPin, const char *hostname, HardwareSerial *log)
{
    this->spi = spi;
    this->ser_log = log;
    this->eth_if.mac = mac;
    this->eth_if.csPin = csPin;
    this->eth_if.rstPin = rstPin;
    this->eth_if.hostname = hostname;
}

AppComms::~AppComms()
{
}

void AppComms::begin()
{
    Ethernet.setCsPin(this->eth_if.csPin);
    Ethernet.setRstPin(this->eth_if.rstPin);
    Ethernet.setHostname(this->eth_if.hostname);
    Ethernet.begin(this->eth_if.mac, this->spi);
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

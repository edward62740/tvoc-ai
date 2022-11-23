#include "FreeRTOS.h"
#include "STM32FreeRTOS.h"
#include <Arduino.h>
#include <Ethernet3.h>
#include "app_comms.h"

uint8_t Eth_MAC[] = {0x02, 0xF0, 0x0D, 0xBE, 0xEF, 0x01};

HardwareSerial DebugSerial(PD6, PD5);
SPIClass ethSPI(PB5, PB4, PB3);
AppComms comms(&ethSPI, Eth_MAC, PB6, PB8, "voc-sensor-t", &DebugSerial);


void mainTask(void *pvParameters)
{
 (void) pvParameters;
    DebugSerial.begin(115200);
    DebugSerial.println("Starting...");
    comms.begin();
    while (!comms.isReady())
    {
        DebugSerial.println("Waiting for Ethernet connection...");
        delay(1000);
    }
    DebugSerial.println("Ethernet started");
    comms.connect("<redacted>", 8086);

    for (;;)
    {
        // connect to the influxdb port
        uint8_t var = random(0, 100);
        comms.send<uint8_t>(var, "tmp", "<redacted>", sizeof(var));
        delay(2500);
    }
}
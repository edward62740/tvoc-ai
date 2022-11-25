#include "FreeRTOS.h"
#include "STM32FreeRTOS.h"
#include <Arduino.h>
#include <Ethernet3.h>
#include "app_comms.h"
#include "app_main.h"
#include "app_sensor.h"
#include "TFT_eSPI.h"

HardwareSerial DebugSerial(PD6, PD5);
SPIClass ethSPI(PB5, PB4, PB3);
AppComms comms(&ethSPI, self_mac, PB6, PB8, self_name, &DebugSerial);
AppSensor sensor(&DebugSerial);
TFT_eSPI tft = TFT_eSPI();
TaskHandle_t sensorTask;
void mainTask(void *pvParameters)
{
    pinMode(led_meas, OUTPUT);
    pinMode(led_err, OUTPUT);
    pinMode(led_act, OUTPUT);
    pinMode(led_link, OUTPUT);

    DebugSerial.begin(115200);
    DebugSerial.println("Starting...");
    /* tft.init();
      tft.initDMA();
      tft.resetViewport();
      tft.fillScreen(TFT_BLUE);
      tft.setCursor(0, 0);
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(2);
      tft.println("Starting...");
      tft.fillRectHGradient(0, 0, 240, 240, TFT_WHITE, TFT_RED);
      for (int i = 0; i < 2; i++)
      {
          tft.fillScreen(random(0x0000, 0xFFFF));
          delay(50);
      }*/
   
    
    comms.begin();
    while (!comms.isReady())
    {
        DebugSerial.println("Waiting for Ethernet connection...");
        vTaskDelay(200);
    }
    digitalWrite(led_link, HIGH);
    DebugSerial.println("Ethernet started");
    comms.connect(db_ip, db_port);

    sensor.startSensorTask(sensorTask, 2);

    for (;;)
    {
        if (xSemaphoreTake(sensor.xIsMeasurementReady(), 1) == pdTRUE)
            comms.sendAll(sensor.SensorResultMap, db_name, db_token) ? digitalWrite(led_err, LOW) : digitalWrite(led_err, HIGH);
        digitalToggle(led_act);
        comms.isReady() ? digitalWrite(led_link, HIGH) : digitalWrite(led_link, LOW);
        vTaskDelay(20);
    }
}
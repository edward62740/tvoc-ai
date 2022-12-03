#include "FreeRTOS.h"
#include "STM32FreeRTOS.h"
#include <Arduino.h>
#include <Ethernet3.h>
#include "app_comms.h"
#include "app_main.h"
#include "app_sensor.h"
#include "TFT_eSPI.h"
#include "app_display.h"

HardwareSerial DebugSerial(UART_RX, UART_TX);
SPIClass ethSPI(ETH_MOSI, ETH_MISO, ETH_SCK);
AppComms comms(&ethSPI, self_mac, ETH_CS, ETH_RST, self_name);
AppSensor sensor(1);
TFT_eSPI tft;
AppDisplay display(&tft, LCD_BL);
TaskHandle_t sensorTask;

extern void HardFault_Handler(void);

void HardFault_Handler(void)
{
    NVIC_SystemReset();
}

void mainTask(void *pvParameters)
{
    pinMode(LED_MEAS, OUTPUT);
    pinMode(LED_ERR, OUTPUT);
    pinMode(LED_ACT, OUTPUT);
    pinMode(LED_LINK, OUTPUT);
    pinMode(LCD_BL, OUTPUT);
    pinMode(PIR_INT, INPUT_PULLDOWN);
    analogWrite(LCD_BL, 0);
    DEBUG_BEGIN(115200);

    display.init();

    display.drawNoEth();
    comms.begin(); // Blocking function to start ethernet
    while (!comms.isReady())
    {
        DEBUG_PRINTLN("Waiting for Ethernet connection...");
        vTaskDelay(200);
    }
    display.drawLinkUp();
    digitalWrite(LED_LINK, HIGH);
    comms.connect(db_ip, db_port); // Blocking function to connect to influxdb

    /* Spawns a task to run sensor algorithms, should be yielded to by main task every 3s (+-) 5% for accuracy */
    sensor.startSensorTask(sensorTask, (UBaseType_t)2);

    for (;;)
    {
        if (xSemaphoreTake(sensor.xIsMeasurementReady(), 1) == pdTRUE) // If sensor yields semp
        {
            comms.sendAll(sensor.SensorResultMap, db_name, db_token) ? digitalWrite(LED_ERR, LOW) : digitalWrite(LED_ERR, HIGH); // Send data to influxdb
            display.drawData(sensor.SensorResultMap, Ethernet.link()); // Draw data on display
        }
        if(PIR_SENSOR_ENABLED) displaySleep(); //  Check if display state is appropriate
        digitalToggle(LED_ACT);
        comms.isReady() ? digitalWrite(LED_LINK, HIGH) : digitalWrite(LED_LINK, LOW);
        vTaskDelay(20); // Give other tasks a chance to run
    }
}

void displaySleep(void)
{
    static uint32_t last_pir = HAL_GetTick();
    static bool prev_state = true;
    if (digitalRead(PIR_INT))
    {
        if (!prev_state)
        {
            for (int i = 5; i < 255; i++)
            {
                analogWrite(LCD_BL, i);
                vTaskDelay(2);
            }
        }
        prev_state = true;
        last_pir = HAL_GetTick();
    }

    if (HAL_GetTick() - last_pir >= DISPLAY_SLEEP_TIMEOUT_MS)
    {
        if (prev_state)
        {
            for (int i = 255; i > 5; i--)
            {
                analogWrite(LCD_BL, i);
                vTaskDelay(2);
            }
        }
        prev_state = false;
        analogWrite(LCD_BL, 5);
    }
    else
        analogWrite(LCD_BL, 255);
}
#include <Arduino.h>
#include "FreeRTOS.h"
#include "STM32FreeRTOS.h"
#include "app_main.h"

void setup()
{
    // Hand over control to FreeRTOS
    xTaskCreate(
        mainTask,                     /* Task function. */
        (const portCHAR *)"mainTask", /* name of task. */
        8192,                        /* Stack size of task */
        NULL,                         /* parameter of the task */
        0,                            /* priority of the task */
        NULL);                        /* Task handle to keep track of created task */


    vTaskStartScheduler();

    while (1)
        ;
}

void loop()
{
}

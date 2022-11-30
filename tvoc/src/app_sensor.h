#ifndef APP_SENSOR_H
#define APP_SENSOR_H
#include <Arduino.h>
#include "STM32FreeRTOS.h"
#include "FreeRTOS.h"

#include <zmod4xxx.h>
#include <zmod4xxx_hal.h>
#include <iaq_2nd_gen.h>
#include <map>

class AppSensor
{
public:
    AppSensor(HardwareSerial *log = &Serial);
    ~AppSensor();
    void startSensorTask(TaskHandle_t handle, UBaseType_t priority);
    SemaphoreHandle_t xIsMeasurementReady(); // Function to return xMeasFlag

    std::map<const char *, float> SensorResultMap; // Map to store sensor results

private:
    void sensorTask(void *pvParameters); // Main sensor task, will regularly update SensorResultMap and give xMeasFlag through xIsMeasurementReady
    static void startSensorTaskImpl(void *_this);

    TaskHandle_t *appSensorTask; // Task handle for the sensor task
    HardwareSerial *ser_log; // Serial port for logging
    struct
    {
        /* zmod4xxx built-ins */
        zmod4xxx_dev_t dev;
        uint8_t zmod4xxx_status;
        uint8_t prod_data[7];
        uint8_t adc_result[32] = {0};
        iaq_2nd_gen_handle_t algo_handle;
        iaq_2nd_gen_results_t algo_results;

        SemaphoreHandle_t xMeasFlag; // Ensures that the same measurement is not read twice
        bool isDataValid = false; // Indicates if the data is valid
        volatile uint64_t runTimeSecs = 0; // Time since the sensor startup
        volatile uint32_t numMeas = 0; // Number of measurements taken since startup
    } s;
};

#endif // APP_SENSOR_H
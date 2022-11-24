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
    SemaphoreHandle_t xIsMeasurementReady();
    std::map<char *, float> SensorResultMap;
private:
    TaskHandle_t *appSensorTask;
    HardwareSerial *ser_log;
    struct
    {
        zmod4xxx_dev_t dev;
        uint8_t zmod4xxx_status;
        uint8_t prod_data[7];
        uint8_t adc_result[32] = {0};
        iaq_2nd_gen_handle_t algo_handle;
        iaq_2nd_gen_results_t algo_results;

        SemaphoreHandle_t xMeasFlag;
        bool setupError = false;
    } s;
    
    void sensorTask(void *pvParameters);
    static void startSensorTaskImpl(void *_this);
};

#endif // APP_SENSOR_H
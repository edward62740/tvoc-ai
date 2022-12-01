#include "app_sensor.h"
#include <zmod4410_config_iaq2.h>
#include "app_main.h"

#ifdef ENABLE_FACTORY_CLEAN_SENSOR
extern "C" {
#include "m7f/zmod4xxx_cleaning.h"
}
#endif

AppSensor::AppSensor(HardwareSerial *log)
{
    this->s.xMeasFlag = xSemaphoreCreateBinary();
    this->ser_log = log;
}

AppSensor::~AppSensor()
{
    delete this;
}

void AppSensor::startSensorTask(TaskHandle_t handle, UBaseType_t priority)
{
    this->appSensorTask = &handle;
    this->ser_log->println("Starting sensor task");
    xTaskCreate(
        this->startSensorTaskImpl,      /* Task function. */
        (const portCHAR *)"sensorTask", /* name of task. */
        16384,                          /* Stack size of task */
        this,                           /* parameter of the task */
        priority,                       /* priority of the task */
        this->appSensorTask);           /* Task handle to keep track of created task */
}

void AppSensor::sensorTask(void *pvParameters)
{
    int8_t lib_ret;
    zmod4xxx_err api_ret;

    this->ser_log->println(F("Starting the Sensor!"));

    api_ret = init_hardware(&(this->s.dev));
    if (api_ret)
    {
        this->ser_log->print(F("Error "));
        this->ser_log->print(api_ret);
        this->ser_log->println(F(" during init hardware, exiting program!\n"));
    }
    this->s.dev.i2c_addr = ZMOD4410_I2C_ADDR;
    this->s.dev.pid = ZMOD4410_PID;
    this->s.dev.init_conf = &zmod_sensor_type[INIT];
    this->s.dev.meas_conf = &zmod_sensor_type[MEASUREMENT];
    this->s.dev.prod_data = this->s.prod_data;
    api_ret = zmod4xxx_read_sensor_info(&(this->s.dev));
    if (api_ret)
    {
        this->ser_log->print(F("Error "));
        this->ser_log->print(api_ret);
        this->ser_log->println(
            F(" during reading sensor information, exiting program!\n"));
    }

    #ifdef ENABLE_FACTORY_CLEAN_SENSOR
    this->ser_log->println(F("Cleaning sensor..."));
    int8_t ret = zmod4xxx_cleaning_run(&(this->s.dev));
    if (ret) {
       this->ser_log->printf("Error %d during cleaning procedure, exiting program!\n", ret);
    }
    #endif

    /* Preperation of sensor */
    api_ret = zmod4xxx_prepare_sensor(&(this->s.dev));
    if (api_ret)
    {
        this->ser_log->print(F("Error "));
        this->ser_log->print(api_ret);
        this->ser_log->println(F(" during preparation of the sensor, exiting program!\n"));
    }


    /* One time initialization of the algorithm */
    lib_ret = init_iaq_2nd_gen(&(this->s.algo_handle));
    if (lib_ret)
    {
        this->ser_log->println(F("Error "));
        this->ser_log->print(lib_ret);
        this->ser_log->println(F(" during initializing algorithm, exiting program!"));
    }
    api_ret = zmod4xxx_start_measurement(&(this->s.dev));
    if (api_ret)
    {
        this->ser_log->print(F("Error "));
        this->ser_log->print(api_ret);
        this->ser_log->println(F(" during starting measurement, exiting program!\n"));
    }
    for (;;)
    {
        int8_t lib_ret;
        zmod4xxx_err api_ret;
        uint32_t polling_counter = 0;

        do
        {
            api_ret = zmod4xxx_read_status(&(this->s.dev), &(this->s.zmod4xxx_status));
            if (api_ret)
            {
                this->ser_log->print(F("Error "));
                this->ser_log->print(api_ret);
                this->ser_log->println(F(" during read of sensor status, exiting program!\n"));
            }
            polling_counter++;
            this->s.dev.delay_ms(200);
        } while ((this->s.zmod4xxx_status & STATUS_SEQUENCER_RUNNING_MASK) &&
                 (polling_counter <= ZMOD4410_IAQ2_COUNTER_LIMIT));

        if (ZMOD4410_IAQ2_COUNTER_LIMIT <= polling_counter)
        {
            api_ret = zmod4xxx_check_error_event(&(this->s.dev));
            if (api_ret)
            {
                this->ser_log->print(F("Error "));
                this->ser_log->print(api_ret);
                this->ser_log->println(F(" during read of sensor status, exiting program!\n"));
            }
            this->ser_log->print(F("Error"));
            this->ser_log->print(ERROR_GAS_TIMEOUT);
            this->ser_log->print(F(" exiting program!"));
        }
        else
        {
            polling_counter = 0;
        }

        api_ret = zmod4xxx_read_adc_result(&(this->s.dev), this->s.adc_result);
        if (api_ret)
        {
            this->ser_log->print(F("Error "));
            this->ser_log->print(api_ret);
            this->ser_log->println(F(" during read of ADC results, exiting program!\n"));
        }

        /* calculate the algorithm */
        lib_ret = calc_iaq_2nd_gen(&(this->s.algo_handle), &(this->s.dev), this->s.adc_result, &(this->s.algo_results));
        if ((lib_ret != IAQ_2ND_GEN_OK) && (lib_ret != IAQ_2ND_GEN_STABILIZATION))
        {
            digitalWrite(LED_ERR, HIGH);
            this->ser_log->println(F("Error when calculating algorithm, exiting program!"));
        }
        else
        {
            digitalWrite(LED_ERR, LOW);
            this->SensorResultMap["logRcda"] = this->s.algo_results.log_rcda;
            this->SensorResultMap["EtOH"] = this->s.algo_results.etoh;
            this->SensorResultMap["TVOC"] = this->s.algo_results.tvoc;
            this->SensorResultMap["eCO2"] = this->s.algo_results.eco2;
            this->SensorResultMap["IAQ"] = this->s.algo_results.iaq;
            this->SensorResultMap["Measurements"] = static_cast<float>(this->s.numMeas++);
            this->SensorResultMap["Status"] = static_cast<float>(this->s.zmod4xxx_status);
            this->SensorResultMap["MeasCount"] = static_cast<float>(this->s.numMeas);
            xSemaphoreGive(this->s.xMeasFlag);
            this->ser_log->println(F("*********** Measurements ***********"));

            this->ser_log->print(F(" log_Rcda = "));
            this->ser_log->print(this->s.algo_results.log_rcda);
            this->ser_log->println(F(" logOhm"));
            this->ser_log->print(F(" EtOH = "));
            this->ser_log->print(this->s.algo_results.etoh);
            this->ser_log->println(F(" ppm"));
            this->ser_log->print(F(" TVOC = "));
            this->ser_log->print(this->s.algo_results.tvoc);
            this->ser_log->println(F(" mg/m^3"));
            this->ser_log->print(F(" eCO2 = "));
            this->ser_log->print(this->s.algo_results.eco2);
            this->ser_log->println(F(" ppm"));
            this->ser_log->print(F(" IAQ  = "));
            this->ser_log->println(this->s.algo_results.iaq);

            lib_ret == IAQ_2ND_GEN_STABILIZATION ? this->s.isDataValid = false : this->s.isDataValid = true;
            this->SensorResultMap["Valid"] = static_cast<float>(this->s.isDataValid);

            this->ser_log->println(uxTaskGetStackHighWaterMark(NULL));
        }
        digitalWrite(LED_MEAS, LOW);
        /* wait 1.99 seconds before starting the next measurement */
        vTaskDelay(1999 / portTICK_PERIOD_MS);
        digitalWrite(LED_MEAS, HIGH);
        /* start a new measurement before result calculation */
        api_ret = zmod4xxx_start_measurement(&(this->s.dev));
        if (api_ret)
        {
            this->ser_log->println(F("Error when starting measurement, exiting program!\n"));
        }
    }
}

void AppSensor::startSensorTaskImpl(void *_this)
{
    static_cast<AppSensor *>(_this)->sensorTask(NULL);
}

SemaphoreHandle_t AppSensor::xIsMeasurementReady()
{
    return this->s.xMeasFlag;
}

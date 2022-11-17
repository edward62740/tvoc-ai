#include <Arduino.h>

#include <zmod4410_config_iaq2.h>
#include <zmod4xxx.h>
#include <zmod4xxx_hal.h>
#include <iaq_2nd_gen.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Ethernet3.h>
#include <Firebase_ESP_Client.h>
#include <stm32f7xx_hal_gpio.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {
    0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x0F};

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;
TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

/* Global Variables */
zmod4xxx_dev_t dev;

/* Sensor target variables */
uint8_t zmod4xxx_status;
uint8_t prod_data[ZMOD4410_PROD_DATA_LEN];
uint8_t adc_result[32] = {0};

/* IAQ 2ND GEN Variables */
iaq_2nd_gen_handle_t algo_handle;
iaq_2nd_gen_results_t algo_results;
HardwareSerial DebugSerial(PD6, PD5);
SPIClass ethSPI(PB5, PB4, PB3);
IPAddress gw(10, 10, 1, 1);
IPAddress sn(255, 255, 255, 0);


void setup()
{

    SystemCoreClockUpdate();

    DebugSerial.begin(115200, SERIAL_8N1);

    pinMode(PG12, OUTPUT);
    digitalWrite(PG12, HIGH);
    pinMode(PG13, OUTPUT);
    digitalWrite(PG13, HIGH);
    pinMode(PG14, OUTPUT);
    digitalWrite(PG14, HIGH);
    pinMode(PG15, OUTPUT);
    digitalWrite(PG15, HIGH);
    pinMode(PB8, OUTPUT);
    pinMode(PB6, OUTPUT);
    DebugSerial.println("STARTED");
       // tft.init();
    //tft.setRotation(1);
   // tft.fillScreen(TFT_RED);
   // tft.setCursor(0, 0);
    Ethernet.setCsPin(PB6);
    Ethernet.setRstPin(PB8);

    while (Ethernet.begin(mac, &ethSPI) == 0)
    {
        DebugSerial.println(Ethernet.linkReport());
        DebugSerial.println(Ethernet.speedReport());
        DebugSerial.println("Failed to configure Ethernet using DHCP");
        DebugSerial.print("My IP address: ");
        for (byte thisByte = 0; thisByte < 4; thisByte++)
        {
            // print the value of each byte of the IP address:
            DebugSerial.print(Ethernet.localIP()[thisByte], DEC);
            DebugSerial.print(".");
        }
        DebugSerial.println();
    }
    // print your local IP address:

    delay(100);
    DebugSerial.println(Ethernet.linkReport());
    DebugSerial.println(Ethernet.speedReport());
    DebugSerial.println("Failed to configure Ethernet using DHCP");
    DebugSerial.print("My IP address: ");
    for (byte thisByte = 0; thisByte < 4; thisByte++)
    {
        // print the value of each byte of the IP address:
        DebugSerial.print(Ethernet.localIP()[thisByte], DEC);
        DebugSerial.print(".");
    }
    DebugSerial.println();
    delay(1000);

    int8_t lib_ret;
    zmod4xxx_err api_ret;

    digitalToggle(PG15);
    DebugSerial.println(F("Starting the Sensor!"));
    api_ret = init_hardware(&dev);
    if (api_ret)
    {
        DebugSerial.print(F("Error "));
        DebugSerial.print(api_ret);
        DebugSerial.println(F(" during init hardware, exiting program!\n"));
    }

    dev.i2c_addr = ZMOD4410_I2C_ADDR;
    dev.pid = ZMOD4410_PID;
    dev.init_conf = &zmod_sensor_type[INIT];
    dev.meas_conf = &zmod_sensor_type[MEASUREMENT];
    dev.prod_data = prod_data;
    digitalToggle(PG15);
    api_ret = zmod4xxx_read_sensor_info(&dev);
    if (api_ret)
    {
        DebugSerial.print(F("Error "));
        DebugSerial.print(api_ret);
        DebugSerial.println(
            F(" during reading sensor information, exiting program!\n"));
    }
    digitalToggle(PG15);
    /* Preperation of sensor */
    api_ret = zmod4xxx_prepare_sensor(&dev);
    if (api_ret)
    {
        DebugSerial.print(F("Error "));
        DebugSerial.print(api_ret);
        DebugSerial.println(F(" during preparation of the sensor, exiting program!\n"));
    }
    digitalToggle(PG15);
    /* One time initialization of the algorithm */
    lib_ret = init_iaq_2nd_gen(&algo_handle);
    if (lib_ret)
    {
        DebugSerial.println(F("Error "));
        DebugSerial.print(lib_ret);
        DebugSerial.println(F(" during initializing algorithm, exiting program!"));
    }
    digitalToggle(PG15);
    api_ret = zmod4xxx_start_measurement(&dev);
    if (api_ret)
    {
        DebugSerial.print(F("Error "));
        DebugSerial.print(api_ret);
        DebugSerial.println(F(" during starting measurement, exiting program!\n"));
    }
    digitalToggle(PG15);
}

void loop()
{
    // put your main code here, to run repeatedly:
    /* lib_ret -> Return of Library
     * api_ret -> Return of API
     */
    int8_t lib_ret;
    zmod4xxx_err api_ret;
    uint32_t polling_counter = 0;

    do
    {
        api_ret = zmod4xxx_read_status(&dev, &zmod4xxx_status);
        if (api_ret)
        {
            DebugSerial.print(F("Error "));
            DebugSerial.print(api_ret);
            DebugSerial.println(F(" during read of sensor status, exiting program!\n"));
        }
        polling_counter++;
        dev.delay_ms(200);
    } while ((zmod4xxx_status & STATUS_SEQUENCER_RUNNING_MASK) &&
             (polling_counter <= ZMOD4410_IAQ2_COUNTER_LIMIT));

    if (ZMOD4410_IAQ2_COUNTER_LIMIT <= polling_counter)
    {
        api_ret = zmod4xxx_check_error_event(&dev);
        if (api_ret)
        {
            DebugSerial.print(F("Error "));
            DebugSerial.print(api_ret);
            DebugSerial.println(F(" during read of sensor status, exiting program!\n"));
        }
        DebugSerial.print(F("Error"));
        DebugSerial.print(ERROR_GAS_TIMEOUT);
        DebugSerial.print(F(" exiting program!"));
    }
    else
    {
        polling_counter = 0;
    }

    api_ret = zmod4xxx_read_adc_result(&dev, adc_result);
    if (api_ret)
    {
        DebugSerial.print(F("Error "));
        DebugSerial.print(api_ret);
        DebugSerial.println(F(" during read of ADC results, exiting program!\n"));
    }

    /* calculate the algorithm */
    lib_ret = calc_iaq_2nd_gen(&algo_handle, &dev, adc_result, &algo_results);
    if ((lib_ret != IAQ_2ND_GEN_OK) && (lib_ret != IAQ_2ND_GEN_STABILIZATION))
    {
        DebugSerial.println(F("Error when calculating algorithm, exiting program!"));
    }
    else
    {
        /*  DebugSerial.println(F("*********** Measurements ***********"));
          for (int i = 0; i < 13; i++)
          {
              DebugSerial.print(F(" Rmox["));
              DebugSerial.print(i);
              DebugSerial.print(F("] = "));
              DebugSerial.print(algo_results.rmox[i] / 1e3);
              DebugSerial.println(F(" kOhm"));
          }
  */
        /*  DebugSerial.print(F(" log_Rcda = "));
            DebugSerial.print(algo_results.log_rcda);
            DebugSerial.println(F(" logOhm"));
            DebugSerial.print(F(" EtOH = "));
            DebugSerial.print(algo_results.etoh);
            DebugSerial.println(F(" ppm"));
            DebugSerial.print(F(" TVOC = "));
            DebugSerial.print(algo_results.tvoc);
            DebugSerial.println(F(" mg/m^3"));
            DebugSerial.print(F(" eCO2 = "));
            DebugSerial.print(algo_results.eco2);
            DebugSerial.println(F(" ppm"));*/
        DebugSerial.print(F(" IAQ  = "));
        DebugSerial.println(algo_results.iaq);

        if (lib_ret == IAQ_2ND_GEN_STABILIZATION)
        {
            DebugSerial.println(F("Warmup!"));
        }
        else
        {
            DebugSerial.println(F("Valid!"));
        }
    }

    /* wait 1.99 seconds before starting the next measurement */
    dev.delay_ms(1990);
    digitalToggle(PG15);
    /* start a new measurement before result calculation */
    api_ret = zmod4xxx_start_measurement(&dev);
    if (api_ret)
    {
        DebugSerial.println(F("Error when starting measurement, exiting program!\n"));
    }
}

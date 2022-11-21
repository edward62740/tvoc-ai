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

#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 2. Install SSLClient library */
// https://github.com/OPEnSLab-OSU/SSLClient
#include <SSLClient.h>

/* 3. Create Trus anchors for the server i.e. www.google.com */
// https://github.com/OPEnSLab-OSU/SSLClient/blob/master/TrustAnchors.md
// or generate using this site https://openslab-osu.github.io/bearssl-certificate-utility/
#include "trust_anchors.h"

// For NTP time client
#include "MB_NTP.h"

// For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

/* 4. Define the API Key */
#define API_KEY "<redacted>"

/* 5. Define the RTDB URL */
#define DATABASE_URL "<redacted>" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 6. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "USER_EMAIL"
#define USER_PASSWORD "USER_PASSWORD"

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
IPAddress dnss(10, 10, 1, 1);
IPAddress self(10, 10, 1, 183);
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
SSLClient ssl_client(client, TAs, (size_t)TAs_NUM, PD0);

// For NTP client
EthernetUDP udpClient;

MB_NTP ntpClient(&udpClient, "pool.ntp.org" /* NTP host */, 123 /* NTP port */, 0 /* timezone offset in seconds */);
unsigned long sendDataPrevMillis = 0;

int count = 0;

volatile bool dataChanged = false;
uint32_t timestamp = 0;

void ResetEthernet()
{
    Ethernet.hardreset();
}

void networkConnection()
{

    ResetEthernet();

    Ethernet.begin(mac, &ethSPI);

    unsigned long to = millis();

    while (Ethernet.linkReport() == "LINK" || millis() - to < 2000)
    {
        delay(100);
    }

    if (Ethernet.linkReport() == "LINK")
    {
        DebugSerial.print("Connected with IP ");
        DebugSerial.println(Ethernet.localIP());
    }
    else
    {
        DebugSerial.println("Can't connected");
    }
}

// Define the callback function to handle server status acknowledgement
void networkStatusRequestCallback()
{
    // Set the network status
    fbdo.setNetworkStatus(Ethernet.linkReport() == "LINK");
}

// Define the callback function to handle server connection
void tcpConnectionRequestCallback(const char *host, int port)
{

    // You may need to set the system timestamp to use for
    // auth token expiration checking.

    if (timestamp == 0)
    {
        DebugSerial.print("Getting time from NTP server...");
        timestamp = ntpClient.getTime(2000 /* wait 2000 ms */);

        if (timestamp > 0)
            Firebase.setSystemTime(timestamp);
    }

    DebugSerial.print("Connecting to server via external Client... ");
    if (!ssl_client.connect(host, port))
    {
        DebugSerial.println("failed.");
        return;
    }
    DebugSerial.println("success.");
}

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
    Ethernet.setCsPin(PB6);
    Ethernet.setRstPin(PB8);
    Ethernet.setHostname("VOC Sensor");
   Ethernet.begin(mac, self, sn, gw, dnss, &ethSPI);
   
    // print your local IP address:
    Ethernet.WoL();
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
    DebugSerial.println(Ethernet.dnsServerIP());
    DebugSerial.println();
    delay(1000);
    DebugSerial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the RTDB URL (required) */
    config.database_url = DATABASE_URL;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

    /* fbdo.setExternalClient and fbdo.setExternalClientCallbacks must be called before Firebase.begin */

    /* Assign the pointer to global defined external SSL Client object */
    fbdo.setExternalClient(&ssl_client);

    /* Assign the required callback functions */
    fbdo.setExternalClientCallbacks(tcpConnectionRequestCallback, networkConnection, networkStatusRequestCallback);

    // Comment or pass false value when WiFi reconnection will control by your code or third party library
    Firebase.reconnectWiFi(true);

    Firebase.setDoubleDigits(5);
    if (Firebase.signUp(&config, &auth, "", ""))
    {
        DebugSerial.println("ok");

        /** if the database rules were set as in the example "EmailPassword.ino"
         * This new user can be access the following location.
         *
         * "/UserData/<user uid>"
         *
         * The new user UID or <user uid> can be taken from auth.token.uid
         */
    }
    else
        DebugSerial.printf("%s\n", config.signer.signupError.message.c_str());
    config.token_status_callback = tokenStatusCallback;
    Firebase.begin(&config, &auth);
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

    while (1)
    {
        if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
        {
            sendDataPrevMillis = millis();

            DebugSerial.printf("Set bool... %s\n", Firebase.RTDB.setBool(&fbdo, F("/test/bool"), count % 2 == 0) ? "ok" : fbdo.errorReason().c_str());

            count++;
        }
    }
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

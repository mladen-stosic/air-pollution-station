/*

@author Mladen Stosic

@brief Air pollution station using WeMos D1 Mini,
PMSA003 and BME280
Data is sent to opensensemap.org approx every 3 minutes

Connections:

! D0 must be connected to RST !

BME280 -> WeMos D1 mini
SCL    -> SCL (PIN D1)
SDK    -> SDK (PIN D2)
GND    -> GND
VCC    -> Vcc

PMS -> WeMos D1 mini
TX  -> RX
RX  -> TX
GND -> GND
VCC -> Vcc

*/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <PMS.h>
#include <ESP8266WiFi.h>
#include <stdint.h>


Adafruit_BME280 bme; // I2C
PMS pms(Serial);
PMS::DATA data;

// Network SSID
const char* ssid = "******";
const char* password = "******";

static String pressure;
static String temperature;
static String humidity;
static uint16_t pm1_0;
static uint16_t pm2_5;
static uint16_t pm10;
static uint16_t pm1_0rd[4];
static uint16_t pm2_5rd[4];
static uint16_t pm10rd[4];

static uint16_t wifiDelay = 0;

// Max Waiting time for WiFi Connection
#define WIFI_TIME_LIMIT 30*500
// Deep sleep time
#define DEEP_SLEEP_TIME 120e6
// 30s to get valid data from PMS
#define DATA_VALID_TIME 30000

// Personal sensor ID obtained from opensensemap.org
//senseBox ID
#define SENSEBOX_ID "**************"
//Sensor IDs
// Temperature - BME280
#define SENSOR1_ID "**************"
// Humidity - BME280
#define SENSOR2_ID "**************"
// Atmospheric pressure - BME280
#define SENSOR3_ID "**************"
// PM 1.0 - PMSA003
#define SENSOR4_ID "**************"
// PM 2.5 - PMSA003
#define SENSOR5_ID "**************"
// PM 10 - PMSA003
#define SENSOR6_ID "**************"

static char server[] = "ingress.opensensemap.org";
static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress myIP(192, 168, 0, 42);
WiFiClient client;

// Send data to the server via WiFi
void sendData(String measurement, String sensorId){

  //Json fill
  String jsonValue = "{\"value\":";
  jsonValue += measurement;
  jsonValue += "}";

  if (client.connect(server, 80)){

    client.print("POST /boxes/"); client.print(SENSEBOX_ID); client.print("/"); client.print(sensorId); client.println(" HTTP/1.1");
    client.print("Host:");
    client.println(server);
    client.print("Authorization:");
    client.println("1cfd32ceb172e447b737c904c2eedf5d2cf9aba15ef65b8dd1015a46cafaa1e1");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print("Content-Length: "); client.println(jsonValue.length());
    client.println();
    //Send data
    client.println(jsonValue);

    waitForServerResponse();

  } 
}

void waitForServerResponse () {
  if (!client.connected()) {
      client.stop();
      // Disable power to sensors
      pms.sleep();
      // Deep sleep for 1 minute
      ESP.deepSleep(DEEP_SLEEP_TIME / 2);
  }
}

// Read values from PMS Air quality sensor
void readPMS() {
  static uint8_t i;
  static uint8_t data_read;
  i = 0;
  data_read = 0;
  while (!data_read){
    pms.requestRead();
    if (pms.readUntil(data)){
      pm1_0rd[i] = data.PM_AE_UG_1_0;
      pm2_5rd[i] = data.PM_AE_UG_2_5;
      pm10rd[i]  = data.PM_AE_UG_10_0;
      i++;
      if (i > 3){
        data_read = 1;
      }
    }
  }
  // Calculate average value 
  pm1_0 = 0;
  pm2_5 = 0;
  pm10  = 0;
  for (i = 0; i < 4; i++){
    pm1_0 += pm1_0rd[i] >> 2;
    pm2_5 += pm2_5rd[i] >> 2;
    pm10  += pm10rd[i] >> 2;
  }
}

// Read values from BME280 sensor
void readBME() {
    temperature = String(bme.readTemperature(), 1);
    pressure = String(bme.readPressure() / 100.0F, 1);
    humidity = String(bme.readHumidity(),0);
}

void setup() {
    // Init hardware
    bme.begin(0x76);
    Serial.begin(9600);

    // Enable power to sensors
    pms.passiveMode();
    pms.wakeUp();

    // Connect to WiFi
    WiFi.hostname("Name");
    WiFi.begin(ssid, password);
    wifiDelay = 0;

    // Wait for wifi connection
    while ((WiFi.status() != WL_CONNECTED) && (wifiDelay < WIFI_TIME_LIMIT)){
        wifiDelay += 500; // Store wifi connection elapsed time 
        delay(500);
        Serial.print(".");
    }
    // If unable to connect to WiFi reset MCU
    if (wifiDelay > WIFI_TIME_LIMIT){
      // Disable power to sensors
      pms.sleep();
      // Deep sleep for 1 minute
      ESP.deepSleep(DEEP_SLEEP_TIME / 2);
    }

    // Wait for 30s to get valid data from PMS Sensor
    delay(DATA_VALID_TIME);
    // TODO Light sleep??

    // Read sensor data
    readBME();
    readPMS();

    // Send values to server via WiFi
    sendData(String(pm1_0), SENSOR4_ID);
    sendData(String(pm2_5), SENSOR5_ID);
    sendData(String(pm10), SENSOR6_ID);
    sendData(temperature, SENSOR1_ID);
    sendData(humidity, SENSOR2_ID);
    sendData(pressure, SENSOR3_ID);
    

    // Disable power to sensors
    pms.sleep();
    // Deep sleep for 2 minutes
    ESP.deepSleep(DEEP_SLEEP_TIME);
}

void loop() {
}
# air-pollution-station
Arduino project

### Hardware used:
WeMos D1 Mini(ESP8266), PMSA003. BME280

### Project description:
Gather data from PMS and BME sensors and send them to opensensemap.org

### Implemetation:
After reset wake up PMS sensor, put it in passive mode. Try to connect to WiFi, if connection fails go to low power mode for 2 minutes.
When connected, wait 30 seconds for PMS Sensor to start sending valid values. Gather 4 different measurements from it and get their average value. This is done to increase accuracy. Gather data from BME Sensor. Send all gathered values to opensensemap.org server. Put PMS sensor to sleep. Enter deep sleep for 3 minutes, when MCU will be reset by internal clock pulling it's RST pin to GND.


#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_AHTX0.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#include "credentials.h"

#define SCREEN_I2C  0x3C
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1


#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"
#define TIMESERVER1 "pool.ntp.org"
#define TIMESERVER2 "time.nis.gov"

#define ONBOARD_LED  2


Adafruit_AHTX0 aht;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert); 
Point datapoint("Room data");


// Function prototypes
void i2c_setup();
void wifi_setup();
void display_setup();
void aht20_setup();
void influxdb_setup();

// ------------------------------------------------------------
void i2c_setup()
{
  Wire.begin();
}
// ------------------------------------------------------------ 


// ------------------------------------------------------------
void wifi_setup()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("OK. ");
  Serial.println(WiFi.localIP());
}
// ------------------------------------------------------------
void display_setup()
{
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_I2C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println(WiFi.localIP());
  display.println("Started");

  display.display();
}
// ------------------------------------------------------------

void aht20_setup()
{
  if (aht.begin()) {
    Serial.println("Found AHT20");
    display.println("AHT20 connected");
    display.display();
  } else {
    Serial.println("Didn't find AHT20");
    for(;;) {}
  } 
}
// ------------------------------------------------------------
void influxdb_setup()
{
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
    for(;;) {}
  }
}
// ------------------------------------------------------------
void setup() 
{
  Serial.begin(115200);
  pinMode(ONBOARD_LED, OUTPUT);

  digitalWrite(ONBOARD_LED, HIGH);
  i2c_setup();
  Serial.println("I2C setup completed");
  wifi_setup();
  Serial.println("WiFi setup completed");
  display_setup();
  Serial.println("Display setup completed");
  aht20_setup();
  Serial.println("AHT20 sensor setup completed");
  influxdb_setup();
  Serial.println("InfluxDB setup completed");

  timeSync(TZ_INFO, TIMESERVER1, TIMESERVER2);
  Serial.println("Time synced with network time servers");

  digitalWrite(ONBOARD_LED, LOW);

}

void loop() 
{
  sensors_event_t humidity_event, temp_event;
  float humidity, temp;

  digitalWrite(ONBOARD_LED, HIGH);
  delay(150);
  digitalWrite(ONBOARD_LED, LOW);
  
  aht.getEvent(&humidity_event, &temp_event);
  humidity = humidity_event.relative_humidity;
  temp = temp_event.temperature;

  display.clearDisplay();
  display.setCursor(0, 0);

  display.setCursor(0,10);
  display.print("Temp: "); display.print(temp); display.println(" C");
  display.setCursor(0,20);
  display.print("Hum:  "); display.print(humidity); display.println(" %");

  Serial.print("Temperature: ");Serial.print(temp);Serial.println(" C");
  Serial.print("Humidity:    ");Serial.print(humidity);Serial.println(" %");
  display.display();

  datapoint.clearFields();
  datapoint.addField("temperature", temp);
  datapoint.addField("humidity", humidity);

  if (!client.writePoint(datapoint)) 
  {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  else
  {
    Serial.println("Data written to InfluxDB\n");
  }

  delay(5000);
}
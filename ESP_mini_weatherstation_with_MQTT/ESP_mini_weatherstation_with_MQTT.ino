
/**************************************************************
 _    _       _       _____      _  _  _             _     _
\ \  / /     (_)     (____ \    | || || |           | |   | |
 \ \/ / ____  _ ____  _   \ \   | || || | ___   ____| | _ | |
  )  ( |    \| |  _ \| |   | |  | ||_|| |/ _ \ / ___) |/ || |
 / /\ \| | | | | | | | |__/ /   | |___| | |_| | |   | ( (_| |
/_/  \_\_|_|_|_|_| |_|_____/     \______|\___/|_|   |_|\____|

***************************************************************
 *! Project    : Mini Weather Station with OLED Display
 * Purpose    : Smart Systems & IoT Automation
 *              - Displaying sensor data (Temp, Humidity,
 *                Altitude, Lux) on OLED
 *              - MQTT communication with Node-RED
 *              - Multi-page OLED interface with button
 *              - LED control and status icon display
 **Author     : XminD Team (x.mind4world@gmail.com)
 * Date       : 2025-05-06
 *
 **Instagram  : https://instagram.com/x_mindworld
 **Telegram   : https://t.me/x_mindworld
 ***************************************************************/

// --- Include necessary libraries ---
#include <ArduinoJson.h>
#include "wifi_mqtt.h"
#include <Wire.h>
#include <SPI.h>
#include "DHT.h"
#include <BH1750.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>

// --- Define parameters ---
#define DHTPIN 19
#define DHTTYPE DHT11
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 5
#define SCREEN_ADDRESS 0x3D //? Change this to your OLED's I2C address if needed (0x3C is common)
#define BUTTON_PAGE 4
#define BUTTON_LED 15

byte led = 14;
unsigned long previousMillis = millis();
float temp, humidity, altitude, lux;
int screenIndex = 0;
bool ledState = false;

// --- Initialize sensor and display objects ---
BH1750 lightMeter;
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Print sensor values to Serial Monitor ---
void printSensorValue()
{
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  Serial.print("Altitude: ");
  Serial.print(altitude);
  Serial.println(" Meter");
  Serial.print("Illumination: ");
  Serial.print(lux);
  Serial.println(" Lux");
}

// --- Send sensor values to MQTT broker ---
void sendSensorValuesMQTT()
{
  DynamicJsonDocument doc(1024); //! Create a JSON document using ArduinoJson.org WebSite and library.
  doc["temperature"] = temp;
  doc["humidity"] = humidity;
  doc["altitude"] = altitude;
  doc["lux"] = lux;

  char buffer[256];
  serializeJson(doc, buffer);
  client.publish("weatherstation", buffer); //? Publish the JSON data to the MQTT topic "weatherstation"
}

// --- Display sensor values on OLED ---
void displaySensorValues()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(temp);
  display.println(" C");

  display.print("Hum: ");
  display.print(humidity);
  display.println(" %");

  display.print("Alt: ");
  display.print(altitude);
  display.println(" m");

  display.print("Lux: ");
  display.print(lux);
  display.println(" lx");

  display.display();
}

// --- OLED Pages ---
void showPage1()
{
  display.clearDisplay();
  display.setFont(&FreeSans9pt7b);
  display.setCursor(0, 20);
  display.print("Temp: ");
  display.print(temp);
  display.println(" C");

  display.setCursor(0, 50);
  display.print("Hum: ");
  display.print(humidity);
  display.println(" %");

  display.display();
}

void showPage2()
{
  display.clearDisplay();
  display.setFont(&FreeSans9pt7b);
  display.setCursor(0, 20);
  display.print("Alt: ");
  display.print(altitude);
  display.println(" m");

  display.setCursor(0, 50);
  display.print("Lux: ");
  display.print(lux);
  display.println(" lx");

  display.display();
}

void showPage3()
{
  display.clearDisplay();
  display.setFont(&FreeSans9pt7b);
  display.setCursor(0, 30);
  display.print("LED: ");

  if (ledState)
  {
    display.fillCircle(80, 30, 6, SSD1306_WHITE);
  }
  else
  {
    display.drawCircle(80, 30, 6, SSD1306_WHITE);
  }

  display.display();
}

// --- Setup Wifi, MQTT, OLED & Sensors ---
void setup()
{

  Serial.begin(9600); //! for ESP32 9600 is better than 115200;
  pinMode(led, OUTPUT);
  pinMode(BUTTON_PAGE, INPUT_PULLUP);
  pinMode(BUTTON_LED, INPUT_PULLUP);
  Wire.begin();       //? Initialize I2C communication
  dht.begin();        //? Initialize DHT sensor
  lightMeter.begin(); //? Initialize BH1750 sensor
  connectAP();
  client.setServer(mqtt_server, 1883); //? Set MQTT server
  client.setCallback(callback);        //? Set MQTT callback function

  //* Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; //? Stop here if failed
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Display Ready...");
  display.display();
  delay(1000);
}

// --- Main loop ---
void loop()
{

  //* Check MQTT connection and reconnect if necessary
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  //* Read sensor values every 10 seconds
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 10000)
  {
    temp = dht.readTemperature();
    humidity = dht.readHumidity();
    altitude = random(20, 800); //? Simulated altitude value
    lux = lightMeter.readLightLevel();
    previousMillis = currentMillis;
    printSensorValue();     //? Print sensor values to Serial Monitor
    sendSensorValuesMQTT(); //? Send sensor values to MQTT broker
  }

  //* Change display page with button press
  static bool lastPageButtonState = HIGH;
  bool currentPageButtonState = digitalRead(BUTTON_PAGE);
  if (lastPageButtonState == HIGH && currentPageButtonState == LOW)
  {
    screenIndex = (screenIndex + 1) % 3;
    delay(200);
  }
  lastPageButtonState = currentPageButtonState;

  //* Local LED control with button press
  static bool lastLedButtonState = HIGH;
  bool currentLedButtonState = digitalRead(BUTTON_LED);
  if (screenIndex == 2 && lastLedButtonState == HIGH && currentLedButtonState == LOW)
  { //! It works only on the LED page(3rd page)
    ledState = !ledState;
    digitalWrite(led, ledState);
    delay(200);
  }
  lastLedButtonState = currentLedButtonState;

  //* Display the current page based on screenIndex
  switch (screenIndex)
  {
  case 0:
    showPage1();
    break;
  case 1:
    showPage2();
    break;
  case 2:
    showPage3();
    break;
  }
}

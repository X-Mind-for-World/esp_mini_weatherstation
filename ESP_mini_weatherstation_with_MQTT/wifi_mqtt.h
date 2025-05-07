/*

! This code is part of the ESP32 Mini Weatherstation and LED control with MQTT project.
*- This code connects an ESP32 to a WiFi network and an MQTT broker to control an LED and send sensor
*data to the broker. The LED state can be controlled via MQTT messages from Node-RED or any MQTT client.
*- Data can be sent in JSON format, and the ESP32 can also receive commands to turn the LED on or off.
*- You can see data changes in real-time on Grafana dashboard.
*- This code is designed to be used with the Arduino IDE/PlatformIO.

*/

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "credentials.h" //? Include your WiFi credentials header file.

extern byte led;      //? Declare the LED pin as external.
extern bool ledState; //? Declare the LED state as external.

// --- MQTT parameters ---
String clientID = "ESP-";
const char *mqtt_server = "localhost";    //! Replace with your MQTT broker IP address or hostname.
const char *mqtt_user = "admin";          //! Replace with your MQTT username.
const char *mqtt_password = "myPassword"; //! Replace with your MQTT password.
WiFiClient espClient;                     //? Create a WiFi client object.
PubSubClient client(espClient);           //? Create a PubSubClient object.

// --- Function declarations
void reconnect() //? Function to reconnect to MQTT broker
{
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        clientID = "ESP-";
        clientID += String(random(0xffff), HEX);
        if (client.connect(clientID.c_str(), mqtt_user, mqtt_password))
        {
            Serial.println("connect to MQTT");
            client.subscribe("fan"); //? Subscribe to the "fan" topic that controls the LED state from Node-RED.
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void callback(char *topic, byte *message, unsigned int length) //? Callback function to handle incoming MQTT messages
{
    // Serial.print("Message arrived on topic: ");
    // Serial.print(topic);
    // Serial.print(". Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++)
    {
        messageTemp += (char)message[i];
    }

    if (String(topic) == "fan")
    {
        if (messageTemp == "true")
        {
            digitalWrite(led, HIGH);
            ledState = true;
        }
        else
        {
            digitalWrite(led, LOW);
            ledState = false;
        }
    }
}

void connectAP()
{ //? Function to connect to WiFi network.
    Serial.println("Connect to WiFi");
    WiFi.begin(ssid, password);
    byte cnt = 0;

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
        cnt++;

        if (cnt > 30)
        { //? If it takes more than 30 seconds to connect, restart the ESP32.
            ESP.restart();
        }
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP()); //? Print the IP address assigned to the ESP32.
}
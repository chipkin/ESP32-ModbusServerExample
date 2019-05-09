/**
 * ESP32 Modbus Server Example 
 * --------------------------------------
 * In this project we are using the CAS Modbus stack (https://store.chipkin.com) to generate a 
 * simple Modbus TCP server with three registers 40001-40003. Depening on the state of the registers
 * the built in LED wil on/off/blink. A Modbus Client/Master application (such as CAS Modbus Scanner)
 * can be used to read/write to these registers and to change the values.
 * 
 * 40001 = LED Mode (1 = Off, 2 = On, 3 = Blink)
 * 40002 = LED Blink Speed in milliseconds (Default: 200)
 * 40003 = Spare 
 * 
 * Created by: Steven Smethurst 
 * Created on: May 9, 2019 
 * Last updated: May 9, 2019 
 * 
 */
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// Application Version
// -----------------------------
const uint32_t APPLICATION_VERSION_MAJOR = 0;
const uint32_t APPLICATION_VERSION_MINOR = 0;
const uint32_t APPLICATION_VERSION_PATCH = 1;

// Application settings
// -----------------------------
// ToDo: Update these prameters with your local wifi SSID and password.
const char* APPLICATION_WIFI_SSID = "---YOUR SSID---";
const char* APPLICATION_WIFI_PASSWORD = "--YOUR PASSWORD---";
const uint32_t APPLICATION_LED_PIN = LED_BUILTIN;
const long APPLICATION_LED_BLINK_RATE_MS = 250; // interval at which to blink (milliseconds)
const uint32_t APPLICATION_SERIAL_BAUD_RATE = 115200;

// LED
// -----------------------------
// LED Mode
const uint16_t LED_MODE_OFF = 1;
const uint16_t LED_MODE_ON = 2;
const uint16_t LED_MODE_BLINK = 3;
uint32_t gLEDMode = LED_MODE_BLINK;
const uint16_t LED_MODE_STATE_COUNT = LED_MODE_BLINK;

// LED State
uint16_t gLEDState = LOW;

void setup()
{
    // Hardware setup
    // ==========================================
    // Set the digital pin as output:
    pinMode(APPLICATION_LED_PIN, OUTPUT);
    Serial.begin(APPLICATION_SERIAL_BAUD_RATE);
    delay(3000); // Let the Serial port connect, and give us some time to reprogram if needed.

    Serial.print("FYI: ESP32 CAS BACnet Stack example version: ");
    Serial.print(APPLICATION_VERSION_MAJOR);
    Serial.print(".");
    Serial.print(APPLICATION_VERSION_MINOR);
    Serial.print(".");
    Serial.println(APPLICATION_VERSION_PATCH);
    uint64_t chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC address(length: 6 bytes).
    Serial.printf("FYI: ESP32 Chip ID: %04X, (%08X)\n", (uint16_t)(chipid >> 32), (uint32_t)chipid);

    // WiFi connection
    // ==========================================
    Serial.println("FYI: Connecting to wifi...");
    WiFi.begin(APPLICATION_WIFI_SSID, APPLICATION_WIFI_PASSWORD);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("FYI: Connected to ");
    Serial.println(APPLICATION_WIFI_SSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Subnet mask: ");
    Serial.println(WiFi.subnetMask());

    // ToDo: Modbus stack stuff
}

void loop()
{
    // ToDo: Modbus stack loop

    unsigned long currentMillis = millis();

    static unsigned long lastMemoryCheck = 0;
    if (lastMemoryCheck < currentMillis) {
        lastMemoryCheck = currentMillis + 30000;
        Serial.print("FYI: FreeHeap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.print(" / ");
        Serial.print(ESP.getHeapSize());
        Serial.print(" (");
        Serial.print(((float)ESP.getFreeHeap() / (float)ESP.getHeapSize()) * 100.0f);
        Serial.println(" %)");
    }

    switch (gLEDMode) {
        default:
        case LED_MODE_OFF:
            if (gLEDState != LOW) {
                gLEDState = LOW;
                digitalWrite(APPLICATION_LED_PIN, gLEDState);
            }
            break;
        case LED_MODE_ON:
            if (gLEDState != HIGH) {
                gLEDState = HIGH;
                digitalWrite(APPLICATION_LED_PIN, gLEDState);
            }
            break;
        case LED_MODE_BLINK:
            // check to see if it's time to blink the LED; that is, if the difference
            // between the current time and last time you blinked the LED is bigger than
            // the interval at which you want to blink the LED.
            static long lastBlink = 0;
            if (currentMillis - lastBlink >= APPLICATION_LED_BLINK_RATE_MS) {
                // save the last time you blinked the LED
                lastBlink = currentMillis;

                // if the LED is off turn it on and vice-versa:
                if (gLEDState == LOW) {
                    gLEDState = HIGH;
                    Serial.print("+");
                } else {
                    gLEDState = LOW;
                    Serial.print("-");
                }

                // set the LED with the ledState of the variable:
                digitalWrite(APPLICATION_LED_PIN, gLEDState);
            }
            break;
    }
}
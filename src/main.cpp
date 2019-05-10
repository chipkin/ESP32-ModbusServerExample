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
 * 40003 = LED Current state (0 = OFF, 1 = ON)
 * 40004 = Count. This value incurments once a second and loops every ~18 hours. 
 * 
 * Created by: Steven Smethurst 
 * Created on: May 9, 2019 
 * Last updated: May 9, 2019 
 * 
 */
#include <Arduino.h>
#include <WiFi.h>

// Missing file
// This file is part of the CAS Modbus stack and is not included in this repo
// More information about the CAS Modbus stack can be found here !!!! TODO !!!!
#include <CASModbusAdapter.h> // Loads all the Modbus functions.
#include <ChipkinEndianness.h> // Helps with OS depenent Endianness

// Application Version
// ==========================================
const uint32_t APPLICATION_VERSION_MAJOR = 0;
const uint32_t APPLICATION_VERSION_MINOR = 0;
const uint32_t APPLICATION_VERSION_PATCH = 1;

// Application settings
// ==========================================
// ToDo: Update these prameters with your local wifi SSID and password.
const char* APPLICATION_WIFI_SSID = "---YOUR SSID---";
const char* APPLICATION_WIFI_PASSWORD = "---YOUR PASSWORD---";

const uint32_t APPLICATION_LED_PIN = LED_BUILTIN;
const uint16_t APPLICATION_LED_BLINK_RATE_MS = 250; // interval at which to blink (milliseconds)
const uint32_t APPLICATION_SERIAL_BAUD_RATE = 115200;
const uint8_t APPLICATION_MODBUS_SLAVE_ADDRESS = 1;
const uint16_t APPLICATION_MODBUS_TCP_PORT = 502;

// Modbus Constance
const uint32_t MODBUS_TYPE_TCP = 2;
const uint8_t MODBUS_FUNCTION_03_HOLDING_REGISTERS = 3;
const uint8_t MODBUS_FUNCTION_06_WRITE_SINGLE_REGISTERS = 6;

// Database
// ==========================================
const uint16_t DATABASE_SIZE = 10;
uint16_t gDatabase[DATABASE_SIZE];

const uint16_t DATABASE_OFFSET_LED_MODE = 0;
const uint16_t DATABASE_OFFSET_LED_BLINK_SPEED = 1;
const uint16_t DATABASE_OFFSET_LED_STATE = 2;
const uint16_t DATABASE_OFFSET_COUNT = 3;

// LED
// ==========================================
// LED Mode
const uint16_t LED_MODE_OFF = 1;
const uint16_t LED_MODE_ON = 2;
const uint16_t LED_MODE_BLINK = 3;
const uint16_t LED_MODE_STATE_COUNT = LED_MODE_BLINK;

// Globls
// ==========================================
WiFiClient gClient;
WiFiServer gServer;

// Callback functions
// ==========================================
bool sendModbusMessage(const unsigned short connectionId, const unsigned char* payload, const unsigned short payloadSize);
unsigned int recvModbusMessage(unsigned short& connectionId, unsigned char* payload, unsigned short maxPayloadSize);
unsigned long currentTime();
bool setModbusValue(const unsigned char slaveAddress, const unsigned char function, const unsigned short startingAddress, const unsigned short length, const unsigned char* data, const unsigned short dataSize, unsigned char* errorCode);
bool getModbusValue(const unsigned char slaveAddress, const unsigned char function, const unsigned short startingAddress, const unsigned short length, unsigned char* data, const unsigned short maxPayloadSize, unsigned char* errorCode);

void setup()
{
    // Hardware setup
    // ==========================================
    // Set the digital pin as output:
    pinMode(APPLICATION_LED_PIN, OUTPUT);
    Serial.begin(APPLICATION_SERIAL_BAUD_RATE);
    delay(3000); // Let the Serial port connect, and give us some time to reprogram if needed.

    Serial.print("FYI: ESP32 CAS Modbus Stack example version: ");
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

        if (millis() > 1000 * 30) {
            Serial.print("\n\nError: Can not connect to the WiFi. Restarting and trying again\n\n");
            ESP.restart();
            return;
        }
    }
    Serial.println("");
    Serial.print("FYI: Connected to ");
    Serial.println(APPLICATION_WIFI_SSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Subnet mask: ");
    Serial.println(WiFi.subnetMask());

    // Set up the TCP stack
    gServer = WiFiServer(APPLICATION_MODBUS_TCP_PORT);
    gServer.begin();
    delay(1000); // Wait a second for the port to be fully setup.

    // Database defaults
    gDatabase[DATABASE_OFFSET_LED_MODE] = LED_MODE_BLINK;
    gDatabase[DATABASE_OFFSET_LED_BLINK_SPEED] = APPLICATION_LED_BLINK_RATE_MS;
    gDatabase[DATABASE_OFFSET_LED_STATE] = LOW;
    gDatabase[DATABASE_OFFSET_COUNT] = 0;

    // Set up the CAS BACnet stack.
    // ==========================================
    LoadModbusFunctions();
    Serial.print("FYI: CAS Modbus Stack version: ");
    Serial.print(fpModbusStack_GetAPIMajorVersion());
    Serial.print(".");
    Serial.print(fpModbusStack_GetAPIMinorVersion());
    Serial.print(".");
    Serial.println(fpModbusStack_GetAPIPatchVersion());
    Serial.print(".");
    Serial.println(fpModbusStack_GetAPIBuildVersion());

    fpModbusStack_Init(MODBUS_TYPE_TCP, sendModbusMessage, recvModbusMessage, currentTime);
    fpModbusStack_SetSlaveId(APPLICATION_MODBUS_SLAVE_ADDRESS);
    fpModbusStack_RegisterGetValue(getModbusValue);
    fpModbusStack_RegisterSetValue(setModbusValue);
}

void loop()
{
    // Modbus stack loop
    int modbusStackStatus = fpModbusStack_Loop();
    if (modbusStackStatus != 1) {
        Serial.printf("modbusStackStatus=%d\n", modbusStackStatus);
    }

    unsigned long currentMillis = millis();

    static unsigned long nextMemoryCheck = 0;
    if (nextMemoryCheck < currentMillis) {
        nextMemoryCheck = currentMillis + 30000;
        Serial.print("FYI: FreeHeap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.print(" / ");
        Serial.print(ESP.getHeapSize());
        Serial.print(" (");
        Serial.print(((float)ESP.getFreeHeap() / (float)ESP.getHeapSize()) * 100.0f);
        Serial.println(" %)");
    }

    static unsigned long nextCountUpdate = 0;
    if (nextCountUpdate < currentMillis) {
        nextCountUpdate = currentMillis + 1000;
        gDatabase[DATABASE_OFFSET_COUNT]++;
    }

    switch (gDatabase[DATABASE_OFFSET_LED_MODE]) {
        default:
        case LED_MODE_OFF:
            if (gDatabase[DATABASE_OFFSET_LED_STATE] != LOW) {
                gDatabase[DATABASE_OFFSET_LED_STATE] = LOW;
                digitalWrite(APPLICATION_LED_PIN, gDatabase[DATABASE_OFFSET_LED_STATE]);
            }
            break;
        case LED_MODE_ON:
            if (gDatabase[DATABASE_OFFSET_LED_STATE] != HIGH) {
                gDatabase[DATABASE_OFFSET_LED_STATE] = HIGH;
                digitalWrite(APPLICATION_LED_PIN, gDatabase[DATABASE_OFFSET_LED_STATE]);
            }
            break;
        case LED_MODE_BLINK:
            // check to see if it's time to blink the LED; that is, if the difference
            // between the current time and last time you blinked the LED is bigger than
            // the interval at which you want to blink the LED.
            static long nextBlink = 0;
            if (nextBlink < currentMillis) {
                // save the last time you blinked the LED
                nextBlink = currentMillis + gDatabase[DATABASE_OFFSET_LED_BLINK_SPEED];

                // if the LED is off turn it on and vice-versa:
                if (gDatabase[DATABASE_OFFSET_LED_STATE] == LOW) {
                    gDatabase[DATABASE_OFFSET_LED_STATE] = HIGH;
                    Serial.print("+");
                } else {
                    gDatabase[DATABASE_OFFSET_LED_STATE] = LOW;
                    Serial.print("-");
                }

                // set the LED with the ledState of the variable:
                digitalWrite(APPLICATION_LED_PIN, gDatabase[DATABASE_OFFSET_LED_STATE]);
            }
            break;
    }
}

// Callback functions
// ===========================
bool sendModbusMessage(const unsigned short connectionId, const unsigned char* payload, const unsigned short payloadSize)
{
    if (payload == NULL || payloadSize <= 0) {
        return false; // Nothing to do.
    }

    if (!gClient) {
        Serial.println("Error: No client, Can not send message");
        return false;
    }
    if (!gClient.connected()) {
        Serial.println("Error: Not connected to a client, Can not send message");
        return false;
    }

    gClient.write(payload, payloadSize);
    Serial.printf("FYI: sendModbusMessage bytes=[%d] \n", payloadSize);
    // Serial.printf("Message first 12 bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ... \n", payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7], payload[8], payload[9], payload[10], payload[11]);
    return true;
}
unsigned int recvModbusMessage(unsigned short& connectionId, unsigned char* payload, unsigned short maxPayloadSize)
{
    if (payload == NULL || maxPayloadSize <= 0) {
        return 0; // Nothing to do.
    }

    if (!gClient) {
        gClient = gServer.available();
        if (!gClient.connected()) {
            return 0;
        }
        Serial.printf("FYI: New TCP connection\n");
        Serial.print("FYI: RemoteIP: ");
        Serial.print(gClient.remoteIP());
        Serial.print(", Port: ");
        Serial.println(gClient.remotePort());
    }
    if (!gClient.connected()) {
        Serial.printf("FYI: Disconnected TCP connection\n");
        return 0;
    }

    if (gClient.available() > maxPayloadSize) {
        // Too many bytes for our buffer
        gClient.flush();
        return false;
    }

    size_t recved = gClient.readBytes(payload, maxPayloadSize);
    if (recved > 0) {
        Serial.printf("FYI: recvModbusMessage bytes=[%d] \n", recved);
        // Serial.printf("Message first 12 bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ... \n", payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7], payload[8], payload[9], payload[10], payload[11]);
    }
    return recved;
}
unsigned long currentTime()
{
    return millis() / 1000;
}
bool setModbusValue(const unsigned char slaveAddress, const unsigned char function, const unsigned short startingAddress, const unsigned short length, const unsigned char* data, const unsigned short dataSize, unsigned char* errorCode)
{
    Serial.printf("FYI: setModbusValue slaveAddress=[%d], function=[%d], startingAddress=[%d], length=[%d]\n", slaveAddress, function, startingAddress, length);

    if (function == MODBUS_FUNCTION_06_WRITE_SINGLE_REGISTERS && startingAddress + length < DATABASE_SIZE) {
        memcpy(((uint8_t*)gDatabase) + startingAddress * sizeof(uint16_t), data, length * sizeof(uint16_t));
        for (uint16_t offset = 0; offset < length; offset++) {
            Serial.printf("   %d = %d (0x%04X)\n", 40001 + startingAddress + offset, gDatabase[startingAddress + offset], gDatabase[startingAddress + offset]);
        }
        return true;
    }

    Serial.println("Error: Value not written. Malformed request");
    return false;
}
bool getModbusValue(const unsigned char slaveAddress, const unsigned char function, const unsigned short startingAddress, const unsigned short length, unsigned char* data, const unsigned short maxPayloadSize, unsigned char* errorCode)
{
    Serial.printf("FYI: getModbusValue slaveAddress=[%d], function=[%d], startingAddress=[%d], length=[%d]\n", slaveAddress, function, startingAddress, length);

    if (function == MODBUS_FUNCTION_03_HOLDING_REGISTERS && startingAddress + length < DATABASE_SIZE) {
        memcpy(data, ((uint8_t*)gDatabase) + startingAddress * sizeof(uint16_t), length * sizeof(uint16_t));
        for (uint16_t offset = 0; offset < length; offset++) {
            Serial.printf("   %d = %d (0x%04X)\n", 40001 + startingAddress + offset, gDatabase[startingAddress + offset], gDatabase[startingAddress + offset]);
        }
        return true;
    }

    Serial.println("Error: Read request not supported. Malformed request");
    return false;
}
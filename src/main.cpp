#include <Arduino.h>

#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// Don't use src/config.h for privacy reasons
#include "../../config.h"

//
// ESP32 Arduino
// NTP client with 7 segment display
// 20-jan-2018 - 10:00
//

//Prototypes
void BlankAll();
void DisplayBlank();
void DisplayByte(int Num);

//
// SPI 8 digit 7 segment led module
// https://www.dfrobot.com/wiki/index.php/3-Wire_LED_Module_(SKU:DFR0090)
//
// Datasheet 74HC595
// http://www.ti.com/lit/ds/symlink/sn74hc595.pdf
//
// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
//

// Pin connected to latch pin (ST_CP) of 74HC595
const int latchPin = 19;
// Pin connected to clock pin (SH_CP) of 74HC595
const int clockPin = 21;
// Pin connected to Data in (DS) of 74HC595
const int dataPin = 22;

// 7 segment table 0..9 and blank
byte Tab[] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90, 0xff};

// NTP client
WiFiUDP ntpUDP;

// UTC + 1
const int Offset = 1;

NTPClient timeClient(ntpUDP, "ntp.xs4all.nl", 0, 60000);

// Global variables
bool noconnection = false;
int Minutes;
int NewMinutes = -1;
char buffer[5];

//
// WiFi event handler
//
void WiFiEvent(WiFiEvent_t event) 
{
    Serial.printf("[WiFi-event] event: %d  - ", event);
    switch(event) 
    {
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.print("WiFi connected - ");
            Serial.print("IP address: "); Serial.println(WiFi.localIP());
            noconnection = false;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("WiFi lost connection");
            WiFi.disconnect();
            WiFi.begin(ssid, password);
            noconnection = true;
            break;
        case SYSTEM_EVENT_STA_START:
            Serial.println("ESP32 station start");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("ESP32 station connected to AP");
            break;
    }
}

//
// SetUp
//
void setup()
{
    // Init GPIO direction
    pinMode(BUILTIN_LED, OUTPUT);
    digitalWrite(BUILTIN_LED, LOW);
    pinMode(latchPin, OUTPUT);
    pinMode(dataPin, OUTPUT);  
    pinMode(clockPin, OUTPUT);
    // Blank display during start up
    BlankAll();
    // Init Serial port
 	Serial.begin(115200);
    // Init WiFI
	WiFi.enableAP(false);
    // Handle WiFi event
    WiFi.onEvent(WiFiEvent);          
    WiFi.mode(WIFI_STA);    
    // WiFi connect
    WiFi.begin(ssid, password);
    // wait 60 S before reboot
    int trying = 60;
    while (WiFi.status() != WL_CONNECTED) 
    {
        Serial.print(".");
        delay(1000);
        if (trying == 0)
            ESP.restart();
        else
            trying--;
    }
    timeClient.begin();
    timeClient.setTimeOffset(Offset * 3600);
}

//
// Endless loop
//
void loop() 
{
    timeClient.update();
    // H:M to 7 segment display   
    // turn off the output so the LEDs don't light up
    digitalWrite(latchPin, LOW);
    // Display Seconds
    DisplayByte(timeClient.getSeconds());
     // if noconnection E else 1 segment Blank
    if (noconnection)
        shiftOut(dataPin, clockPin, MSBFIRST, 0x86);
    else
        DisplayBlank();
    // Display Minutes
    DisplayByte(timeClient.getMinutes());
    // if noconnection E else 1 segment Blank
    if (noconnection)
        shiftOut(dataPin, clockPin, MSBFIRST, 0x86);
    else
        DisplayBlank();
    // Display Hours
    DisplayByte(timeClient.getHours());
    // turn on the output so the LEDs can light up:
    digitalWrite(latchPin, HIGH);
    Minutes = timeClient.getMinutes();
    // Check updated Minutes
    if (Minutes != NewMinutes)
    {
        NewMinutes = Minutes;
        // H:M to serial port
        sprintf(buffer, "%02d:%02d", timeClient.getHours(), timeClient.getMinutes());
        Serial.println(buffer);
    }
    delay(100);
}

//
// Blank all digits
//
void BlankAll()
{
    digitalWrite(latchPin, LOW);
    for (int x = 0; x < 8; x++)
        shiftOut(dataPin, clockPin, MSBFIRST, Tab[10]);
    digitalWrite(latchPin, HIGH);
}

//
// Display 1 digit blank
//
void DisplayBlank()
{
    shiftOut(dataPin, clockPin, MSBFIRST, Tab[10]);
}

//
// Display Byte
//
void DisplayByte(int Num)
{
    byte H = Num / 10;
    byte L = Num % 10;
    shiftOut(dataPin, clockPin, MSBFIRST, Tab[L]);
    shiftOut(dataPin, clockPin, MSBFIRST, Tab[H]);
}

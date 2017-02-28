// Libraries
#include <Wire.h>
/* #include "RTClib.h" */

#include <SPI.h>
#include "LedMatrix.h"


#define NUMBER_OF_DEVICES 1
#define CS_PIN 4
LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CS_PIN);

#include <Time.h>
#include <Timezone.h>

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>

//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);
TimeChangeRule *tcr;        //pointer to the time change rule, use to get the TZ abbrev
time_t utc;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,"de.pool.ntp.org");

// Local includes
/* #include "pindefs.h" */
#include "otherdefs.h"

// Customizable options
/* #include "english.h" // exchange this for the language you need */
#include "deutsch.h" // exchange this for the language you need
boolean blink_enable = false;
boolean blinknow = false;
#define FREQ_DISPLAY 1000 // Hz
#define FREQ_TIMEUPDATE 2 // Hz
//unsigned long check_interval = 500; // time update rate
//#define refresh_rate 2560 // display refresh rate in microseconds

/* volatile int cols[]={ // PC0,PD4,PB6!,PB3,PD5,PB4,PC2,PC3 */
  /* MTX_COL1,MTX_COL2,MTX_COL3,MTX_COL4,MTX_COL5,MTX_COL6,MTX_COL7,MTX_COL8}; // ON=LOW */
/* volatile int rows[]={ // PB2,PC1,PD7,PB5,PD2,PD6,PD3,PB7! */
  /* MTX_ROW1,MTX_ROW2,MTX_ROW3,MTX_ROW4,MTX_ROW5,MTX_ROW6,MTX_ROW7,MTX_ROW8}; // ON=HIGH */

enum ClockMode {
  NORMAL,
  SET_MIN,
  SET_HRS,
  END,
};
ClockMode clockmode = NORMAL;

volatile char disp[8]={
  0B11111111,
  0B11111111,
  0B11111111,
  0B11111111,
  0B11111111,
  0B11111111,
  0B11111111,
  0B11111111,
};

char testdisp[8]={
  0B11111111,
  0B11111111,
  0B11111111,
  0B11111111,
  0B11111111,
  0B11111111,
  0B11111111,
  0B11111111,
};

// RTC stuff
/* RTC_DS1307 rtc; */
unsigned long disp_sec;
unsigned long disp_min;
unsigned long disp_hrs;

volatile boolean updatenow = false;

boolean buttonState = LOW;
unsigned long buttonMillis = 0;
boolean buttonHandled = true;

bool NTPupdated = false;
unsigned int updateInterval = 10000;
unsigned int lastUpdate = 0;
unsigned int blinkInterval = 1000;
unsigned int lastBlink = 0;

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    // put your setup code here, to run once:


    WiFiManager wifiManager;
    wifiManager.autoConnect("WaschmaschineAP");
    /* WiFiConnected = true; */

    ArduinoOTA.setHostname("WaschmaschineOTA");
    ArduinoOTA.onStart([]() {
            Serial.println("OTA Start");
            });
    ArduinoOTA.onEnd([]() {
            Serial.println("\nOTA End");
            });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
            });
    ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("OTA Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
            });
    ArduinoOTA.begin();

    /* broadcastIp = ~WiFi.subnetMask() | WiFi.gatewayIP(); */

    timeClient.begin();
    ledMatrix.init();
    ledMatrix.clear();
}


void loop() {
    ArduinoOTA.handle();

    if((millis()-lastUpdate)>updateInterval)
    {
        lastUpdate = millis();
        NTPupdated = timeClient.update();
        if(CE.utcIsDST(timeClient.getEpochTime()))
        {
            timeClient.setTimeOffset(7200+150);
        }
        else
        {
            timeClient.setTimeOffset(3600+150);
        }
    }


    if((millis()-lastBlink)>blinkInterval)
    {
        updatenow = true;
        lastBlink = millis();
        if(updatenow) {
            updateTime();
            prepareDisplay();
            updatenow = false;
        }
    }

}

void updateTime() {
  // Adjust 2.5 minutes = 150 seconds forward
  // So at 12:03 it already reads "five past 12"
  /* DateTime now = rtc.now().unixtime() + 150; */
  /* DateTime now = timeClient.getEpochTime() + 150; */

  disp_sec = timeClient.getSeconds();
  disp_min = timeClient.getMinutes();
  disp_hrs = timeClient.getHours();

  disp_min /= 5;

  if(disp_min >= min_offset)
    ++disp_hrs %= 12;
  else
    disp_hrs   %= 12;
}

void prepareDisplay() {
    blinknow = !blinknow;
    ledMatrix.clear();
    Serial.println('*');
    FOR_ALLROWS {
        disp[r]=B00000000;
        FOR_ALLCOLS {
            /* if((clockmode != SET_MIN || !blinknow)) */
                /* disp[r] |= minutes[disp_min][r] & (B10000000 >> c); */
            /* if((clockmode != SET_HRS || !blinknow)) */
                /* disp[r] |= hours  [disp_hrs][r] & (B10000000 >> c); */
            /* if(clockmode == NORMAL && blink_enable && !blinknow) */
                /* disp[r] |= blinky[r]; */
            disp[r] |= minutes[disp_min][r] & (B10000000 >> c);
            disp[r] |= hours  [disp_hrs][r] & (B10000000 >> c);
            if(disp[r] & (B10000000 >> c))
                ledMatrix.setPixel(r,7-c);
            if(blinknow)
                ledMatrix.setPixel(1,0);
        }
        Serial.println(disp[r], HEX);
    }
    ledMatrix.commit();
}

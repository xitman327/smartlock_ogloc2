#include <Arduino.h>
#include <RTClib.h>

bool rtcok;
RTC_DS1307 rtc;

void init_rtc(){
    if (!rtc.begin()) {
        Serial.println("RTC ERROR");
    }
    else {
        Serial.println("RTC OK");
        rtcok = 1;
    }
}
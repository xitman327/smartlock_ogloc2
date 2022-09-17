#include <Arduino.h>


#include <Tone32.h>

#include "FS.h"
#include "SPIFFS.h"
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <CSV_Parser.h>

char * csv_str = "cardid,cardname,date,cardact\n";
CSV_Parser cp(csv_str, "xsss");

int32_t *cardid = (int32_t*)cp["cardid"];
char **cardname = (char**)cp["cardname"];
char **carddate = (char**)cp["date"];
char **cardact= (char**)cp["cardact"];

void read_csv(){
    File file_csv = SPIFFS.open("/log.csv", FILE_READ);
    if (file_csv) {
        file_csv.readBytes(csv_str, file_csv.size());
    }
    file_csv.close();
    ~file_csv;
}

void write_csv(){
    SPIFFS.remove("/log.csv");
    File file_csv = SPIFFS.open("/log.csv", FILE_WRITE, true);
    if (file_csv) {
        file_csv.print(csv_str);
    }
    file_csv.close();
    ~file_csv;
}


#define tone_pin 12
#define tn_channel 0

#define home_shucle 34
#define status_led 2

bool beep_valid, beep_invalid;

DynamicJsonDocument jobact(1000);


//do tone
void tone_valid(){
    tone(tone_pin, NOTE_D6, 500, tn_channel);
    noTone(tone_pin, tn_channel);
}
void tone_reject(){
    tone(tone_pin, NOTE_E3, 500, tn_channel);
    noTone(tone_pin, tn_channel);
}

#include "wifimgr.h"
#include "rtctime.h"
#include "stepper.h"
#include "battery.h"
#include "cards.h"
#include "srv.h"


// The setup() function runs once each time the micro-controller starts
void setup()
{
    analogSetAttenuation(ADC_11db);
    pinMode(bk_bat, OUTPUT);
    handle_batteries();

    //setup wifi
    //wifimanager will handle wifi if no ap found
    //set hostname so we enter the webinterface with smartlock.local instead of ip
    //hotspot will be a feature
    delay(100);
    Serial.begin(115200);


    //load card keys and names
    SPIFFS.begin();

    File file = SPIFFS.open("/keys.txt", FILE_READ);
    if (file && file.available()) {
        Serial.println("file exists");
        deserializeJson(jobact, file);
    }
    else {
        Serial.println("file doesnt exits");
    }
    file.close();
    ~file;


    ledcSetup(0, 1000, 12);// tone setup    


    ArduinoOTA.setHostname("smartlock");
    ArduinoOTA.begin();

    wifi_init();
    init_rtc();
    init_rfid();
    srv_init();
    stepper_init();

    
}


#define debug_tm 100
#define comb_tm 500
uint32_t tm0, tm1, tm2;
uint32_t tm_comb;
bool bul1;
bool stprn;

void loop()
{
    stprn = myStepper.isRunning();
    tm2 = max(tm2, (uint32_t)micros() - tm1);
    tm1 = micros();
    if(!stprn){
        wm.process();
        ArduinoOTA.handle();
    }
    
    myStepper.run();

    if(millis() - tm_comb > comb_tm && !stprn){// this causes 50ms delay if run continiusly
        tm_comb = millis();
        logic();
        check_combination();
        handle_batteries();
    }
    
    rotate_lock();

    if(millis() - tm0 > debug_tm){
        tm0 = millis();
        Serial.printf("lag: %6u bat1: %4u bat2: %4u bk_bat: %1u rotor_state: %s state: %u sw_val: %1u stepper: %5u isrunnung: %u \r", tm2, bat1_voltage, bat2_voltage, sel_bat, state_str[rotator], bsman, digitalRead(home_sw), stepstodeg(myStepper.currentPosition()), stprn);
        tm2 = 0;
        if(stprn && !bul1){
            myStepper.enableOutputs();
            bul1 = 1;
        }else if(!stprn && bul1){
            bul1 = 0;
            myStepper.disableOutputs();
        }
    }

}


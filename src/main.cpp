#include <Arduino.h>

#include <SPI.h>
//#include <MFRC522.h>
#include "EasyMFRC522.h"
#include <Tone32.h>
#include <WiFiManager.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "FS.h"
#include "SPIFFS.h"
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <RTClib.h>

#define tone_pin 12
#define tn_channel 0
#define ss1 14
#define ss2 13
#define rsta 17//22
#define rstb 16//21
#define bk_bat 27
#define mot_dir 32
#define mot_step 33
#define mot_en 25
#define bat_1 39
#define bat_2 36
#define home_shucle 34
#define status_led 2
#define home_sw 4

#define steps_rev 15000
#define steps_deg 30
#define degtosteps(a) (a*steps_deg)
#define stepstodeg(a) (a/steps_deg) 

bool resulta, resultb, checka, checkb;
bool shouldopen, isopen, dooropen;
bool store_card;
char card_name[50];

uint32_t bat1_voltage, bat2_voltage;

bool beep_valid, beep_invalid;
bool rtcok;

byte lock_state;
enum lockStates { error, locked, unlocked, lock, unlock, home, test };
String state_str[] = {"error", "locked", "unlocked", "lock", "unlock", "home", "test"};
lockStates rotator = home;
bool home_inv;
RTC_DS1307 rtc;

AsyncWebServer server(80);

EasyMFRC522 mfrc522A(ss1, rsta);
EasyMFRC522 mfrc522B(ss2, rstb);
WiFiManager wm;
DynamicJsonDocument jobact(1000);

// #include "FastAccelStepper.h"
// FastAccelStepperEngine engine = FastAccelStepperEngine();
// FastAccelStepper *stepper = NULL;
#include "AccelStepper.h"
AccelStepper myStepper(AccelStepper::DRIVER, mot_step, mot_dir);

byte tag[4];
char current_tag[42];
int result;

//do tone
void tone_valid(){
    tone(tone_pin, NOTE_D6, 500, tn_channel);
    noTone(tone_pin, tn_channel);
}
void tone_reject(){
    tone(tone_pin, NOTE_E3, 500, tn_channel);
    noTone(tone_pin, tn_channel);
}
void logic() {
    if (resulta) {
        if (mfrc522A.detectTag(tag)) {

            sprintf(current_tag, "%X%X%X%X", tag[0], tag[1], tag[2], tag[3]);
            checka = 1;
        }
    }
    if (resultb) {
        if (mfrc522B.detectTag(tag)) {
            sprintf(current_tag, "%X%X%X%X", tag[0], tag[1], tag[2], tag[3]);
            checkb = 1;
        }
    }

    if (beep_valid) {
        beep_valid = 0;
        tone_valid();
    }
    if (beep_invalid) {
        beep_invalid = 0;
        tone_reject();
    }

}
void check_combination() {
    if (checka || checkb) {
        

        Serial.println(current_tag);
        if (store_card) {
            store_card = 0;
            char datetime[50];
            DateTime now;
            if (rtcok) {
                now = DateTime(rtc.now());
            }
            else {
                now = DateTime(millis() / 1000);
            }
            sprintf(datetime, "%d:%d:%d %d/%d/%d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
            JsonObject newcard = jobact.createNestedObject(String(current_tag));
            newcard["name"] = String(card_name);
            newcard["registered_date"] = String(datetime) ;

            result = jobact.containsKey(current_tag);

            if (result) {
                File file = SPIFFS.open("/keys.txt", FILE_WRITE, true);
                serializeJson(jobact, file);
                file.close();
                ~file;
                beep_valid = 1;
                Serial.println("saved");
                result = 0;
                return;
            }else
            {
                beep_invalid = 1;
                Serial.println("error saving");
                return;
            }
            ~newcard;
        }

        result = jobact.containsKey(current_tag);
        if (result) {
            char datetime[50];
            DateTime now;
            if (rtcok) {
                now = DateTime(rtc.now());
            }
            else {
                now = DateTime(millis() / 1000);
            }
            sprintf(datetime, "%d:%d:%d %d/%d/%d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
            JsonObject card = jobact[String(current_tag)];
            String tmp;
            tmp += isopen ? "LOCKED FROM " : "UNLOCKED FROM ";
            tmp += checka ? "OUTSIDE" : checkb ? "INSIDE" : "";
            card[String(datetime)] = tmp;
            String nsame = card["name"];
            Serial.println(nsame + " grandet");
            File file = SPIFFS.open("/keys.txt", FILE_WRITE, true);
            serializeJson(jobact, file);
            file.close();
            ~file;
            beep_valid = 1;
            result = 0; 
            if(rotator == locked){
                rotator = unlock;
            }else if (rotator == unlocked){
                rotator = lock;
            }
            ~card;
        }
        else {
            beep_invalid = 1;
            Serial.println("rejected");
        }

        checka ? Serial.println("a fired") : checka = 0;
        checkb ? Serial.println("b fired") : checkb = 0;
        checka = 0;
        checkb = 0;
    }
}


uint8_t state_home, state_test;
bool bsman, seccondhit;
void rotate_lock() {
  switch(rotator){
    case home:
        switch(state_home){
            case 0:
                if (digitalRead(home_sw)){
                    //stepper->move(500);
                    myStepper.move(500);
                }
                state_home = 1;
                break;
            case 1:
                if (digitalRead(home_sw) == 0 && !myStepper.isRunning()){
                    //stepper->move(15000);
                    myStepper.move(15000);
                    state_home = 2;
                }
                break;
            case 2:
                if(digitalRead(home_sw) == 1 && myStepper.isRunning() && !seccondhit){
                    myStepper.stop();
                    myStepper.setCurrentPosition(degtosteps(2*360) + 100);
                    myStepper.move(25000);
                    seccondhit = 1;
                    
                }
                if(seccondhit && myStepper.distanceToGo() < 20000){
                    state_home = 3;
                }
                if(!myStepper.isRunning()){
                    state_home = 3;
                }
                break;
            case 3:
                if(digitalRead(home_sw) == 1 && myStepper.isRunning()){
                    //stepper->forceStop();
                    myStepper.stop();
                    if(myStepper.currentPosition() > 20000){
                        rotator = locked;
                        if(home_inv){
                            myStepper.setCurrentPosition(degtosteps(2*360) + 100);
                        }else{
                            myStepper.setCurrentPosition(degtosteps(2*360) - 100);
                        }
                    }else{
                        rotator = locked;
                        if(home_inv){
                            myStepper.setCurrentPosition(degtosteps(2*360) + 100);
                        }else{
                            myStepper.setCurrentPosition(degtosteps(2*360) - 100);
                        }
                    }state_home = 3;
                    state_home = 3;

                }else if(digitalRead(home_sw) == 0 && !myStepper.isRunning()){
                    myStepper.move(-15000);
                    home_inv = 1;
                }
                break;
            default:
                break;
        }
        
      break;
      
    case lock:
        // if(rotator != locked){
        //     //stepper->moveTo(degtosteps(2*360));
        //     myStepper.moveTo(degtosteps(2*360));
        //     rotator = locked;
        // }
        if(rotator != locked){
            if(!myStepper.isRunning() && !bsman){
                myStepper.moveTo(degtosteps(2*360));
                bsman = 1;
            }else
            if(digitalRead(home_sw) && myStepper.distanceToGo()< degtosteps(270) && bsman){
                myStepper.stop();
                myStepper.setCurrentPosition(degtosteps(2*360));
                rotator = locked;
                bsman = 0;
            }else
            if(!digitalRead(home_sw) && !myStepper.isRunning() && bsman){
                myStepper.move(5000);
            }
        }
    break;

    case unlock:
        // if(rotator != unlocked){
        //     //stepper->move(degtosteps(1*360));
        //     myStepper.moveTo(degtosteps(1*360));
        //     rotator = unlocked;
        // }
        if(rotator != unlocked){
            if(!myStepper.isRunning() && !bsman){
                myStepper.moveTo(degtosteps(1*360));
                bsman = 1;
            }else
            if(digitalRead(home_sw) && ~myStepper.distanceToGo() < degtosteps(270) && bsman){
                myStepper.stop();
                myStepper.setCurrentPosition(degtosteps(1*360));
                rotator = unlocked;
                bsman = 0;
            }else
            if(!digitalRead(home_sw) && !myStepper.isRunning() && bsman){
                myStepper.move(-5000);
            }
        }
      break;

    default:
        break;
        
    case test:
        switch (state_test){
            case 0:
                if(digitalRead(home_sw) && !myStepper.isRunning()){
                    myStepper.move(500);
                }
                if(!myStepper.isRunning()){
                    state_test++;
                }
                break;
            case 1:
                if(!digitalRead(home_sw) && !myStepper.isRunning()){
                    myStepper.move(-500);
                }else
                if(digitalRead(home_sw) && myStepper.isRunning()){
                    myStepper.stop();
                    myStepper.setCurrentPosition(0);
                    state_test++;
                }
                break;
            case 2:
                if(digitalRead(home_sw) && !myStepper.isRunning()){
                    myStepper.move(10000);
                }else
                if(digitalRead(home_sw) && myStepper.isRunning() && myStepper.currentPosition() > 500){
                    myStepper.stop();
                    Serial.print('\n' + myStepper.currentPosition() + '\n');
                    //rotator = lock;
                    state_test++;
                }
                break;
            default:
                break;
        }
        break;
  }
}

#define bt1_r1 1200
#define bt1_r2 470
#define bt2_r1 1200
#define bt2_r2 470
#define bt1_thresshold 6000
#define bt2_thresshold 6000
#define bt1_ratio (bt1_r1+bt1_r2)/bt1_r2
#define bt2_ratio (bt2_r1+bt2_r2)/bt2_r2

byte sel_bat;
uint32_t vadc1, vadc2;
void handle_batteries(){
    vadc1 = analogReadMilliVolts(bat_1);
    vadc2 = analogReadMilliVolts(bat_2);
    // bat1_voltage = (vadc1 * bt1_r2) / (bt1_r1 + bt1_r2);
    // bat2_voltage = (vadc2 * bt2_r2) / (bt2_r1 + bt2_r2);
    bat1_voltage = vadc1 * bt1_ratio;
    bat2_voltage = vadc2 * bt2_ratio;

    if(bat1_voltage < bt1_thresshold && sel_bat == 0 && bat2_voltage > bt1_thresshold){
        sel_bat = 1;
    }else if (bat1_voltage > bt1_thresshold + 1000 && sel_bat == 1){
        sel_bat = 0;
    }

    digitalWrite(bk_bat, sel_bat? 1:0);

}
// TaskHandle_t Task1;
// void Task1code(void * parameters){
//     //myStepper.run();
//     handle_batteries();
// }
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
    WiFi.mode(WIFI_STA);

    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(60);
    wm.setHostname("smartlock");
    wm.autoConnect("AutoConnectAP", "password");

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

    if (!rtc.begin()) {
        Serial.println("RTC ERROR");
    }
    else {
        Serial.println("RTC OK");
        rtcok = 1;
    }

    ledcSetup(0, 1000, 12);// tone setup    

    SPI.begin();
    mfrc522A.init();   // Init MFRC522 module
    mfrc522B.init();   // Init MFRC522 module
    resulta = mfrc522A.getMFRC522()->PCD_PerformSelfTest();
    resultb = mfrc522B.getMFRC522()->PCD_PerformSelfTest();

    if (!resulta) {
        Serial.println("RFIDA FAILLED!");
        beep_invalid = 1;
    }
    else {
        Serial.println("RFIDA OK");
        beep_valid = 1;
    }
    if (!resultb) {
        Serial.println("RFIDB FAILLED!");
        beep_invalid = 1;
    }
    else {
        Serial.println("RFIDB OK");
        beep_valid = 1;
    }

    ArduinoOTA.setHostname("smartlock");
    ArduinoOTA.begin();

    myStepper.setMaxSpeed(13000);
    myStepper.setAcceleration(50000);
    myStepper.setSpeed(15000);
    //myStepper.setMinPulseWidth(20);
    myStepper.setEnablePin(mot_en);
    myStepper.setPinsInverted(0,0,1);
    //myStepper.enableOutputs();

    // engine.init();
    // stepper = engine.stepperConnectToPin(mot_step);
    // if (stepper) {
    //     stepper->setDirectionPin(mot_dir);
    //     stepper->setEnablePin(mot_en);
    //     stepper->setAutoEnable(true);

    //     // If auto enable/disable need delays, just add (one or both):
    //      stepper->setDelayToEnable(50);
    //      stepper->setDelayToDisable(50);

    //     stepper->setSpeedInUs(500);  // the parameter is us/step !!!
    //     stepper->setAcceleration(4000);
    // }

    // xTaskCreatePinnedToCore(
    //   Task1code, /* Function to implement the task */
    //   "Task1", /* Name of the task */
    //   10000,  /* Stack size in words */
    //   NULL,  /* Task input parameter */
    //   0,  /* Priority of the task */
    //   &Task1,  /* Task handle. */
    //   0); /* Core where the task should run */
    // rotator = home;

    server.on("/loggen", HTTP_GET, [](AsyncWebServerRequest* request) {
        String const_page;
        int num_of_cards = jobact.nesting();
        const_page += num_of_cards;
        request->send(200, "text/plain", const_page);
    });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        int paramsNr = request->params();
        if(paramsNr >0){
          Serial.println(paramsNr);
          for (int i = 0; i < paramsNr; i++) {
              AsyncWebParameter* p = request->getParam(i);
              Serial.print(p->name());
              Serial.print("  ");
              Serial.println(p->value());
              if (p->name() == String("ID")) {
                  p->value().toCharArray(card_name, sizeof(p->value()));
                  //strcpy(card_name, p->name());
                  store_card = 1;
                  Serial.println("STORE CARD");
              }
              else if (p->name() == String("FUNCTION")) {
                  if (p->value() == String("LOCK")) {
                      rotator = lock;
                  }
                  else if (p->value() == String("UNLOCK")) {
                      rotator = unlock;
                  }
              }
              else if (p->name() == String("TIME")) {
                  char* pEnd;
                  String str = p->value();
                  time_t tms = strtoul(str.c_str(), NULL, 10);
                  DateTime time = DateTime(tms);
                  rtc.adjust(time);
                  time = rtc.now();
                  Serial.println(p->value().c_str());
                  Serial.println(tms);
                  Serial.print("time set to ");
                  Serial.print(time.year(), DEC);
                  Serial.print('/');
                  Serial.print(time.month(), DEC);
                  Serial.print('/');
                  Serial.print(time.day(), DEC);
                  Serial.print(" ");
                  Serial.print(time.hour(), DEC);
                  Serial.print(':');
                  Serial.print(time.minute(), DEC);
                  Serial.print(':');
                  Serial.print(time.second(), DEC);
                  Serial.println();
              }
          }
          
        }
        request->send(SPIFFS, "/index.html", String(), false);
      });

       server.begin();

    
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


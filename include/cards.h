#include <Arduino.h>
#include <SPI.h>
#include "EasyMFRC522.h"

#define ss1 14
#define ss2 13
#define rsta 17//22
#define rstb 16//21

bool resulta, resultb, checka, checkb;
bool shouldopen, isopen, dooropen;
bool store_card;
char card_name[50];

byte tag[4];
char current_tag[42];
int result;

EasyMFRC522 mfrc522A(ss1, rsta);
EasyMFRC522 mfrc522B(ss2, rstb);

extern void tone_valid();
extern void tone_reject();
//extern DynamicJsonDocument jobact(1000);

void init_rfid(){
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

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
        
        char datetime[50];
        DateTime now;
        if (rtcok) {
            now = DateTime(rtc.now());
        }
        else {
            now = DateTime(millis() / 1000);
        }
        sprintf(datetime, "%02d:%02d:%02d %d/%d/%d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
        
        
        Serial.println(current_tag);
        if (store_card) {
            store_card = 0;

            result = jobact.containsKey(current_tag);
            if(result){
                beep_invalid = 1;
                result = 0;
                return;
            }

            JsonObject newcard = jobact.createNestedObject(String(current_tag));
            newcard["name"] = String(card_name);
            newcard["registered_date"] = String(datetime);

            result = jobact.containsKey(current_tag);

            if (result) {
            char tmpss[200];
            sprintf(tmpss, "%s,%s,%s,%s\n", current_tag, card_name, datetime, "Card Added");
            File file_csv = SPIFFS.open("/logs.csv", FILE_APPEND);
            if (file_csv) {
                file_csv.print(tmpss);
            }else{
                Serial.println("file failled");
            }
            file_csv.close();
            ~file_csv;
            }

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
            JsonObject card = jobact[String(current_tag)];
            // card[String(datetime)] = tmp;
            String nsame = card["name"];
            Serial.println(nsame + " grandet");
            // File file = SPIFFS.open("/keys.txt", FILE_WRITE, true);
            // serializeJson(jobact, file);
            // file.close();
            // ~file;
            beep_valid = 1;
            result = 0; 

            // int sss = cp.getRowsCount() + 1;
            // cardid[sss] = current_tag;
            // cardname[sss] = card_name;
            // carddate[sss] = datetime;
            // String tmp;
            // tmp += rotator == unlocked ? "LOCKED FROM " : "UNLOCKED FROM ";
            // tmp += checka ? "OUTSIDE" : checkb ? "INSIDE" : "";
            char tmpss[200];
            sprintf(tmpss, "%s,%s,%s,%s \n", current_tag, nsame, datetime, strs[(checka | checkb << 1 | (rotator == unlocked) << 2)]);
            File file_csv = SPIFFS.open("/logs.csv", FILE_APPEND);
            if (file_csv) {
                file_csv.print(tmpss);
            }else{
                Serial.println("file failled");
            }
            file_csv.close();
            ~file_csv;

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
            char tmpss[200];
            sprintf(tmpss, "%s,%s,%s,%s\n", current_tag, "unknown", datetime, "Unknown Card");
            File file_csv = SPIFFS.open("/logs.csv", FILE_APPEND);
            if (file_csv) {
                file_csv.print(tmpss);
            }else{
                Serial.println("file failled");
            }
            file_csv.close();
            ~file_csv;
        }

        checka ? Serial.println("a fired") : checka = 0;
        checkb ? Serial.println("b fired") : checkb = 0;
        checka = 0;
        checkb = 0;
    }
}
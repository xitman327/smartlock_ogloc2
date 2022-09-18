
AsyncWebServer server(80);

extern char card_name[50];
extern bool store_card;

void srv_init(){
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
                      beep_valid = 1;
                  }
                  else if (p->value() == String("UNLOCK")) {
                      rotator = unlock;
                      beep_valid = 1;
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
              else if (p->name() == String("clear_log")) {
                if(SPIFFS.exists("/logs.csv")){
                    SPIFFS.remove("/logs.csv");
                    File file_csv = SPIFFS.open("/logs.csv", FILE_WRITE, true);
                    if (file_csv) {
                        file_csv.print("Card ID,Card Name,Date,Action\n");
                    }else{
                        Serial.println("file failled");
                    }
                    file_csv.close();
                    ~file_csv;
                    beep_valid = 1;
                }
              }
              else if (p->name() == String("remove_cards")) {
                if(SPIFFS.exists("/keys.txt")){
                    SPIFFS.remove("/keys.txt");
                    jobact.clear();
                    File file = SPIFFS.open("/keys.txt", FILE_WRITE, true);
                    serializeJson(jobact, file);
                    file.close();
                    ~file;
                    beep_valid = 1;
                }

              }
          }
          
        }
        request->send(SPIFFS, "/index.html", String(), false);
      });

      server.on("/logs.csv", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/logs.csv", String(), false);
      });
      server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/favicon.ico", "image/x-icon");
      });


       server.begin();
}
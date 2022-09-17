#include "ESPAsyncWebServer.h"
//#include "stepper.h"
#include <RTClib.h>
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
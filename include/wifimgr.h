#include <WiFiManager.h>
#include "WiFi.h"
WiFiManager wm;

void wifi_init(){
    
    WiFi.mode(WIFI_STA);

    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(60);
    wm.setHostname("smartlock");
    wm.autoConnect("AutoConnectAP", "password");
}
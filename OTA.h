#ifndef OTA_H
#define OTA_H

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <nvs_flash.h>

extern uint8_t mac[6];
class OTA {
public:
  OTA()
    : otaTaskHandle(nullptr), otaActive(false) {}

  void begin(const char* nameprefix, const char* ssid, const char* password) {
    if (otaActive) return;  // zaten aktifse çık

    uint16_t maxlen = strlen(nameprefix) + 7;
    char* fullhostname = new char[maxlen];
    snprintf(fullhostname, maxlen, "%s-%02x%02x%02x", nameprefix, mac[3], mac[4], mac[5]);
    ArduinoOTA.setHostname(fullhostname);

    // WiFi AP başlat
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);

    ArduinoOTA.setPassword("openeye");

    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else
        type = "filesystem";

      //Serial.println("Start updating " + type);
    });

    ArduinoOTA.onEnd([]() {
      //Serial.println("\nEnd");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
      // Serial.printf("Error[%u]: ", error);
      // if (error == OTA_AUTH_ERROR) Serial.println("\nAuth Failed");
      // else if (error == OTA_BEGIN_ERROR) Serial.println("\nBegin Failed");
      // else if (error == OTA_CONNECT_ERROR) Serial.println("\nConnect Failed");
      // else if (error == OTA_RECEIVE_ERROR) Serial.println("\nReceive Failed");
      // else if (error == OTA_END_ERROR) Serial.println("\nEnd Failed");
    });

    ArduinoOTA.begin();
    otaActive = true;

    xTaskCreatePinnedToCore(otaHandleTask, "OTA_HANDLE", 10000, this, 1, &otaTaskHandle, 0);
    //Serial.println("OTA Initialized");
    //Serial.print("AP IP: ");
    //Serial.println(WiFi.softAPIP());
  }

  void stop() {
    if (!otaActive) return;
    if (otaTaskHandle) {
      vTaskDelete(otaTaskHandle);
      otaTaskHandle = nullptr;
    }
    ArduinoOTA.end();             // OTA durdur
    WiFi.softAPdisconnect(true);  // AP kapat
    WiFi.mode(WIFI_OFF);          // WiFi tamamen kapat

    otaActive = false;
    //Serial.println("OTA Stopped");
  }

  bool isActive() {
    return otaActive;
  }

private:
  TaskHandle_t otaTaskHandle;
  bool otaActive;

  static void otaHandleTask(void* parameter) {
    //OTA* instance = (OTA*)parameter;
    for (;;) {
      ArduinoOTA.handle();
      vTaskDelay(pdMS_TO_TICKS(1));
    }
  }
};

extern OTA ota;

#endif

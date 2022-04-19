#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

ESP8266WiFiMulti WiFiMulti;

#include <ArduinoJson.h>

#include "secrets.h"

#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>

#define LED_PIN 5
#define NUM_LEDS 28
CRGB leds[NUM_LEDS];

#define SYSTEM_SIZE 8000  // Peak wattage of the solar system.

void setup() {
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);

  Serial.begin(115200);

  leds[NUM_LEDS-1].setRGB( 255, 0, 0);
  FastLED.show();

  WiFi.mode(WIFI_OFF);  //Prevents reconnection issue (taking too long to connect)
  delay(1000);
  WiFi.mode(WIFI_STA);  //Only Station No AP, This line hides the viewing of ESP as wifi hotspot

  WiFi.begin(SECRET_SSID, SECRET_WIFI_PASSWORD);  //Connect to your WiFi router
  Serial.println("\n");
  Serial.print("Connecting");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
}

void loop() {

    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, LOCAL_SERVER)) {  // HTTP


      Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          
          StaticJsonDocument<256> res;
          deserializeJson(res, payload);
          JsonObject obj = res.as<JsonObject>();

          // {"ratedPower":9.1,"gridPower":249.0,"relay2Power":0.0,"feedInPower":5.0,"relay1Power":0.0,"batPower1":0.0}

          if (!obj.containsKey("gridPower")) {
            Serial.println("Couldn't get JSON data.");
            leds[NUM_LEDS - 1] = CRGB(255, 255, 255);
            FastLED.show();
            http.end();
            return;
          }

          Light(obj["feedInPower"].as<signed int>(), obj["gridPower"].as<signed int>());

        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());

        leds[NUM_LEDS-1].setRGB( 0, 255, 0);
        FastLED.show();
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");

      leds[NUM_LEDS-1].setRGB( 0, 0, 255);
      FastLED.show();
    }
  
  delay(30000);
}

void Light(int FeedIn, int Grid) {

  int feedInMapped = map(FeedIn, -SYSTEM_SIZE, SYSTEM_SIZE, -NUM_LEDS, NUM_LEDS);
  int gridMapped = map(Grid, 0, SYSTEM_SIZE, 0, NUM_LEDS);

  Serial.print("Grid Mapped: ");
  Serial.println(gridMapped);

  Serial.print("FeedIn Mapped: ");
  Serial.println(feedInMapped);

  // {"ratedPower":9.1,"gridPower":249.0,"relay2Power":0.0,"feedInPower":5.0,"relay1Power":0.0,"batPower1":0.0}
  // Solar usage is gridPower - feedInPower when feedInPower > 0.
  int solarHomeUsage = FeedIn > 0 ? gridMapped - feedInMapped : gridMapped;

  for (int dot = 0; dot < NUM_LEDS; dot++) {

    if (dot < solarHomeUsage) {
      leds[dot].setRGB( 0, 200, 255);
      FastLED.delay(100);
      FastLED.show();
      continue;
    }

    else if (FeedIn < 0 && dot < -feedInMapped + solarHomeUsage) {
      leds[dot].setRGB( 255, 20, 0);
      FastLED.delay(100);
      FastLED.show();
    }

    else if (FeedIn > 0 && dot < feedInMapped + solarHomeUsage) {
      leds[dot].setRGB( 255, 70, 0);
      FastLED.delay(100);
      FastLED.show();
    }

    else {
      leds[dot].setRGB( 0, 0, 0);
      FastLED.delay(25);
      FastLED.show();
    }

  }
}

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>

#include "secrets.h"

#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>

#define LED_PIN 16
#define NUM_LEDS 39
CRGB leds[NUM_LEDS];

#define SYSTEM_SIZE 8000  // Peak wattage of the solar system.

const char *ssid = SECRET_SSID;  // WIFI Stuff
const char *password = SECRET_WIFI_PASSWORD;

//Your Domain name with URL path or IP address with path
String serverName = LOCAL_SERVER;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

void setup() {
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);

  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop() {
  // Send an HTTP POST request depending on timerDelay
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;

      String serverPath = serverName;

      // Your Domain name with URL path or IP address with path
      http.begin(client, serverPath.c_str());

      // Send HTTP GET request
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();

        StaticJsonDocument<256> res;
        deserializeJson(res, payload);
        JsonObject obj = res.as<JsonObject>();

        // {"ratedPower":9.1,"gridPower":249.0,"relay2Power":0.0,"feedInPower":5.0,"relay1Power":0.0,"batPower1":0.0}

        if (!obj.containsKey("gridPower")) {
          Serial.println("Couldn't get JSON data.");
          leds[NUM_LEDS - 1] = CRGB(255, 255, 255);
          FastLED.show();
          return;
        }

        String gridPower = obj["gridPower"];
        String feedInPower = obj["feedInPower"];

        Light(feedInPower.toInt(), gridPower.toInt());

        delay(5000);

      } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);

        leds[NUM_LEDS - 1] = CRGB(255, 255, 255);
        FastLED.show();
      }

      // Free resources
      http.end();

    } else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

void Light(int FeedIn, int Grid) {

  int feedInMapped = map(FeedIn, -SYSTEM_SIZE, SYSTEM_SIZE, -NUM_LEDS, NUM_LEDS);
  int gridMapped = map(Grid, 0, SYSTEM_SIZE, 0, NUM_LEDS);

  Serial.print("Grid Mapped: ");
  Serial.println(gridMapped);

  Serial.print("FeedIn Mapped: ");
  Serial.println(feedInMapped);

  Serial.println("__________");

  // {"ratedPower":9.1,"gridPower":249.0,"relay2Power":0.0,"feedInPower":5.0,"relay1Power":0.0,"batPower1":0.0}
  // Solar usage is gridPower - feedInPower when feedInPower > 0.
  int solarHomeUsage = FeedIn > 0 ? gridMapped - feedInMapped : gridMapped;

  FastLED.clear();

  for (int i = 0; i < NUM_LEDS; i++) {

    if (i < solarHomeUsage) {
      leds[i] = CRGB(0, 255, 200);
      Serial.print("Blue ");
      delay(100);
      FastLED.show();
      continue;
    }

    else if (FeedIn < 0 && i < -feedInMapped + solarHomeUsage) {
      leds[i] = CRGB(255, 20, 0);
      Serial.print("Orange ");
      delay(100);
      FastLED.show();  // These seperate .show()s are because of some sort of bug with ESP8266.
    }

    else if (FeedIn > 0 && i < feedInMapped + solarHomeUsage) {
      leds[i] = CRGB(255, 70, 0);
      Serial.print("Yellow ");
      delay(100);
      FastLED.show();
    }
  }
  Serial.println("");
}

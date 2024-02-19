#include <FastLED.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <TimeLib.h>

#define BAUD 115200
#define DATA_PIN 5
#define FIRST_ROW 0
#define LAST_ROW 8
#define FIRST_LED_IN_A_ROW 0
#define LAST_LED_IN_A_ROW 8
#define TOTAL_OF_LEDS LAST_ROW * LAST_LED_IN_A_ROW
#define ONE_SECOND 1000
#define FIVE_SECONDS 5000
#define TEN_SECONDS 10000
#define ONE_MINUTE 60000
#define LED_BRIGHTNESS 255
#define GREEN "L"
#define YELLOW "A"
#define RED "B"
#define CLASS_1 "a"
#define CLASS_2 "b"
#define UNDEFINED "UNDEFINED"
#define WIFI_CONNECTION_ATTEMPTS 5
#define HTTP_STATUS_OK 200

#define WIFI_SSID "DESKTOP-SUPREMO"
#define WIFI_PASSWORD "12345678"

CRGB leds[TOTAL_OF_LEDS];
const char* NTP_SERVER = "pool.ntp.org";
int currentRow = FIRST_ROW;
int currentWeekday = 0;
int httpCode = 0;
String status = UNDEFINED;
String classOfTheDay = UNDEFINED;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER);

void setup() {
    Serial.begin(BAUD);
    setupLeds();
    connectWifi();
    getCurrentDateTime();

    classOfTheDay = setClassOfTheDay();
}

void loop() {
    if (classOfTheDay == UNDEFINED) {
        logClassOfTheDayUndefined();
    } else {
        getLightStatus();
        setLightStatus();
    }

    if (WiFi.status() != WL_CONNECTED) {
        connectWifi();
    }
}

void connectWifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED) {
        logWifiAttemptError();
        attempts++;

        if (attempts >= WIFI_CONNECTION_ATTEMPTS) {
            logWiFiConnectionError();
            attempts = 0;
        }
    }

    logWiFiSuccess();
}

void getCurrentDateTime() {
    timeClient.begin();

    while (!timeClient.update()) {
        logNTPSyncError();
        timeClient.forceUpdate();
        timeClient.forceUpdate();
    }

    logNTPSyncSuccess();
}

void setupLeds() {
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, TOTAL_OF_LEDS);
    FastLED.setBrightness(LED_BRIGHTNESS);
}

void setLightStatus() {
    logSetLightStatus();

    CRGB color;
    if (status == YELLOW) {
        color = CRGB::Yellow;
    } else if (status == RED) {
        color = CRGB::Red;
    } else if (status == GREEN) {
        color = CRGB::Green;
    }

    if (currentRow < LAST_ROW) {
        turnON(currentRow, color);
        turnOFF(currentRow, color);
        currentRow++;
    } else {
        currentRow = FIRST_ROW;
    }
}

void turnON(int row, CRGB color) {
    for (int currentLed = FIRST_LED_IN_A_ROW; currentLed < LAST_LED_IN_A_ROW; currentLed++) {
        leds[row * LAST_LED_IN_A_ROW + currentLed] = color;
    }
    FastLED.show();
    delay(ONE_MINUTE);
}

void turnOFF(int row, CRGB color){
    for (int currentLed = FIRST_LED_IN_A_ROW; currentLed < LAST_LED_IN_A_ROW; currentLed++) {
        leds[currentRow * LAST_LED_IN_A_ROW + currentLed] = CRGB::Black;
    }
}

String setClassOfTheDay() {
    time_t today = timeClient.getEpochTime();
    currentWeekday = weekday(today);

    if (currentWeekday == 3 || currentWeekday == 5) {
      return CLASS_1;
    } else if (currentWeekday == 4 || currentWeekday == 6) {
        return CLASS_2;
    } else {
      return UNDEFINED;
    }

    logDayOfWeek();
    logSetClassOfTheDay();
}

void getLightStatus() {
    HTTPClient http;
    String url = "https://niloweb.com.br/transit-room/api/point_consulta_reg_" + classOfTheDay + ".php";

    http.begin(url);
    http.setTimeout(FIVE_SECONDS);

    int httpCode = http.GET();

    if (httpCode == HTTP_STATUS_OK) {
        String payload = http.getString();
        StaticJsonDocument<HTTP_STATUS_OK> doc;
        DeserializationError error = deserializeJson(doc, payload);

        const char* res = doc[0]["res"];
        status = String(res);

    } else {
        logErroToGetStatus();
    }

    http.end();
}

void showAlert(CRGB color, int numberOfBlinks) {
    for (int blink = 0; blink < numberOfBlinks; blink++) {
        fill_solid(leds, TOTAL_OF_LEDS, color);
        FastLED.show();
        delay(ONE_SECOND);
        fill_solid(leds, TOTAL_OF_LEDS, CRGB::Black);
        FastLED.show();
        delay(ONE_SECOND);
    }
}

void logWiFiSuccess() {
    Serial.println();
    Serial.print("WiFi connected in ");
    Serial.print(WIFI_SSID);
    Serial.print(" with IP ");
    Serial.println(WiFi.localIP());
    showAlert(CRGB::Green, 3);
}

void logWifiAttemptError() {
    Serial.print("Failed to connect to WiFi ");
    Serial.print(WIFI_SSID);
    Serial.print(" with status ");
    Serial.println(WiFi.status());
    showAlert(CRGB::Yellow, 1);
}

void logNTPSyncSuccess() {
    time_t utcTime = timeClient.getEpochTime();
    Serial.print("Time updated for ");
    Serial.print(timeClient.getFormattedTime());
    Serial.print(" ");
    Serial.print(day(utcTime));
    Serial.print("/");
    Serial.print(month(utcTime));
    Serial.print("/");
    Serial.print(year(utcTime));
    Serial.println();
    showAlert(CRGB::Green, 3);
}

void logClassOfTheDayUndefined(){
    Serial.println("Class of the day UNDEFINED");
    showAlert(CRGB::White, 60);
}

void logWiFiConnectionError() {
    showAlert(CRGB::Red, 1);
    delay(TEN_SECONDS);
}

void logNTPSyncError() {
    Serial.print("Try setting time in ");
    Serial.println(NTP_SERVER);
    showAlert(CRGB::Yellow, 3);
    delay(FIVE_SECONDS);
}

void logSetLightStatus(){
    Serial.print("Set light status: ");
    Serial.println(status);
}

void logDayOfWeek(){
    Serial.print("The day of the week is ");
    Serial.println(currentWeekday);
    showAlert(CRGB::White, currentWeekday);
}

void logClassOfTheDay(){
    Serial.print("The class of the day is ");
    Serial.print(classOfTheDay);
}

void logErroToGetStatus(){
  Serial.print("Failed to connect with status ");
  Serial.println(httpCode);
  showAlert(CRGB::Red, 5);
}

void logSetClassOfTheDay(){
  Serial.print("The class of day is ");
  Serial.println(classOfTheDay);

  if (classOfTheDay = CLASS_1) {
    showAlert(CRGB::Cyan, 1);
  } else {
    showAlert(CRGB::Cyan, 2);
  }
}
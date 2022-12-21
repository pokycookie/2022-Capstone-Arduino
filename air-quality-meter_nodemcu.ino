#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

#define wifiPin 5 // D1
#define httpPin 4 // D2

void httpPOST(float pm10, float pm25, float pm100, float form, float temp, float humi) {
  digitalWrite(httpPin, HIGH);
  
  StaticJsonDocument<JSON_OBJECT_SIZE(6)> doc;
  doc["pm10"] = pm10;
  doc["pm25"] = pm25;
  doc["pm100"] = pm100;
  doc["form"] = form;
  doc["temp"] = temp;
  doc["humi"] = humi;

  String body = "";
  serializeJson(doc, body);
  Serial.println(body);
 
  HTTPClient http;
  http.begin("http://pknu-air-pokycookie.koyeb.app/api/data");
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(body);
  Serial.println(http.getString());

  int tr = 5;
  while(tr > 0) {
    if (httpResponseCode > 0) break;
    else Serial.println("Error HTTP");
    delay(100);
    tr--;
  }
  http.end();

  digitalWrite(httpPin, LOW);
}

void resetWifi(const char* ssid, const char* password) {
  int wifiTry = 0;
  digitalWrite(wifiPin, LOW);
  
  // Turn OFF
  if (WiFi.status() == WL_CONNECTED) WiFi.mode(WIFI_OFF);
  // Turn ON
  Serial.print("WiFi connecting to ");
  Serial.print(ssid);
  if (WiFi.status() != WL_CONNECTED) WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifiTry += 1;
    if (wifiTry > 20) {
      Serial.println("WiFi Error");
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED){
    Serial.println("WiFi Connected");
    digitalWrite(wifiPin, HIGH);
  }
}

int split(const char* str, char result[10][10]) {
  int resultI = 0;
  int resultJ = 0;
  for (int i = 0; i < strlen(str); i++) {
    const char target = str[i];
    if (target == ' ') {
      result[resultI][resultJ] = '\0';
      resultI++;
      resultJ = 0;
      continue;
    }
    result[resultI][resultJ] = target;
    resultJ++;
  }
  result[resultI][resultJ] = '\0';
  resultI++;
  return resultI;
}

void getSerial() {
  char temp[100];
  if (Serial.available()) {
    int readSize = Serial.readBytesUntil('\n', temp, 99);
    temp[readSize] = '\0';

    char result[10][10];
    split(String(temp).c_str(), result);

    if (String(result[0]).equals(String("UNO"))) {
      float pm10 = atof(result[1]);
      float pm25 = atof(result[2]);
      float pm100 = atof(result[3]);
      float form = atof(result[4]);
      float temp = atof(result[5]);
      float humi = atof(result[6]);
      if (WiFi.status() == WL_CONNECTED){
        httpPOST(pm10, pm25, pm100, form, temp, humi);
      }
    } else if(String(result[0]).equals(String("SSID"))) {
      String ssid = String(result[1]);
      Serial.println(ssid);
      for (int i = 0; i < ssid.length(); i++) {
      EEPROM.write(i, (char)ssid.charAt(i));
      }
      EEPROM.write(ssid.length(), ';');
      EEPROM.commit();
    } else if(String(result[0]).equals(String("PW"))) {
      String password = String(result[1]);
      Serial.println(password);
      for (int j = 0; j < password.length(); j++) {
      EEPROM.write(50 + j, (char)password.charAt(j));
      }
      EEPROM.write(50 + password.length(), ';');
      EEPROM.commit();
    } else if(String(result[0]).equals(String("RST"))) {
      String tempSSID;
      String tempPW;
    
      for(int i = 0; i < 50; i++) {
        if (EEPROM.read(i) == ';') break;
        tempSSID += (char)EEPROM.read(i);
      }
      tempSSID.trim();
      const char* ssid = tempSSID.c_str();
      
      for(int j = 0; j < 50; j++) {
        if (EEPROM.read(j + 50) == ';') break;
        tempPW += (char)EEPROM.read(j + 50);
      }
      tempPW.trim();
      const char* password = tempPW.c_str();
      Serial.println(ssid);
      Serial.println(password);
      resetWifi(ssid, password);
    }
  }
}

void setup() {
  // Serial
  Serial.begin(115200);

  // LED
  pinMode(wifiPin, OUTPUT);
  pinMode(httpPin, OUTPUT);
  
  // EEPROM
  EEPROM.begin(1000);
  String tempSSID;
  String tempPW;
  
  // SSID
  for(int i = 0; i < 50; i++) {
    if (EEPROM.read(i) == ';') break;
    tempSSID += (char)EEPROM.read(i);
  }
  tempSSID.trim();
  const char* ssid = tempSSID.c_str();
  Serial.println(ssid);

  // PW
  for(int j = 0; j < 50; j++) {
    if (EEPROM.read(j + 50) == ';') break;
    tempPW += (char)EEPROM.read(j + 50);
  }
  tempPW.trim();
  const char* password = tempPW.c_str();
  Serial.println(password);

  // WiFi
  resetWifi(ssid, password);
}

void loop() {
  getSerial();
}

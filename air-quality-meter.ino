#include <LiquidCrystal_I2C.h>
#include <MECHA_PMS5003ST.h>
#include <SoftwareSerial.h>

#define blePin 13
#define bzrPin 12

SoftwareSerial dust(4,5); // (TX, RX)
SoftwareSerial esp(8,9); // (TX, RX)
LiquidCrystal_I2C lcd(0x27, 20, 4);

MECHA_PMS5003ST pms(&dust);

float s_pm10 = 0;
float s_pm25 = 0;
float s_pm100 = 0;
float s_form = 0;
float s_temp = 0;
float s_humi = 0;

int postCount = 0;
int MODE = 0; // measuring:0, blutooth: 1

void clearAll() {
  clearLine(0);
  clearLine(1);
  clearLine(2);
  clearLine(3);
}

void clearLine(int line) {
  lcd.setCursor(0, line);
  lcd.print("                    ");
  lcd.setCursor(0, line);
}

void getBLE() {
  String readStr;
  while(Serial.available()) {
    delay(10);
    char temp = Serial.read();
    if (temp == ';') {
      break;
    }
    readStr += temp;
  }

  if (readStr.length() > 0) {
    clearAll();
    clearLine(0);
    lcd.print("[Bluetooth MODE]");
    
    // change mode
    if (readStr == "start") {
      MODE = 0;
      digitalWrite(blePin, LOW);
      clearAll();
    }
    // set ssid
    if (readStr.substring(0, 5) == "ssid:") {
      Serial.print("Set ssid: ");
      String ssid = readStr.substring(5, readStr.length());
      Serial.println(ssid);
      clearLine(1);
      lcd.print("WiFi SSID changed");
      clearLine(2);
      lcd.print(":");
      lcd.print(ssid);
      String str = String("SSID ") + String(ssid);
      esp.print(str);
    }
    // set pw
    if (readStr.substring(0, 3) == "pw:") {
      Serial.print("Set pw: ");
      String password = readStr.substring(3, readStr.length());
      Serial.println(password);
      clearLine(1);
      lcd.print("WiFi PW changed");
      clearLine(2);
      lcd.print(":");
      lcd.print(password);
      String str = String("PW ") + String(password);
      esp.print(str);
    }
    // reset Wifi
    if (readStr.substring(0, 9) == "resetwifi") {
      String str = String("RST ");
      esp.print(str);
    }
    readStr = "";
  }
}

void getPMS() {
  pms.request();
  if(!pms.read()){
    Serial.println("error!");
    return;
  }

  float pm10;
  float pm25;
  float pm100;
  float form;
  float temp;
  float humi;
  
  Serial.print("PM1.0: \t"); //PM1.0 측정
  pm10 = pms.getPmCf1(1.0);
  Serial.print(pm10);
  Serial.print("ug/m3");
  Serial.println();
 
  Serial.print("PM2.5: \t"); //PM2.5 측정
  pm25 = pms.getPmCf1(2.5);
  Serial.print(pm25);
  Serial.print("ug/m3");
  Serial.println();
 
  Serial.print("PM10: \t"); //PM10 측정
  pm100 = pms.getPmCf1(10);
  Serial.print(pm100);
  Serial.print("ug/m3");
  Serial.println();
  
  Serial.print("Form: \t"); //포름알데히드 측정
  form = pms.getForm();
  Serial.print(form);
  Serial.print("ug/m3");
  Serial.println();
 
  Serial.print("Temp: \t"); //온도 측정
  temp = pms.getTemp();
  Serial.print(temp);
  Serial.print("'C");
  Serial.println();
 
  Serial.print("Humi: \t"); //습도 측정
  humi = pms.getHumi();
  Serial.print(humi);
  Serial.print("%");
  Serial.println();
  Serial.println();

  if (pm10 > 75 || pm25 > 75 || pm100 > 150 || form > 922) {
    digitalWrite(bzrPin, HIGH);
  } else {
    digitalWrite(bzrPin, LOW);
  }

  clearLine(0);
  lcd.print("TEMP: ");
  lcd.print(temp);
  lcd.print("'C");

  clearLine(1);
  lcd.print("HUMI: ");
  lcd.print(humi);
  lcd.print("%");

  clearLine(2);
  lcd.print("PM10: ");
  lcd.print(pm10);
  lcd.print("ug/m3");

  clearLine(3);
  lcd.print("FORM: ");
  lcd.print(form);
  lcd.print("ug/m3");

  s_pm10 = (s_pm10 + pm10) / 2;
  s_pm25 = (s_pm25 + pm25) / 2;
  s_pm100 = (s_pm100 + pm100) / 2;
  s_form = (s_form + form) / 2;
  s_temp = (s_temp + temp) / 2;
  s_humi = (s_humi + humi) / 2;

  if (++postCount > 9 && MODE == 0) {
    String str = String("UNO ") + String(s_pm10) + String(" ") + String(s_pm25) + String(" ") + String(s_pm100) + String(" ") + String(s_form) + String(" ") + String(s_temp) + String(" ") + String(s_humi) + String(" ");
    esp.print(str);
    postCount = 0;
    s_pm10 = 0;
    s_pm25 = 0;
    s_pm100 = 0;
    s_form = 0;
    s_temp = 0;
    s_humi = 0;
  }

  // Bluetooth Mode
  if (Serial.available()) {
    char temp = Serial.read();
    if (temp == 'b') {
      MODE = 1;
      clearAll();
      clearLine(0);
      lcd.print("[Bluetooth MODE]");
      digitalWrite(blePin, HIGH);
    }
  }
  
  delay(1000);
}

void setup() {
  Serial.begin(9600);
  esp.begin(115200);
  pms.begin();
  pms.setMode(PASSIVE);

  // LED
  pinMode(blePin, OUTPUT);
  pinMode(bzrPin, OUTPUT);

  // LCD
  lcd.begin(20, 4);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("PKNU");
  lcd.setCursor(0, 1);
  lcd.print("AIR QUALITY METER");
}
 
void loop() {
  switch(MODE) {
    case 0:
      getPMS();
      break;
    case 1:
      getBLE();
      break;
  }
}

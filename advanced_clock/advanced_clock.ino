//-------------------------- НАСТРОЙКИ  AUTOCONNECT --------------------------
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
using WebServerClass = ESP8266WebServer;
#include <AutoConnect.h>
#include <LittleFS.h>
FS& FlashFS = LittleFS;
#define PARAM_FILE "/settings.json"

static const char PAGE_ELEMENTS[] PROGMEM = R"(
{
  "uri": "/settings",
  "title": "Настройки",
  "menu": true,
  "element": [
    {
      "name": "tablecss",
      "type": "ACStyle",
      "value": "table{font-family:arial,sans-serif;border-collapse:collapse;width:100%;color:black;}td,th{border:1px solid #dddddd;text-align:center;padding:8px;}tr:nth-child(even){background-color:#dddddd;}"
    },
    {
      "name": "owm_setup",
      "type": "ACText",
      "value": "Настройки OpenWeatherMap",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970"
    },
    {
      "name": "owm_apikey",
      "type": "ACInput",
      "label": "API-ключ",
      "placeholder": "32 знака"
    },
    {
      "name": "owm_city_id",
      "type": "ACInput",
      "label": "ID города",
      "placeholder": "6 знаков"
    },
    {
      "name": "hr_001",
      "type": "ACElement",
      "value": "<hr style=\"height:1px;border-width:0;color:gray;background-color:#52a6ed\">",
      "posterior": "par"
    },
    {
      "name": "clock_setup",
      "type": "ACText",
      "value": "Настройки времени",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970"
    },
    {
      "name": "clock_server",
      "type": "ACInput",
      "label": "NTP-сервер",
      "value": "pool.ntp.org",
      "placeholder": "pool.ntp.org"
    },
    {
      "name": "clock_offset",
      "type": "ACInput",
      "label": "Смещение времени",
      "value": "7200",
      "apply": "number",
      "pattern": "\\d*"
    },
    {
      "name": "clock_workdays_alarm_enabled",
      "type": "ACCheckbox",
      "value": "check",
      "label": "Использовать будильник по будням?",
      "labelposition": "infront",
      "checked": true
    },
    {
      "name": "clock_workdays_alarm_time",
      "type": "ACInput",
      "label": "Время будильников &#040;через разделитель /&#041;",
      "value": "07:30/07:40",
      "placeholder": "07:30/07:40"
    },
    {
      "name": "clock_restdays_alarm_enabled",
      "type": "ACCheckbox",
      "value": "check",
      "label": "Использовать будильник по выходным?",
      "labelposition": "infront",
      "checked": false
    },
    {
      "name": "clock_restdays_alarm_time",
      "type": "ACInput",
      "label": "Время будильников &#040;через разделитель /&#041;",
      "value": "07:30/07:40",
      "placeholder": "07:30/07:40"
    },
    {
      "name": "hr_002",
      "type": "ACElement",
      "value": "<hr style=\"height:1px;border-width:0;color:gray;background-color:#52a6ed\">",
      "posterior": "par"
    },
    {
      "name": "display_setup",
      "type": "ACText",
      "value": "Настройки времени",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970"
    },
    {
      "name": "display_backlight_schedule",
      "type": "ACCheckbox",
      "value": "check",
      "label": "Включать подсветку по времени?",
      "labelposition": "infront",
      "checked": true
    },
    {
      "name": "display_backlight_schedule_time",
      "type": "ACInput",
      "label": "Время работы подсветки &#040;через разделитель -&#041;",
      "value": "07:30-00:00",
      "placeholder": "07:30-00:00"
    },
    {
      "name": "display_active",
      "type": "ACSelect",
      "option": [
        "Погода/время",
        "Спектрометр"
      ],
      "label": "Текущий экран",
      "selected": 1
    },
    {
      "name": "load",
      "type": "ACSubmit",
      "value": "Load",
      "uri": "/settings"
    },
    {
      "name": "save",
      "type": "ACSubmit",
      "value": "Save",
      "uri": "/save"
    },
    {
      "name": "adjust_width",
      "type": "ACElement",
      "value": "<script type=\"text/javascript\">window.onload=function(){var t=document.querySelectorAll(\"input[type='text']\");for(i=0;i<t.length;i++){var e=t[i].getAttribute(\"placeholder\");e&&t[i].setAttribute(\"size\",e.length*.8)}};</script>"
    }
  ]
}
)";

static const char PAGE_SAVE[] PROGMEM = R"(
{
  "uri": "/save",
  "title": "Настройки",
  "menu": false,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "format": "Настройки сохранены в файл %s",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970"
    },
    {
      "name": "ok",
      "type": "ACSubmit",
      "value": "OK",
      "uri": "/settings"
    }
  ]
}
)";

WebServerClass  server;
AutoConnect portal(server);
AutoConnectConfig config;
AutoConnectAux  elementsAux;
AutoConnectAux  saveAux;
//------------------------// НАСТРОЙКИ  AUTOCONNECT //------------------------
//----------------------------------- ПИНЫ -----------------------------------
const int buttonPin = 12;
const int vibroPin = 13;
#define DHTPIN D5

//---------------------------------// ПИНЫ //---------------------------------
//-------------------- ОБЪЯВЛЕНИЕ ИСПОЛЬЗУЕМЫХ ПЕРЕМЕННЫХ --------------------
char      owm_apikey[64];
char      owm_city_id[32];
char      clock_server[32];
char      clock_offset[32];
bool      clock_workdays_alarm_enabled;
char      clock_workdays_alarm_time[32];
bool      clock_restdays_alarm_enabled;
char      clock_restdays_alarm_time[32];
bool      display_backlight_schedule;
char      display_backlight_schedule_time[32];
long      display_active;
//------------------// ОБЪЯВЛЕНИЕ ИСПОЛЬЗУЕМЫХ ПЕРЕМЕННЫХ //------------------
//------------------------------ ИНИЦИАЛИЗАЦИЯ -------------------------------
#include <DHT.h>
#include <DHT_U.h>
#include <NTPClient.h>
#include <Arduino_JSON.h>
#include "GyverButton.h"
#define _LCD_TYPE 1
#include <LCD_1602_RUS_ALL.h>
#define DHTTYPE    DHT11
DHT_Unified dht(DHTPIN, DHTTYPE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String lang = "ru";
String jsonBuffer;
String temp_inside;
unsigned long lastTime = -50000;
unsigned long timerDelay = 50000;
unsigned long lastClockTime = 0;
unsigned long clockDelay = 500;
char      daysOfTheWeek[7][12] = {"Вс", "Пн", "Вт", "Ср", "Чт", "Пт", "Сб"};
String current_mode = "time_weather_mode";
GButton butt1(buttonPin);
LCD_1602_RUS lcd(0x27, 16, 2);
boolean   read_config_file = false;
//----------------------------// ИНИЦИАЛИЗАЦИЯ //-----------------------------
//------------------------------- СПЕКТРОМЕТР --------------------------------
#define DRIVER_VERSION 1    // 0 - маркировка драйвера кончается на 4АТ, 1 - на 4Т
#define AUTO_GAIN 1         // автонастройка по громкости (экспериментальная функция)
#define VOL_THR 35          // порог тишины (ниже него отображения на матрице не будет)
#define LOW_PASS 30         // нижний порог чувствительности шумов (нет скачков при отсутствии звука)
#define DEF_GAIN 60         // максимальный порог по умолчанию
#define SAMPLES 128         // ширина спектра х2
// вручную забитый массив тонов, сначала плавно, потом круче
byte posOffset[16] = {2, 3, 4, 6, 8, 10, 12, 14, 16, 20, 25, 30, 35, 60, 80, 100};
#define SAMPLING_FREQUENCY 10000 //Hz, must be 10000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT.
unsigned int sampling_period_us;
#define LOG_OUT 1
#define printByte(args) write(args);
//double prevVolts = 100.0;
byte gain = DEF_GAIN;   // усиление по умолчанию
unsigned long gainTimer;
byte maxValue, maxValue_f;
float k = 0.1;

double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned long newTime, oldTime;

#include "arduinoFFT.h"
arduinoFFT FFT = arduinoFFT();


byte v1[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111};
byte v2[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111};
byte v3[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111};
byte v4[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111};
byte v5[8] = {0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
byte v6[8] = {0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
byte v7[8] = {0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
byte v8[8] = {0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
void lcdChars() {
  lcd.createChar(0, v1);
  lcd.createChar(1, v2);
  lcd.createChar(2, v3);
  lcd.createChar(3, v4);
  lcd.createChar(4, v5);
  lcd.createChar(5, v6);
  lcd.createChar(6, v7);
  lcd.createChar(7, v8);
}

//-----------------------------// СПЕКТРОМЕТР //------------------------------
size_t utf8len(char *s)
  {
    size_t len = 0;
    for (; *s; ++s) if ((*s & 0xC0) != 0x80) ++len;
    return len;
  }

void lcd_print_center(int line_number, String symbols, int offset_symbols) {
  
  char symbols_char[symbols.length() + 1];
  symbols.toCharArray(symbols_char, symbols.length() + 1);
  int s_length = utf8len(symbols_char);
  lcd.setCursor(0, line_number);
  if (s_length%2) {
    symbols.replace(" ", "  ");
  }
  lcd.setCursor((16-s_length)/2+offset_symbols, line_number);
  lcd.print(symbols);
}

String httpGETRequest(const char* serverName) {
  HTTPClient http;
  http.begin(serverName);
  int httpResponseCode = http.GET();
  String payload = "{}"; 
  if (httpResponseCode>0) {
    payload = http.getString();
  }
  http.end();
  return payload;
}

void display_weather() {
  if(WiFi.status()== WL_CONNECTED){
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?id=" + String(owm_city_id) + "&lang=" + lang + "&units=metric&APPID=" + String(owm_apikey);
    jsonBuffer = httpGETRequest(serverPath.c_str());
    JSONVar myObject = JSON.parse(jsonBuffer);
    if (JSON.typeof(myObject) == "undefined") {
      return;
    }
    lcd.clear();
    String temp = JSON.stringify(myObject["main"]["temp"]);
    String feels_like = JSON.stringify(myObject["main"]["feels_like"]);
    String pressure = JSON.stringify(myObject["main"]["pressure"]);
    String humidity = JSON.stringify(myObject["main"]["humidity"]);
    String wind_speedy = JSON.stringify(myObject["wind"]["speed"]);
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      lcd_print_center(0, "Улица: " + temp + "°C", 0);
      }
    else {
      temp_inside = String(event.temperature).substring(0, String(event.temperature).length()-1);
      lcd_print_center(0, temp_inside + "°C " + temp + "°C", 0);
    }
    
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}
void display_time() {
  if(WiFi.status()== WL_CONNECTED){
    timeClient.setTimeOffset(atol(clock_offset));
    timeClient.setPoolServerName(clock_server);
    timeClient.update();
    String str_cur_h;
    String str_cur_m;
    String week_day = daysOfTheWeek[timeClient.getDay()];
    int cur_h = timeClient.getHours();
    if (cur_h < 10) {
      str_cur_h = "0" + String(cur_h);
    } else {
      str_cur_h = String(cur_h);
    }
    int cur_m = timeClient.getMinutes();
    if (cur_m < 10) {
      str_cur_m = "0" + String(cur_m);
    } else {
      str_cur_m = String(cur_m);
    }
    
    String date_string = week_day + ", " + str_cur_h + ":" + str_cur_m;
    lcd_print_center(1, date_string, 0);
    if (cur_h < 8) {
      lcd.noBacklight();
    } else {
      lcd.backlight();
    }
    if (cur_h == 7){
      if (cur_m == 30){
        digitalWrite(vibroPin, HIGH);
        }
      if (cur_m == 40){
        digitalWrite(vibroPin, HIGH);
        }
      if (cur_m == 50){
        digitalWrite(vibroPin, HIGH);
        }
      }
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}

void rootPage() {
  String content = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
Place the root page with the sketch application.&ensp;
__AC_LINK__
</body>
</html>
    )";
    content.replace("__AC_LINK__", String(AUTOCONNECT_LINK(COG_16)));
    server.send(200, "text/html", content);
  }
void show_ip() {
  String content = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
IP-адрес:
__AC_LINK__
</body>
</html>
    )";
    content.replace("__AC_LINK__", String(WiFi.localIP().toString()));
    server.send(200, "text/html", content);
    lcd.clear();
    lcd.backlight(); 
    lcd_print_center(0, WiFi.localIP().toString(), 0);
    lcd_print_center(1, WiFi.SSID(), 0);
    delay(2000);
    if (current_mode == "time_weather_mode") {
      display_time();
      display_weather();
    }
  }
void time_mode() {
  String content = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
Переключаем на режим просмотра времени
</body>
</html>
    )";
    server.send(200, "text/html", content);
    current_mode = "time_weather_mode";
    lcd.clear();
    lcd.backlight();
    lcd_print_center(0, "Time mode", 0);
    lcd_print_center(1, "enabled", 0);
    delay(2000);
    display_weather();
    display_time();
  }

void spectrum_mode() {
  String content = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
Переключаем на режим просмотра спектра аудио
</body>
</html>
    )";
    server.send(200, "text/html", content);
    current_mode = "spectrum_mode";
    lcd.clear();
    lcd.backlight(); 
    lcd_print_center(0, "Spectrum mode", 0);
    lcd_print_center(1, "enabled", 0);
    lcdChars();
    delay(2000);
  }

bool read_config() {
    FlashFS.begin();
    File configFile = FlashFS.open(PARAM_FILE, "r");
    if (!configFile) {
      Serial.println("Failed to open config file");
      return false;
    }
    size_t size = configFile.size();
    if (size > 4096) {
      Serial.println("Config file size is too large");
      return false;
    }
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);
    DynamicJsonDocument doc(4096);
    auto error = deserializeJson(doc, buf.get());
    if (error) {
      Serial.println("Failed to parse config file");
      return false;
    }
    JsonObject root_1  =  doc[1]; // "owm_apikey"
    JsonObject root_2  =  doc[2]; // "owm_city_id"
    JsonObject root_4  =  doc[4]; // "clock_server"
    JsonObject root_5  =  doc[5]; // "clock_offset"
    JsonObject root_6  =  doc[6]; // "clock_workdays_alarm_enabled"
    JsonObject root_7  =  doc[7]; // "clock_workdays_alarm_time"
    JsonObject root_8  =  doc[8]; // "clock_restdays_alarm_enabled"
    JsonObject root_9  =  doc[9]; // "clock_restdays_alarm_time"
    JsonObject root_11 = doc[11]; // "display_backlight_schedule"
    JsonObject root_12 = doc[12]; // "display_backlight_schedule_time"
    JsonObject root_13 = doc[13]; // "display_active"
    const char* owm_apikey_char = root_1["value"];
    strlcpy(owm_apikey, owm_apikey_char, 64);
    const char* owm_city_id_char = root_2["value"];
    strlcpy(owm_city_id, owm_city_id_char, 32);
    const char* clock_server_char = root_4["value"];
    strlcpy(clock_server, clock_server_char, 32);
    const char* clock_offset_char = root_5["value"];
    strlcpy(clock_offset, clock_offset_char, 32);
    clock_workdays_alarm_enabled = root_6["checked"];
    const char* clock_workdays_alarm_time_char = root_7["value"];
    strlcpy(clock_workdays_alarm_time, clock_workdays_alarm_time_char, 32);
    clock_restdays_alarm_enabled = root_8["checked"];
    const char* clock_restdays_alarm_time_char = root_9["value"];
    strlcpy(clock_restdays_alarm_time, clock_restdays_alarm_time_char, 32);
    display_backlight_schedule = root_11["checked"];
    const char* display_backlight_schedule_time_char = root_12["value"];
    strlcpy(display_backlight_schedule_time, display_backlight_schedule_time_char, 32);
    display_active = root_13["selected"];
    Serial.println("config read ok");
  }

void setup() {
  pinMode(vibroPin, OUTPUT);
  digitalWrite(vibroPin, LOW);
  Serial.begin(115200);
  butt1.setDebounce(50);
  butt1.setTimeout(300);
  butt1.setClickTimeout(600);
  butt1.setType(LOW_PULL);
  butt1.setDirection(NORM_OPEN);
  lcd.init(); 
  lcd.backlight();
  server.on("/", rootPage);
  server.on("/ip", show_ip);
  server.on("/time_mode", time_mode);
  server.on("/spectrum_mode", spectrum_mode);
  elementsAux.load(FPSTR(PAGE_ELEMENTS));
  elementsAux.on([] (AutoConnectAux& aux, PageArgument& arg) {
    if (portal.where() == "/settings") {
        FlashFS.begin();
        File param = FlashFS.open(PARAM_FILE, "r");
        if (param) {
          aux.loadElement(param, { "owm_setup", "owm_apikey", "owm_city_id", "clock_setup", "clock_server", "clock_offset", "clock_workdays_alarm_enabled", "clock_workdays_alarm_time", "clock_restdays_alarm_enabled", "clock_restdays_alarm_time", "display_setup", "display_backlight_schedule", "display_backlight_schedule_time", "display_active" } );
          param.close();
        }
        FlashFS.end();
    }
    return String();
  });

  saveAux.load(FPSTR(PAGE_SAVE));
  saveAux.on([] (AutoConnectAux& aux, PageArgument& arg) {
    aux["caption"].value = PARAM_FILE;
    FlashFS.begin();
    File param = FlashFS.open(PARAM_FILE, "w");
    if (param) {
      elementsAux.saveElement(param, { "owm_setup", "owm_apikey", "owm_city_id", "clock_setup", "clock_server", "clock_offset", "clock_workdays_alarm_enabled", "clock_workdays_alarm_time", "clock_restdays_alarm_enabled", "clock_restdays_alarm_time", "display_setup", "display_backlight_schedule", "display_backlight_schedule_time", "display_active" });
      param.close();
    }
    FlashFS.end();
    read_config_file = true;
    return String();
  });

  portal.join({ elementsAux, saveAux });
  config.ota = AC_OTA_BUILTIN;
  portal.config(config);
  portal.begin();
  read_config();
  lcdChars();   // подхватить коды полосочек
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  timeClient.begin();
  lcd_print_center(0, WiFi.localIP().toString(), 0);
  lcd_print_center(1, WiFi.SSID(), 0);
  delay(2000);
  
}


void loop() {
  portal.handleClient();
  if (read_config_file) {
    read_config_file = false;
    read_config();
    if (display_active == 1) {
      time_mode();
    }
    if (display_active == 2) {
      spectrum_mode();
    }
  }
  butt1.tick();
  if (butt1.isSingle()) {
    digitalWrite(vibroPin, HIGH);
    show_ip();
  }
  if (butt1.isDouble()) {
    spectrum_mode();
  }
  if (butt1.isTriple()) {
    display_weather();
  }
  if (butt1.isHold()) {
    digitalWrite(vibroPin, LOW);
    time_mode();
  }
  if (butt1.hasClicks()) {
    Serial.println(butt1.getClicks());
    lcd_print_center(0, "                 ", 0);
    lcd_print_center(0, String(butt1.getClicks()), 0);
    if (current_mode == "time_weather_mode") {
      display_weather();
    }
    if (butt1.getClicks() == 10) {
      ESP.restart();
    }
  }
  if (current_mode == "time_weather_mode") {
    if ((millis() - lastTime) > timerDelay) {
      lastTime = millis();
      display_weather();
    }
    if ((millis() - lastClockTime) > clockDelay){
      lastClockTime = millis();
      display_time();  
    }
  }

  if (current_mode == "spectrum_mode") {
    for (int i = 0; i < SAMPLES; i++) {
      newTime = micros()-oldTime;
      oldTime = newTime;
      vReal[i] = analogRead(A0); // A conversion takes about 1mS on an ESP8266
      vImag[i] = 0;
      while (micros() < (newTime + sampling_period_us)) { yield(); }
    }
    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
    int total_lines = 0;
    for (int pos = 0; pos < 16; pos++) {   // для окошек дисплея с 0 по 15
      // найти максимум из пачки тонов
      if (vReal[posOffset[pos]] > maxValue) maxValue = vReal[posOffset[pos]];
  
      lcd.setCursor(pos, 0);
  
      // преобразовать значение величины спектра в диапазон 0..15 с учётом настроек
      int posLevel = map(vReal[posOffset[pos]], LOW_PASS, gain, 0, 15);
      posLevel = constrain(posLevel, 0, 15);
  
      if (posLevel > 7) {               // если значение больше 7 (значит нижний квадратик будет полный)
        lcd.printByte(posLevel - 8);    // верхний квадратик залить тем что осталось
        lcd.setCursor(pos, 1);          // перейти на нижний квадратик
        lcd.printByte(7);               // залить его полностью
      } else {                          // если значение меньше 8
        lcd.print(" ");                 // верхний квадратик пустой
        lcd.setCursor(pos, 1);          // нижний квадратик
        lcd.printByte(posLevel);        // залить полосками
      }
    }
    if (AUTO_GAIN) {
       maxValue_f = maxValue * k + maxValue_f * (1 - k);
      if (millis() - gainTimer > 1500) {      // каждые 1500 мс
        // если максимальное значение больше порога, взять его как максимум для отображения
        if (maxValue_f > VOL_THR) gain = maxValue_f;
  
        // если нет, то взять порог побольше, чтобы шумы вообще не проходили
        else gain = 100;
        gainTimer = millis();
      }
    }
  }
}

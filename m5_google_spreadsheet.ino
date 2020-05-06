#include <M5StickC.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi Setting
const char* ssid = "your-ssid";
const char* password = "your-password";

// HTTP Setting
const char* host = "https://script.google.com/macros/~~~~"; // Deployed web app URL

void setup_wifi() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid, password);

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_time() {
  configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");
}

void send_get_request(const char *url, String &body) {
  HTTPClient http;
  
  Serial.print("[HTTP] begin...\n");
  // configure traged server and url
  http.begin(url); //HTTP

  const char* headerNames[] = {"Location"};
  http.collectHeaders(headerNames, sizeof(headerNames) / sizeof(headerNames[0]));
  
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
//      Serial.println(payload);
      body = payload;
    }
    else if (httpCode == HTTP_CODE_FOUND) {
      String payload = http.getString();
//      Serial.println(payload);

      Serial.printf("[HTTP] GET... Location: %s\n", http.header("Location").c_str());

      send_get_request(http.header("Location").c_str(), body);
//      Serial.println(body);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

void send_post_request(const char *url, const String &body) {
  HTTPClient http;
  
  Serial.print("[HTTP] begin...\n");
  // configure traged server and url
  http.begin(host); //HTTP

  const char* headerNames[] = {"Location"};
  http.collectHeaders(headerNames, sizeof(headerNames) / sizeof(headerNames[0]));
  
  Serial.print("[HTTP] POST...\n");
  // start connection and send HTTP header
  int httpCode = http.POST(body);
  
  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);
   
    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
//      Serial.println(payload);
    }
    else if (httpCode == HTTP_CODE_FOUND) {
      String payload = http.getString();
//      Serial.println(payload);
      
      Serial.printf("[HTTP] POST... Location: %s\n", http.header("Location").c_str());

      String message_body;
      send_get_request(http.header("Location").c_str(), message_body);
//      Serial.println(message_body);
    }
  } else {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

void setup() {
  // put your setup code here, to run once:
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("Web API test");

  //!IMU
  M5.IMU.Init();
  Serial.printf("imuType = %d\n",M5.IMU.imuType);

  //!IR LED
  pinMode(M5_BUTTON_HOME, INPUT);
  pinMode(M5_BUTTON_RST, INPUT);

  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  delay(100);

  setup_wifi();
  setup_time();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead(M5_BUTTON_HOME) == LOW) {
    while(digitalRead(M5_BUTTON_HOME) == LOW);

    String res_message_body;

    send_get_request(host, res_message_body);

    Serial.println(res_message_body);

    StaticJsonDocument<500> res_doc;

    deserializeJson(res_doc, res_message_body);

    const char* prev_time = res_doc["time"];
    const float prev_accX_f = res_doc["accX"];
    const float prev_accY_f = res_doc["accY"];
    const float prev_accZ_f = res_doc["accZ"];

    float accX_f = 0;
    float accY_f = 0;
    float accZ_f = 0;
    M5.IMU.getAccelData(&accX_f,&accY_f,&accZ_f);

    struct tm timeInfo;
    getLocalTime(&timeInfo); // 時刻を取得

    char time[50];
    sprintf(time, "%04d/%02d/%02d %02d:%02d:%02d",
      timeInfo.tm_year+1900, timeInfo.tm_mon+1, timeInfo.tm_mday,
      timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Accel Data");
    M5.Lcd.printf("Prev: %s\n", prev_time);
    M5.Lcd.printf(" X: %f\n", prev_accX_f);
    M5.Lcd.printf(" Y: %f\n", prev_accY_f);
    M5.Lcd.printf(" Z: %f\n", prev_accZ_f);
    M5.Lcd.printf("New: %s\n", time);
    M5.Lcd.printf(" X: %f\n", accX_f);
    M5.Lcd.printf(" Y: %f\n", accY_f);
    M5.Lcd.printf(" Z: %f\n", accZ_f);

    //jsonデータ作成
    StaticJsonDocument<500> doc;
    doc["time"] = time;
    doc["accX"] = accX_f;
    doc["accY"] = accY_f;
    doc["accZ"] = accZ_f;

    String message_body;

    serializeJson(doc, message_body);

    Serial.println(message_body);

    send_post_request(host, message_body);
  }
}

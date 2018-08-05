#include <OLEDDisplayUi.h>
#include <SSD1306Spi.h>
#define USE_SERIAL Serial
#define DC  (17)
#define CS  (16)
#define RES  (4)
int screenW = 128;
int screenH = 64;
int CenterX = screenW/2;
int CenterY = ((screenH+16)/2);   // top yellow part is 16 px height
SSD1306Spi display(RES, DC, CS);
OLEDDisplayUi ui (&display);
#include "xbmpicture.h"
#include "englishfont.h"
//OLED head need
//#include "cJSON.h"
//cJSON* weather_data;
//char data_buffer[15000] = {};
#include <WiFi.h>

//#include <HTTPClient.h>
#include "time.h"
const char* ssid       = "UniqueStudio-810";
const char* password   = "uniquestudio";

const char* ntpServer = "1.cn.pool.ntp.org";
const long  gmtOffset_sec = 3600*8;
const int   daylightOffset_sec = 0;
//NTPTime need
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#define SERVICE_UUID        "91bad492-b950-4226-aa2b-4ede9fa42f59"
#define CHARACTERISTIC_UUID "0d563a58-196a-48ce-ace2-dfec78acc814"

struct Received {
  bool QQ = false;
  bool Wechat = false;
  bool msg = false;
  bool call = false;
} received;

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        Serial.println("*********");
        Serial.print("New value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);

        Serial.println();
        Serial.println("*********");

        if (value == "QQ")
          received.QQ = true;
        else if (value == "WC")
          received.Wechat = true;
        else if (value == "msg")
          received.msg = true;
        else if (value == "call")
          received.call = true;
      }
    }
};
//BLE

int threshold = 30;
bool touch1detected = false;
bool touch2detected = false;

void gotTouch1(){
  touch1detected = true;  
}

void gotTouch2(){
 touch2detected = true;
}
//touch interrupt

String twoDigits(int digits){
  if(digits < 10) {
    String i = '0'+String(digits);
    return i;
  }
  else {
    return String(digits);
  }
}

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void TimeSync(void) {
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);  
}

void mainpageFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  const int8_t dotR = 27;
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(DialogInput_plain_8);
  display->fillCircle(CenterX-40, 29, dotR/2);
  display->fillCircle(CenterX, 29, dotR/2);
  display->fillCircle(CenterX+40, 29, dotR/2);
  display->drawXbm(CenterX-50, 47,msg_width, msg_height, msg_bits);
  display->drawXbm(CenterX-10, 47,weather_width, weather_height, weather_bits);
  display->drawString(CenterX+40, 47, "GPS");
}

void msgFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(DialogInput_plain_8);

  if (received.call) {
    display->drawString(CenterX, CenterY, "Calling");
    delay(1000);
    received.call = false;
  }
  else if (received.msg) {
    display->drawString(CenterX, CenterY, "New Message");
    delay(1000);
    received.msg = false;
  }
  else if (received.QQ) {
    display->drawString(CenterX, CenterY, "QQ Message");
    delay(1000);
    received.QQ = false;
  }
  else if (received.Wechat) {
    display->drawString(CenterX, CenterY, "New Wechat");
    delay(1000);
    received.Wechat = false;
  }
  else {
    display->drawString(CenterX, CenterY, "EMPTY");
    delay(1000);
  }
    
}

void topOverlay(OLEDDisplay *display, OLEDDisplayUiState *state) {
  struct tm timeinfo;
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(DialogInput_plain_8);
  getLocalTime(&timeinfo);
    
  String timenow = String(timeinfo.tm_hour)+":"+twoDigits(timeinfo.tm_min);

  display->drawString(64, 0, timenow );
  
}

FrameCallback frames[] = { mainpageFrame};
int frameCount = 1;

OverlayCallback overlays[]= {topOverlay};
int overlaysCount = 1;

void setup() {
  Serial.begin(115200);

  Serial.println("The device started, now you can pair it with bluetooth!");
  TimeSync();

  ui.setTargetFPS(45);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, frameCount);
  ui.disableAllIndicators();
  ui.setTimePerTransition(20);
  // Initialising the UI will init the display too.
  ui.setOverlays(overlays, overlaysCount);
  ui.init();
  display.flipScreenVertically();

  BLEDevice::init("BitGlass");
  BLEServer *Server = BLEDevice::createServer();
  BLEService *msgService = Server->createService(SERVICE_UUID);
  BLECharacteristic *whichApp = msgService->createCharacteristic(
                                            CHARACTERISTIC_UUID,
                                            BLECharacteristic::PROPERTY_READ |
                                            BLECharacteristic::PROPERTY_WRITE
                                            );
  whichApp->setCallbacks(new MyCallbacks());
  whichApp->setValue(0);
  msgService->start();

  BLEAdvertising *pAdvertising = Server->getAdvertising();
  pAdvertising->start();

  touchAttachInterrupt(T7, gotTouch1, threshold);
  touchAttachInterrupt(T5, gotTouch2, threshold);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
    if(touch1detected){
      touch1detected = false;
      ui.nextFrame();
      delay(1);
    }
    if(touch2detected){
      touch2detected = false;
      ui.previousFrame();
      delay(1);
    }

      // Don't do stuff if you are below your
      // time budget.
      delay(remainingTimeBudget);
  }
}

/*
 * Implementation for OV7670 Camera + HLK LD2420 Radar Sensor
 * 
 * This file contains the implementation of methods to capture images using the OV7670 camera,
 * detect motion with the LD2420 radar sensor, and send alerts/images to Telegram automatically,
 * mimicking a surveillance camera system.
 * 
 * Author: vuvvvv
 * Repository/Reference: https://github.com/vuvvvv/Cam-Alert-CCTv
 */

//-----------------------------------------------------------------main file-----------------------------------------------------------------------------
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include "OV7670.h"
#include "BMP.h"
#include "time.h"
#include "serve_web.h"
#include "send_text.h"
#include "LD2420.h"
#include "send_photobmp.h"
#include "bmp_to_jpg.h"
extern "C" {
#include "esp_camera.h"
#include "img_converters.h"
}




//--------------camera OV7670 pin------------------
#define PWDN_PIN -1
#define RESET_PIN 18
#define XCLK_PIN 32
#define VSYNC_PIN 34
#define HREF_PIN 35
#define PCLK_PIN 33

#define D0_PIN 19
#define D1_PIN 23
#define D2_PIN 13
#define D3_PIN 12
#define D4_PIN 14
#define D5_PIN 27
#define D6_PIN 26
#define D7_PIN 25


#define SDA_PIN 21
#define SCL_PIN 22
//------------------------------------

// --------------sensor pin-----------------
#define LD2420_RX 16  // ESP RX2 → LD2420 OT1 (TX)
#define LD2420_TX 17  // ESP TX2 → LD2420 RX
//-------------------------------------------

// ---------------wifi setup--------------
// ⚠️ Requires a 2.4GHz Wi-Fi network (WPA2-PSK). This is very important for proper connection.

// wifi name
const char* ssid = "****************************";
// wifi pass 
const char* password = "****************************";
//------------------------------------

// -----------telegram setup---------------------

// ⚠️ Token is important  use yours token
const char* BOT_TOKEN = "************************************************";

// ⚠️ Chat ID is important  use yours id 
const char* CHAT_ID = "***********************************";

// ⚠️⚠️⚠️ Do NOT modify this line! The certificate is required for Telegram security.
const char* TELEGRAM_CERTIFICATE_ROOT = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDxTCCAq2gAwIBAgIBADANBgkqhkiG9w0BAQsFADCBgzELMAkGA1UEBhMCVVMx
EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxGjAYBgNVBAoT
EUdvRGFkZHkuY29tLCBJbmMuMTEwLwYDVQQDEyhHbyBEYWRkeSBSb290IENlcnRp
ZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTA5MDkwMTAwMDAwMFoXDTM3MTIzMTIz
NTk1OVowgYMxCzAJBgNVBAYTAlVTMRAwDgYDVQQIEwdBcml6b25hMRMwEQYDVQQH
EwpTY290dHNkYWxlMRowGAYDVQQKExFHb0RhZGR5LmNvbSwgSW5jLjExMC8GA1UE
AxMoR28gRGFkZHkgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgLSBHMjCCASIw
DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL9xYgjx+lk09xvJGKP3gElY6SKD
E6bFIEMBO4Tx5oVJnyfq9oQbTqC023CYxzIBsQU+B07u9PpPL1kwIuerGVZr4oAH
/PMWdYA5UXvl+TW2dE6pjYIT5LY/qQOD+qK+ihVqf94Lw7YZFAXK6sOoBJQ7Rnwy
DfMAZiLIjWltNowRGLfTshxgtDj6AozO091GB94KPutdfMh8+7ArU6SSYmlRJQVh
GkSBjCypQ5Yj36w6gZoOKcUcqeldHraenjAKOc7xiID7S13MMuyFYkMlNAJWJwGR
tDtwKj9useiciAF9n9T521NtYJ2/LOdYq7hfRvzOxBsDPAnrSTFcaUaz4EcCAwEA
AaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYE
FDqahQcQZyi27/a9BUFuIMGU2g/eMA0GCSqGSIb3DQEBCwUAA4IBAQCZ21151fmX
WWcDYfF+OwYxdS2hII5PZYe096acvNjpL9DbWu7PdIxztDhC2gV7+AJ1uP2lsdeu
9tfeE8tTEH6KRtGX+rcuKxGrkLAngPnon1rpN5+r5N9ss4UXnT3ZJE95kTXWXwTr
gIOrmgIttRD02JDHBHNA7XIloKmf7J6raBKZV8aPEjoJpL1E/QYVN8Gb5DKj7Tjo
2GTzLH4U/ALqn83/B2gX2yKQOC16jdFU8WnjXzPKej17CuPKf1855eJ1usV2GDPO
LPAvTK33sefOT6jEm0pUBsV/fdUID+Ic/n4XuKxe9tQWskMJDE32p2u0mYRlynqI
4uJEvlz36hz1
-----END CERTIFICATE-----
)EOF";
//--------------------------------------------------------------------

// ---------------------important varbles---------------------------
//time
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3 * 3600;
const int daylightOffset_sec = 0;

OV7670* camera;
WiFiServer server(80);
WiFiClientSecure secureClient;
unsigned char bmpHeader[BMP::headerSize];


String streamHost = "";
int streamPort = 80;
String streamPath = "/camera";

Preferences prefs;

LD2420 ld2420;

static int peopleCount = 0;
static bool lastPresence = false;
static unsigned long lastSend = 0;
const unsigned long minInterval = 3000;


static bool triggerLock = false;
static unsigned long presenceEndTime = 0;
const unsigned long clearTimeNeeded = 5000;

// ⚠️ You can change the detection distance, but note that the LD2420 sensor’s maximum range is 8 meters.
const int MAX_DETECTION_CM = 700; // is cm convert to meters {100cm = 1m} but we used cm 
//-------------------------------------------------------------------------------------------------------
const unsigned long ALERT_INTERVAL = 30000;
unsigned long lastAlertTime = 0;

// -------------------------------------------------------------------




// ---------------- setup ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n ESP32 Camera Stream Relay");


  prefs.begin("camera", false);
  String saved = prefs.getString("streamHost", "");
  if (saved.length() > 0) {
    streamHost = saved;
    Serial.printf("Loaded streamHost from memory: %s\n", streamHost.c_str());
  } else {
    Serial.println("No stored streamHost");
  }


  Serial.printf("Connecting to %s", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println();
  Serial.println(WiFi.localIP());


  Serial.println("Initializing camera OV7670...");
  camera = new OV7670(OV7670::Mode::QQVGA_RGB565, SDA_PIN, SCL_PIN,
                      VSYNC_PIN, HREF_PIN, XCLK_PIN, PCLK_PIN,
                      D0_PIN, D1_PIN, D2_PIN, D3_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);
  camera->setRegister(0x13, 0xE7);  // AWB, AGC, AEC enabled
  camera->setRegister(0x0E, 0x61);  // Sleep mode off, enable all
  camera->setRegister(0x60, 0x50);  // Brightness
  camera->setRegister(0x80, 0x50);  // Contrast
  camera->setRegister(0x64, 0xF0);  // Gain
  camera->setRegister(0x65, 0x00);
  camera->setRegister(0x66, 0x00);
  camera->setRegister(0x9B, 0x02);
  camera->setRegister(0x00, 0xF0);




  BMP::construct16BitHeader(bmpHeader, camera->xres, camera->yres);


  Serial2.begin(115200, SERIAL_8N1, LD2420_RX, LD2420_TX);


  if (ld2420.begin(Serial2)) {
    Serial.println("LD2420 initialized successfully");
    ld2420.setUpdateInterval(10);
  } else {
    Serial.println("LD2420 init FAILED check wiring / baud / power");
  }

  server.begin();
  Serial.println("Server started on port 80");
  Serial.println("Open the browser at: http://" + WiFi.localIP().toString());
  Serial.println("==================================================");


  Serial.println("\nconnection to Telegram");
  delay(2000);
  bool testResult = sendTextToTelegram(" ESP32 Connected! IP: " + WiFi.localIP().toString());
  if (testResult) {
    Serial.println("Telegram connection is working correctly!");
  } else {
    Serial.println("Failed to connect to Telegram check the settings");
  }
}
//-----------------------------------------------------------------------------------

//-------------------------------------Loop------------------------------------------
void loop() {
  serve();


  ld2420.update();

  bool presence = ld2420.isDetecting();
  int distance = ld2420.getDistance();

  if (presence && distance > MAX_DETECTION_CM) {
    presence = false;
    distance = 0;
    peopleCount = 0;


    unsigned long now = millis();
    if (now - lastAlertTime >= ALERT_INTERVAL) {
      Serial.printf("Distance (%d cm) exceeded the maximum (%d cm). Empty state enforced.\n", distance, MAX_DETECTION_CM);
      lastAlertTime = now;
    }
  }


  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint > 5000) {
    Serial.printf("Detected: %s | Distance: %d cm | Lock: %s\n",
                  presence ? "Yes" : "No",
                  distance,
                  triggerLock ? "Locked" : "Open");
    lastDebugPrint = millis();
  }

  if (triggerLock) {


    if (!presence) {

      if (presenceEndTime == 0) {
        presenceEndTime = millis();
        Serial.println("Exit complete Starting rearm countdown.");
      }


      if (millis() - presenceEndTime >= clearTimeNeeded) {
        triggerLock = false;
        presenceEndTime = 0;
        Serial.println("Camera system rearmed Ready for a new capture.");
      }
    } else {

      if (presenceEndTime != 0) {
        presenceEndTime = 0;
        Serial.println("Motion detected again Resetting rearm countdown.");
      }
    }
  }


  if (presence && !lastPresence && !triggerLock) {


    if (millis() - lastSend > minInterval) {


      triggerLock = true;
      lastSend = millis();
      peopleCount++;
      presenceEndTime = 0;


      String alertMessage = String("⚠️ Alert ⚠️\n") + "Motion detected at " + String(distance) + " cm\n" + "Number of people: " + String(peopleCount) + "\n" + "Time: " + getFormattedTime();

      Serial.printf("New person! Distance: %d cm | Count: %d. Locked.\n", distance, peopleCount);
      sendTextToTelegram(alertMessage);


      camera->oneFrame();


      if (camera->frame == nullptr || camera->xres == 0 || camera->yres == 0) {
        Serial.println("Failed to capture image: Invalid data");
      } else {

        flipRGB565Vertically(camera->frame, camera->xres, camera->yres);
        swap_rgb565_bytes(camera->frame, camera->xres * camera->yres * 2);


        uint8_t* jpegData = nullptr;
        size_t jpegSize = 0;

        if (convertBMPtoJPEG(camera->frame, camera->xres, camera->yres, &jpegData, &jpegSize, 80, false)) {
          bool sent = sendPhotoToTelegram(jpegData, jpegSize);
          Serial.println(sent ? "Sent image" : "Failed to send");

          free(jpegData);
        } else {
          Serial.println("Failed to convert image to JPEG");
        }
      }
    } else {
      Serial.println("Person detected, but cooldown period has not ended yet.");
    }
  }

  lastPresence = presence;

  delay(20);
}
//------------------------------------------------------------------------------------------------------------

// -----------------get time------------------------------------------
String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Time unavailable";
  }
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}
//--------------------------------------------------------------------

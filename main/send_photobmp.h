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

#include <WiFi.h>
#include <WiFiClientSecure.h>



extern WiFiClientSecure secureClient;

bool sendPhotoToTelegram(uint8_t* jpgData, size_t jpgSize) {
  
  bool success = false;
  
  secureClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  
  if (!secureClient.connect("api.telegram.org", 443)) {
    Serial.println("Failed to connect to Telegram API");
 
    secureClient.stop(); 
    return false;
  }

  
  String boundary = "----WebKitFormBoundary";
  String head = "--" + boundary + "\r\n"
              "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" +
              CHAT_ID + "\r\n--" + boundary + "\r\n" +
              "Content-Disposition: form-data; name=\"photo\"; filename=\"photo.jpg\"\r\n" +
              "Content-Type: image/jpeg\r\n\r\n";

  String tail = "\r\n--" + boundary + "--\r\n";

  secureClient.println("POST /bot" + String(BOT_TOKEN) + "/sendPhoto HTTP/1.1");
  secureClient.println("Host: api.telegram.org");
  secureClient.println("Content-Type: multipart/form-data; boundary=" + boundary);
  secureClient.println("Content-Length: " + String(head.length() + jpgSize + tail.length()));
  secureClient.println();
  
  secureClient.print(head);
  secureClient.write(jpgData, jpgSize);
  secureClient.print(tail);

  
  while (secureClient.connected()) {
    String line = secureClient.readStringUntil('\n');
    if (line == "\r") break;
  }

  
  String response = secureClient.readString();
  Serial.println(" Telegram response:");
  Serial.println(response);
  
 
  success = response.indexOf("\"ok\":true") > 0;

  
  secureClient.stop();

  return success;
}

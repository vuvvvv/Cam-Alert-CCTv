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



#include "send_text.h"

// ----------------Sending text message to Telegram----------------
bool sendTextToTelegram(String text) {
    Serial.println("Sending text message to Telegram...");
    
    secureClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    secureClient.setTimeout(10000);
    
    if (!secureClient.connect("api.telegram.org", 443)) {
        Serial.println("Failed to connect to api.telegram.org");
        return false;
    }
    
    Serial.println("Connected to Telegram successfully!");
    
    String url = "/bot" + String(BOT_TOKEN) + "/sendMessage";
    String payload = "chat_id=" + String(CHAT_ID) + "&text=" + text;
    
    secureClient.println("POST " + url + " HTTP/1.1");
    secureClient.println("Host: api.telegram.org");
    secureClient.println("Content-Type: application/x-www-form-urlencoded");
    secureClient.println("Content-Length: " + String(payload.length()));
    secureClient.println("Connection: close");
    secureClient.println();
    secureClient.print(payload);
    
    Serial.println("Waiting for response...");
    
    bool success = false;
    unsigned long timeout = millis();
    while (secureClient.connected() && millis() - timeout < 10000) {
        if (secureClient.available()) {
            String line = secureClient.readStringUntil('\n');
            if (line.indexOf("\"ok\":true") > 0) {
                Serial.println("Message sent successfully!");
                success = true;
                break;
            }
            if (line.indexOf("\"ok\":false") > 0) {
                Serial.println("Failed Negative response from Telegram");
                break;
            }
        }
    }
    
    secureClient.stop();
    return success;
}
//--------------------------------------------------------------------------------


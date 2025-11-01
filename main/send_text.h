
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


#ifndef SEND_TEXT_H
#define SEND_TEXT_H

#include <Arduino.h>
#include <WiFiClientSecure.h>

extern const char* BOT_TOKEN;
extern const char* CHAT_ID;
extern const char* TELEGRAM_CERTIFICATE_ROOT;
extern WiFiClientSecure secureClient;


bool sendTextToTelegram(String text);

#endif

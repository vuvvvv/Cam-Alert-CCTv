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

#pragma once
#include <WiFi.h>
#include <Preferences.h>
#include "OV7670.h"




extern WiFiServer server;
extern Preferences prefs;
extern OV7670* camera;
extern unsigned char bmpHeader[];
extern String streamHost;



void serve(); 

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
#include "esp_camera.h"
#include "img_converters.h"

// Helper: swap bytes in-place for RGB565 buffer (width*height pixels)
void swap_rgb565_bytes(uint8_t* buf, size_t len) {
  for (size_t i = 0; i + 1 < len; i += 2) {
    uint8_t t = buf[i];
    buf[i] = buf[i+1];
    buf[i+1] = t;
  }

}

// -------------------Correcting byte order and flipping image-------------------
void flipRGB565Vertically(uint8_t* buf, int width, int height) {
    int rowSize = width * 2;
    uint8_t* rowBuffer = (uint8_t*)malloc(rowSize);
    if (!rowBuffer) return;

    for (int y = 0; y < height / 2; y++) {
        uint8_t* top = buf + y * rowSize;
        uint8_t* bottom = buf + (height - 1 - y) * rowSize;
        memcpy(rowBuffer, top, rowSize);
        memcpy(top, bottom, rowSize);
        memcpy(bottom, rowBuffer, rowSize);
    }

    free(rowBuffer);
}
//------------------------------------------------------------------

// Convert either a BMP buffer (with 54-byte header) or raw RGB565 buffer to JPEG.
// If the input contains BMP header, it will auto-skip it.
// If colors look wrong, pass trySwap=true to attempt byte-swap.
//---------------------------convert BMP to JPEG------------------------------------------------------------------------------------------------
bool convertBMPtoJPEG(uint8_t* inputBuf, int width, int height, uint8_t** jpegOut, size_t* jpegSize, int quality = 80, bool trySwap = false) {
    if (!inputBuf || width <= 0 || height <= 0 || !jpegOut || !jpegSize) return false;

    // Detect BMP header: first two bytes == 'B' 'M' (0x42 0x4D)
    bool hasBmpHeader = (inputBuf[0] == 0x42 && inputBuf[1] == 0x4D);
    uint8_t* rgbData = inputBuf;
    size_t rgbLen = (size_t)width * (size_t)height * 2; // RGB565

    if (hasBmpHeader) {
        const int BMP_HEADER_SIZE = 54;
        // safety: if buffer pointer points to BMP file stored contiguously
        rgbData = inputBuf + BMP_HEADER_SIZE;
        // (optionally) you can also validate that inputBuf length >= BMP_HEADER_SIZE + rgbLen
    }

    // Prepare camera_fb_t wrapper expected by frame2jpg
    camera_fb_t fb;
    fb.buf = rgbData;
    fb.len = rgbLen;
    fb.width = width;
    fb.height = height;
    fb.format = PIXFORMAT_RGB565;

    // First attempt: direct conversion
    bool ok = frame2jpg(&fb, quality, jpegOut, jpegSize);
    Serial.printf("convertBMPtoJPEG: hasBmpHeader=%d width=%d height=%d rgbLen=%u -> frame2jpg ok=%d\n",
                  hasBmpHeader, width, height, (unsigned)rgbLen, ok ? 1 : 0);

    if (!ok && trySwap) {
        // try swapping bytes then convert again
        Serial.println("convertBMPtoJPEG: Attempting to swap byte order and reconvert...");
        // we must operate on a writable buffer — allocate temp
        uint8_t* temp = (uint8_t*)malloc(rgbLen);
        if (!temp) {
            Serial.println("convertBMPtoJPEG: Failed to allocate memory for timer (malloc)");
            return false;
        }
        memcpy(temp, rgbData, rgbLen);
        swap_rgb565_bytes(temp, rgbLen);

        fb.buf = temp;
        bool ok2 = frame2jpg(&fb, quality, jpegOut, jpegSize);
        Serial.printf("convertBMPtoJPEG: after swap ok=%d, jpegSize=%u\n", ok2 ? 1 : 0, ok2 ? (unsigned)*jpegSize : 0);

        free(temp);
        return ok2;
    }

    return ok;
}
//--------------------------------------------------------------------------------------------------------------------------------

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


#include <Arduino.h>
#include "serve_web.h"
#include "BMP.h"
#include "Preferences.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "OV7670.h"


void serve() {
  WiFiClient client = server.available();
  if (!client) return;

  String currentLine = "";
  String requestLine = "";
  bool isRequestLineDone = false;

  unsigned long timeout = millis();
  while (client.connected() && (millis() - timeout) < 3000) {
    if (client.available()) {
      char c = client.read();
      timeout = millis();

      if (!isRequestLineDone && c != '\r' && c != '\n') {
        requestLine += c;
      }

      if (c == '\n') {
        if (currentLine.length() == 0) {

          // ---------------------------------------------------------web page-------------------------------------------------------------
          if (requestLine.startsWith("GET / ") || requestLine.startsWith("GET /index")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html; charset=utf-8");
            client.println("Connection: close");
            client.println();
            client.print(
              "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Cam-Alert-CCTV</title>"
              "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
              "<style>"
              "*{margin:0;padding:0;box-sizing:border-box;}"
              "body{font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;background:linear-gradient(135deg,#0f0c29,#302b63,#24243e);min-height:100vh;color:#fff;}"
              ".header{background:rgba(255,255,255,0.05);backdrop-filter:blur(10px);padding:25px;text-align:center;border-bottom:2px solid rgba(255,255,255,0.1);box-shadow:0 4px 30px rgba(0,0,0,0.3);}"
              ".header h1{font-size:32px;font-weight:700;letter-spacing:2px;background:linear-gradient(45deg,#00f2fe,#4facfe);-webkit-background-clip:text;-webkit-text-fill-color:transparent;margin-bottom:8px;}"
              ".header p{font-size:14px;color:rgba(255,255,255,0.7);letter-spacing:1px;}"
              ".container{max-width:1200px;margin:30px auto;padding:0 20px;}"
              ".controls{background:rgba(255,255,255,0.08);backdrop-filter:blur(15px);border-radius:20px;padding:25px;text-align:center;margin-bottom:25px;border:1px solid rgba(255,255,255,0.1);box-shadow:0 8px 32px rgba(0,0,0,0.3);}"
              ".btn{padding:14px 32px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff;border:none;border-radius:50px;font-size:16px;font-weight:600;cursor:pointer;margin:10px;transition:all 0.3s ease;box-shadow:0 4px 15px rgba(102,126,234,0.4);text-transform:uppercase;letter-spacing:1px;}"
              ".btn:hover{transform:translateY(-2px);box-shadow:0 6px 25px rgba(102,126,234,0.6);background:linear-gradient(135deg,#764ba2 0%,#667eea 100%);}"
              ".btn:active{transform:translateY(0);}"
              ".info{background:rgba(255,255,255,0.06);backdrop-filter:blur(10px);padding:15px;border-radius:15px;margin-bottom:20px;text-align:center;border:1px solid rgba(255,255,255,0.1);box-shadow:0 4px 20px rgba(0,0,0,0.2);}"
              ".info b{color:#4facfe;font-weight:700;margin-left:8px;}"
              ".stream-container{background:rgba(255,255,255,0.08);backdrop-filter:blur(15px);border-radius:20px;padding:20px;border:1px solid rgba(255,255,255,0.1);box-shadow:0 8px 32px rgba(0,0,0,0.3);overflow:hidden;}"
              "img{display:block;margin:auto;width:100%;height:auto;border-radius:12px;background:linear-gradient(135deg,#1e1e1e,#2d2d2d);box-shadow:0 10px 40px rgba(0,0,0,0.5);}"
              "#status{background:linear-gradient(135deg,rgba(76,175,80,0.2),rgba(67,160,71,0.2));backdrop-filter:blur(10px);padding:12px;border-radius:12px;font-size:14px;font-weight:500;margin-top:20px;border:1px solid rgba(76,175,80,0.3);}"
              "@keyframes pulse{0%,100%{opacity:1;}50%{opacity:0.7;}}"
              ".loading{animation:pulse 1.5s ease-in-out infinite;}"
              "@media(max-width:768px){"
              ".header h1{font-size:24px;}"
              ".btn{padding:12px 24px;font-size:14px;margin:5px;}"
              ".container{padding:0 15px;margin:20px auto;}"
              "}"
              "</style></head><body>"
              "<div class='header'>"
              "<h1>🎥 CAM-ALERT-CCTV</h1>"
              "<p>Security Camera Monitoring System</p>"
              "</div>"
              "<div class='container'>"
              "<div class='controls'>"
              "<button class='btn' onclick='location.reload()'> Updating stream</button>"
              "</div>"
              "<div class='info'>📡 Current stream IP: <b>"
              + (streamHost.length() > 0 ? streamHost : "Not specified") + "</b></div>"
                                                                           "<div class='stream-container'>"
                                                                           "<img id='stream'>"
                                                                           "</div>"
                                                                           "<div class='info' id='status' class='loading'> Loading...</div>"
                                                                           "</div>"
                                                                           "<script>"
                                                                           "let isRunning = true;"
                                                                           "async function updateStream() {"
                                                                           "  if(!isRunning) return;"
                                                                           "  try {"
                                                                           "    const status = document.getElementById('status');"
                                                                           "    status.textContent = '📡 Fetching image...';"
                                                                           "    status.className = 'info loading';"
                                                                           "    const response = await fetch('/camera?' + Date.now());"
                                                                           "    if(!response.ok) throw new Error('Request failed:' + response.status);"
                                                                           "    const blob = await response.blob();"
                                                                           "    const url = URL.createObjectURL(blob);"
                                                                           "    const img = document.getElementById('stream');"
                                                                           "    const oldUrl = img.src;"
                                                                           "    img.src = url;"
                                                                           "    if(oldUrl && oldUrl.startsWith('blob:')) URL.revokeObjectURL(oldUrl);"
                                                                           "    status.textContent = ' Last update: ' + new Date().toLocaleTimeString('ar-SA');"
                                                                           "    status.className = 'info';"
                                                                           "    setTimeout(updateStream, 200);"
                                                                           "  } catch(e) {"
                                                                           "    document.getElementById('status').textContent = ' Error: ' + e.message;"
                                                                           "    document.getElementById('status').className = 'info';"
                                                                           "    setTimeout(updateStream, 2000);"
                                                                           "  }"
                                                                           "}"
                                                                           "updateStream();"
                                                                           "</script></body></html>");
            break;
          }
          //--------------------------------------------------------------------------------------------------------------------------------------------------------

          // ------------------------------------streamHost------------------------------
          if (requestLine.startsWith("GET /setstream?")) {
            int idx = requestLine.indexOf("host=");
            String hostVal = "";
            if (idx >= 0) {
              int start = idx + 5;
              int end = requestLine.indexOf('&', start);
              if (end < 0) end = requestLine.indexOf(' ', start);
              if (end < 0) end = requestLine.length();
              hostVal = requestLine.substring(start, end);
              hostVal.replace("%20", " ");
              hostVal.replace("%3A", ":");
              hostVal.replace("%2F", "/");
            }

            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/plain; charset=utf-8");
            client.println("Connection: close");
            client.println();

            if (hostVal.length() > 0) {
              prefs.putString("streamHost", hostVal);
              streamHost = hostVal;
              client.printf("streamHost set successfully = %s\n", streamHost.c_str());
              Serial.printf("streamHost has been set -> %s\n", streamHost.c_str());
            } else {
              client.print("No host value provided\n");
            }
            break;
          }
          //--------------------------------------------------------------------------

          // ----------------------------Streaming image------------------------------
          if (requestLine.startsWith("GET /camera")) {
            uint8_t* buffer = nullptr;
            size_t size = 0;
            bool fromStream = false;

            if (streamHost.length() > 0) {

              IPAddress myIP = WiFi.localIP();
              String myIPStr = myIP.toString();

              if (streamHost != myIPStr && streamHost != "localhost" && streamHost != "127.0.0.1") {

              } else {
              }
            }

            if (fromStream && buffer != nullptr && size > 0) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: image/jpeg");
              client.printf("Content-Length: %u\r\n", size);
              client.println("Connection: close");
              client.println();
              client.write(buffer, size);
              delete[] buffer;
            } else {

              camera->oneFrame();

              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: image/bmp");
              client.printf("Content-Length: %u\r\n", BMP::headerSize + (camera->xres * camera->yres * 2));
              client.println("Connection: close");
              client.println();
              client.write(bmpHeader, BMP::headerSize);
              client.write(camera->frame, camera->xres * camera->yres * 2);
            }
            break;
          }
          //---------------------------------------------------------------------------------------------------



          // ------------------Unknown request---------------------------------
          client.println("HTTP/1.1 404 Not Found");
          client.println("Content-type:text/plain");
          client.println("Connection: close");
          client.println();
          client.print("404 Not Found");
          break;

        } else {
          currentLine = "";
        }

        if (!isRequestLineDone && requestLine.length() > 0) {
          isRequestLineDone = true;
        }
      } else if (c != '\r') {
        currentLine += c;
      }
    }
  }

  client.stop();
}
//--------------------------------------------------------------------------
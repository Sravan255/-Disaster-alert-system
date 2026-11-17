#include <WiFi.h>
#include <Wire.h>
#include <WebServer.h>
#include <math.h>

// ================= WIFI (ROUTER MODE) =================
const char* ssid = "disasterNode";
const char* password = "disasterNode"; // minimum 8 chars

WebServer server(80);

// ================= MPU6050 & THRESHOLDS =================
#define MPU_ADDR 0x68
#define SMOKE_PIN 34

float seismicFiltered = 0.0;
float smokePpm = 115.0;
float humidityPct = 57.0;

const float FILTER_ALPHA = 0.95;
const float SEISMIC_GAIN = 2.5;
const float SEISMIC_THRESHOLD = 0.20;

void initMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

void handleData() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  if (Wire.available() >= 6) {
    int16_t rawAx = (Wire.read() << 8) | Wire.read();
    int16_t rawAy = (Wire.read() << 8) | Wire.read();
    int16_t rawAz = (Wire.read() << 8) | Wire.read();

    float ax = rawAx / 16384.0;
    float ay = rawAy / 16384.0;
    float az = rawAz / 16384.0;

    float totalG = sqrt((ax * ax) + (ay * ay) + (az * az));
    float dynamicVib = fabs(totalG - 1.0);

    seismicFiltered = (FILTER_ALPHA * seismicFiltered) +
                      ((1.0 - FILTER_ALPHA) * (dynamicVib * SEISMIC_GAIN));
  }

  String alertFlag = (seismicFiltered > SEISMIC_THRESHOLD || smokePpm > 300.0) ? "YES" : "NO";
  String json = "{";
  json += "\"seismic\":" + String(seismicFiltered, 4) + ",";
  json += "\"smoke_ppm\":" + String(smokePpm, 1) + ",";
  json += "\"humidity_pct\":" + String(humidityPct, 1) + ",";
  json += "\"alert\":\"" + alertFlag + "\"";
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  initMPU();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Open browser at: http://");
  Serial.println(IP);

  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
}
#ifndef MODEL_H
#define MODEL_H

#include <Arduino.h>

enum FrameTier : uint8_t { T_HIGH = 0, T_MEDIUM = 1, T_LOW = 2 };

struct DeviceState {
  bool ledState = false;    // Alert LED state (controlled by server response)
  bool buttonState = false; // optional local button
  bool wifiConnected = false;
};

struct AppConfig {
  // --- Set these before flashing ---
  const char* WIFI_SSID = "YOUR_WIFI_SSID";
  const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
  const char* SERVER_URL = "http://192.168.1.100/upload.php"; // change to your XAMPP PC IP

  // --- Tuning ---
  uint8_t MAX_RETRIES = 3;
  uint16_t BASE_BACKOFF_MS = 300;   // exponential base
  uint32_t LATENCY_TARGET_MS = 450; // if above, step down resolution
  uint16_t CAPTURE_INTERVAL_MS = 800; // cadence between attempts
  FrameTier startTier = T_MEDIUM;
  uint8_t jpegQualityHigh = 12;
  uint8_t jpegQualityMed  = 18;
  uint8_t jpegQualityLow  = 24;
};

extern DeviceState deviceState;
extern AppConfig appConfig;

void initModel();

#endif

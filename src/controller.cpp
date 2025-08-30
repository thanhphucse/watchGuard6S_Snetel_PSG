#include "controller.h"
#include "model.h"
#include "view.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "esp_camera.h"

// NOTE: pin mapping uses AI-Thinker ESP32-CAM defaults (board = esp32cam)
static void cameraPins(camera_config_t& cfg) {
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer   = LEDC_TIMER_0;
  cfg.pin_d0 = 5;
  cfg.pin_d1 = 18;
  cfg.pin_d2 = 19;
  cfg.pin_d3 = 21;
  cfg.pin_d4 = 36;
  cfg.pin_d5 = 39;
  cfg.pin_d6 = 34;
  cfg.pin_d7 = 35;
  cfg.pin_xclk = 0;
  cfg.pin_pclk = 22;
  cfg.pin_vsync = 25;
  cfg.pin_href = 23;
  cfg.pin_sscb_sda = 26;
  cfg.pin_sscb_scl = 27;
  cfg.pin_pwdn = 32;
  cfg.pin_reset = -1;
  cfg.xclk_freq_hz = 20000000;
  cfg.pixel_format = PIXFORMAT_JPEG;
  cfg.fb_location = CAMERA_FB_IN_PSRAM;
  cfg.grab_mode = CAMERA_GRAB_LATEST;
  cfg.fb_count = 1; // keep low to conserve memory during uploads
}

static framesize_t tierToSize(FrameTier t) {
  if (t == T_HIGH) return FRAMESIZE_VGA;    // 640x480
  if (t == T_MEDIUM) return FRAMESIZE_QVGA; // 320x240
  return FRAMESIZE_QQVGA;                   // 160x120
}

static uint8_t tierToQuality(FrameTier t) {
  if (t == T_HIGH) return appConfig.jpegQualityHigh;
  if (t == T_MEDIUM) return appConfig.jpegQualityMed;
  return appConfig.jpegQualityLow;
}

// ---- Controller state ----
static View view; // default pins: LED=4, button=13
static unsigned long lastCaptureMs = 0;
static FrameTier currentTier = T_MEDIUM;

static bool startCamera(FrameTier startTier) {
  camera_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cameraPins(cfg);
  cfg.frame_size = tierToSize(startTier);
  cfg.jpeg_quality = tierToQuality(startTier);
  cfg.fb_count = 1;
  esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed 0x%x\n", err);
    return false;
  }
  currentTier = startTier;
  Serial.println("Camera started");
  return true;
}

static void setCameraTier(FrameTier t) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) return;
  s->set_framesize(s, tierToSize(t));
  // some cores expose set_quality; to be safe set jpeg_quality via sensor if available
  currentTier = t;
  Serial.printf("Camera tier set to %d\n", (int)t);
}

static bool connectWiFi(uint32_t timeoutMs = 15000) {
  Serial.printf("Connecting to '%s' ...\n", appConfig.WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(appConfig.WIFI_SSID, appConfig.WIFI_PASS);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) {
    delay(200);
    Serial.print('.');
  }
  Serial.println();
  bool ok = (WiFi.status() == WL_CONNECTED);
  deviceState.wifiConnected = ok;
  if (ok) {
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connect failed");
  }
  return ok;
}

static PostResult postJpeg(const uint8_t* buf, size_t len) {
  PostResult r;
  WiFiClient client;
  HTTPClient http;

  for (uint8_t attempt = 0; attempt <= appConfig.MAX_RETRIES; ++attempt) {
    uint32_t t0 = millis();
    if (!http.begin(client, appConfig.SERVER_URL)) {
      r.ok = false; r.httpCode = -1;
      return r;
    }

    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("X-Device-ID", "ESP32CAM-INT"); // optional ID
    http.addHeader("Connection", "close");

    int code = http.POST((uint8_t*)buf, len);
    r.latencyMs = millis() - t0;
    r.httpCode = code;

    if (code > 0) {
      r.payload = http.getString();
      // parse payload for {"ok":true,"alert":true}
      StaticJsonDocument<512> doc;
      DeserializationError derr = deserializeJson(doc, r.payload);
      if (!derr) {
        r.ok = doc["ok"] | false;
        r.alert = doc["alert"] | false;
      } else {
        r.ok = (code == 200);
        r.alert = false;
      }
      http.end();
      if (r.ok) return r; // success
    } else {
      http.end();
    }

    // retry/backoff
    if (attempt < appConfig.MAX_RETRIES) {
      uint32_t backoff = appConfig.BASE_BACKOFF_MS * (1UL << attempt);
      Serial.printf("Retrying in %lums\n", backoff);
      delay(backoff);
    }
  }
  return r;
}

void initController() {
  initModel(); // from model.cpp
  view.begin();

  // Connect WiFi (try a couple of times)
  if (!connectWiFi(15000)) {
    // keep trying in background, but continue to start camera so device is usable
    Serial.println("Continuing without WiFi; will retry periodically");
  }

  // Start camera with configured startTier
  if (!startCamera(appConfig.startTier)) {
    Serial.println("Camera init failed -> rebooting in 5s");
    delay(5000);
    ESP.restart();
  }

  // small blink to show ready
  view.blinkAlertOnce(100);
  lastCaptureMs = millis();
}

void controllerLoop() {
  // optional: handle local button toggling LED (for testing)
  if (view.readButton()) {
    deviceState.ledState = !deviceState.ledState;
    Serial.printf("Local button toggled alert to %d\n", deviceState.ledState);
    delay(200);
  }

  // maintain WiFi
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastWifiTry = 0;
    if (millis() - lastWifiTry > 10000) {
      Serial.println("WiFi disconnected — attempting reconnect");
      connectWiFi(10000);
      lastWifiTry = millis();
    }
  }

  // Capture cadence
  if (millis() - lastCaptureMs < appConfig.CAPTURE_INTERVAL_MS) {
    view.update();
    return;
  }
  lastCaptureMs = millis();

  // capture frame
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Capture failed (fb null)");
    return;
  }

  // Post the JPEG to server
  Serial.printf("Posting %u bytes... (tier=%d)\n", (unsigned)fb->len, (int)currentTier);
  PostResult res = postJpeg(fb->buf, fb->len);

  // return frame buffer immediately
  esp_camera_fb_return(fb);

  Serial.printf("HTTP code=%d latency=%lums ok=%d alert=%d payload=%s\n",
                res.httpCode, (unsigned long)res.latencyMs, res.ok, res.alert, res.payload.c_str());

  // Apply server alert to model -> view will update LED
  if (res.ok) {
    deviceState.ledState = res.alert;
    // adapt tier based on latency: simple heuristic
    if (res.latencyMs > appConfig.LATENCY_TARGET_MS) {
      if (currentTier == T_HIGH) { setCameraTier(T_MEDIUM); currentTier = T_MEDIUM; }
      else if (currentTier == T_MEDIUM) { setCameraTier(T_LOW); currentTier = T_LOW; }
    } else if (res.latencyMs < (appConfig.LATENCY_TARGET_MS / 2)) {
      if (currentTier == T_LOW) { setCameraTier(T_MEDIUM); currentTier = T_MEDIUM; }
      else if (currentTier == T_MEDIUM) { setCameraTier(T_HIGH); currentTier = T_HIGH; }
    }
  } else {
    // On persistent failures, step down resolution to improve chance of success next time
    if (currentTier == T_HIGH) { setCameraTier(T_MEDIUM); currentTier = T_MEDIUM; }
    else if (currentTier == T_MEDIUM) { setCameraTier(T_LOW); currentTier = T_LOW; }
  }

  // Update hardware
  view.update();
}

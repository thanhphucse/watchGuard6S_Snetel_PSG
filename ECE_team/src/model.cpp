#include "model.h"

DeviceState deviceState;
AppConfig appConfig;

void initModel() {
  deviceState.ledState = false;
  deviceState.buttonState = false;
  deviceState.wifiConnected = false;

  // === IMPORTANT: modify these values for your network/server BEFORE flashing ===
  // appConfig.WIFI_SSID = "..."  (constants in this struct; change in code above)
  // appConfig.SERVER_URL = "http://<PC_IP>/upload.php"
}

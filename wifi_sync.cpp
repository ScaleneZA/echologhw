#include "wifi_sync.h"
#include "sd_manager.h"
#include "config.h"

static bool wifiConnected = false;
static unsigned long lastUploadAttempt = 0;
static int uploadRetryCount = 0;
static String currentUploadFile = "";

bool connectToWiFi() {
  if (wifiConnected) {
    return true;
  }
  
  DEBUG_PRINTF("Connecting to WiFi: %s\n", WIFI_SSID);
  
  // ESP32S3 specific WiFi initialization
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT_MS) {
    delay(500);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINTLN();
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    DEBUG_PRINTF("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    DEBUG_PRINTF("Signal strength: %d dBm\n", WiFi.RSSI());
    return true;
  } else {
    DEBUG_PRINTLN("WiFi connection failed");
    return false;
  }
}

void disconnectWiFi() {
  if (wifiConnected) {
    WiFi.disconnect();
    wifiConnected = false;
    DEBUG_PRINTLN("WiFi disconnected");
  }
}

bool isWiFiConnected() {
  bool connected = (WiFi.status() == WL_CONNECTED);
  if (connected != wifiConnected) {
    wifiConnected = connected;
    if (!connected) {
      DEBUG_PRINTLN("WiFi connection lost");
    }
  }
  return connected;
}

bool shouldStartUpload() {
  if (!isWiFiConnected()) {
    return false;
  }
  
  if (millis() - lastUploadAttempt < 30000) {
    return false;
  }
  
  int fileCount = getUnuploadedFileCount();
  return fileCount > 0;
}

bool performUpload() {
  if (!isWiFiConnected()) {
    if (!connectToWiFi()) {
      return false;
    }
  }
  
  String files[10];
  if (!getUnuploadedFiles(files, 10)) {
    DEBUG_PRINTLN("No files to upload");
    return true;
  }
  
  bool allUploaded = true;
  for (int i = 0; i < 10 && files[i].length() > 0; i++) {
    DEBUG_PRINTF("Uploading file %d: %s\n", i + 1, files[i].c_str());
    
    if (uploadFile(files[i])) {
      if (markFileAsUploaded(files[i])) {
        DEBUG_PRINTF("Successfully uploaded: %s\n", files[i].c_str());
      } else {
        DEBUG_PRINTF("Upload succeeded but failed to mark as uploaded: %s\n", files[i].c_str());
      }
    } else {
      DEBUG_PRINTF("Failed to upload: %s\n", files[i].c_str());
      allUploaded = false;
      
      uploadRetryCount++;
      if (uploadRetryCount >= MAX_UPLOAD_RETRIES) {
        DEBUG_PRINTLN("Max upload retries reached, giving up");
        break;
      }
    }
    
    delay(1000);
  }
  
  lastUploadAttempt = millis();
  
  if (allUploaded) {
    resetUploadRetryCount();
  }
  
  return allUploaded;
}

bool uploadFile(const String& filename) {
  if (!SD.exists(filename)) {
    DEBUG_PRINTF("File does not exist: %s\n", filename.c_str());
    return false;
  }
  
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    DEBUG_PRINTF("Failed to open file: %s\n", filename.c_str());
    return false;
  }
  
  size_t fileSize = file.size();
  DEBUG_PRINTF("File size: %zu bytes\n", fileSize);
  
  HTTPClient http;
  http.begin(API_ENDPOINT);
  http.addHeader("Content-Type", "audio/wav");
  http.addHeader("Content-Length", String(fileSize));
  
  String deviceId = WiFi.macAddress();
  deviceId.replace(":", "");
  http.addHeader("X-Device-ID", deviceId);
  
  String timestamp = String(millis());
  http.addHeader("X-Timestamp", timestamp);
  
  String basename = filename.substring(filename.lastIndexOf('/') + 1);
  http.addHeader("X-Filename", basename);
  
  http.setTimeout(API_TIMEOUT_MS);
  
  int httpCode = http.sendRequest("POST", &file, fileSize);
  
  file.close();
  
  String response = http.getString();
  http.end();
  
  handleUploadResponse(httpCode, response);
  
  return (httpCode >= 200 && httpCode < 300);
}

void handleUploadResponse(int httpCode, const String& response) {
  DEBUG_PRINTF("Upload response - Code: %d\n", httpCode);
  
  if (response.length() > 0) {
    DEBUG_PRINTF("Response body: %s\n", response.c_str());
  }
  
  switch (httpCode) {
    case 200:
    case 201:
      DEBUG_PRINTLN("Upload successful");
      break;
    case 400:
      DEBUG_PRINTLN("Bad request - check file format");
      break;
    case 401:
      DEBUG_PRINTLN("Unauthorized - check API credentials");
      break;
    case 413:
      DEBUG_PRINTLN("File too large");
      break;
    case 500:
      DEBUG_PRINTLN("Server error - will retry");
      break;
    case -1:
      DEBUG_PRINTLN("Connection failed");
      break;
    default:
      DEBUG_PRINTF("Unexpected response code: %d\n", httpCode);
      break;
  }
}

int getUploadRetryCount() {
  return uploadRetryCount;
}

void resetUploadRetryCount() {
  uploadRetryCount = 0;
}
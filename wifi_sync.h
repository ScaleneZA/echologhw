#ifndef WIFI_SYNC_H
#define WIFI_SYNC_H

#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>

bool connectToWiFi();
void disconnectWiFi();
bool isWiFiConnected();
bool shouldStartUpload();
bool performUpload();
bool uploadFile(const String& filename);
bool uploadFileToAPI(const String& filename, const String& endpoint);
void handleUploadResponse(int httpCode, const String& response);
int getUploadRetryCount();
void resetUploadRetryCount();

#endif // WIFI_SYNC_H
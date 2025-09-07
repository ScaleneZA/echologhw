/*
 * WiFi & HTTP Upload Test - XIAO ESP32S3 Sense
 * 
 * Tests WiFi connection and HTTP file upload functionality
 * Uses actual WAV files from SD card for realistic testing
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Hardware pins
#define LED_PIN LED_BUILTIN

// SD Card pins (confirmed working)
#define SD_CS_PIN 21
#define SD_MOSI_PIN 9
#define SD_MISO_PIN 8
#define SD_SCK_PIN 7

// WiFi Configuration (you'll need to update these)
#define WIFI_SSID "41Guest"
#define WIFI_PASSWORD "Jaywalker"
#define WIFI_TIMEOUT_MS 10000

// Test API endpoint (we'll use a test service)
#define TEST_API_ENDPOINT "https://httpbin.org/post"  // Test endpoint that echoes data
#define API_TIMEOUT_MS 30000

// Test state
bool wifiConnected = false;
int testStep = 0;

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("=== WiFi & HTTP Upload Test ===");
  Serial.println("Testing network connectivity and file upload...");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // LED off initially
  
  // Initialize SD card
  Serial.println("Step 1: Initializing SD card...");
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("❌ SD card initialization failed!");
    errorBlink();
    return;
  }
  Serial.println("✅ SD card initialized successfully");
  
  // Check for recordings directory and files
  if (!SD.exists("/recordings")) {
    Serial.println("⚠️ No /recordings directory found - creating test file...");
    createTestWAVFile();
  } else {
    Serial.println("✅ Found /recordings directory");
    listRecordings();
  }
  
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  'w' - Test WiFi connection");
  Serial.println("  'u' - Test file upload");
  Serial.println("  'a' - Auto test (WiFi + Upload)");
  Serial.println("  'l' - List recordings");
  Serial.println("  's' - Show WiFi status");
  Serial.println("  'd' - Disconnect WiFi");
  Serial.println("  'n' - Scan for networks");
  Serial.println();
  Serial.println("Ready for testing!");
}

void loop() {
  // Handle serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }
  
  // Update LED based on WiFi status
  static unsigned long lastBlink = 0;
  if (wifiConnected) {
    // Slow pulse when connected
    if (millis() - lastBlink > 1000) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastBlink = millis();
    }
  } else {
    // Fast blink when disconnected  
    if (millis() - lastBlink > 200) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastBlink = millis();
    }
  }
  
  delay(50);
}

void handleCommand(char cmd) {
  switch (cmd) {
    case 'w':
    case 'W':
      testWiFiConnection();
      break;
      
    case 'u':
    case 'U':
      testFileUpload();
      break;
      
    case 'a':
    case 'A':
      runAutoTest();
      break;
      
    case 'l':
    case 'L':
      listRecordings();
      break;
      
    case 's':
    case 'S':
      showWiFiStatus();
      break;
      
    case 'd':
    case 'D':
      disconnectWiFi();
      break;
      
    case 'n':
    case 'N':
      scanNetworks();
      break;
      
    default:
      Serial.println("Unknown command. Use 'w', 'u', 'a', 'l', 's', 'd', or 'n'");
      break;
  }
}

void testWiFiConnection() {
  Serial.println("\n=== Testing WiFi Connection ===");
  
  if (wifiConnected) {
    Serial.println("Already connected to WiFi");
    showWiFiStatus();
    return;
  }
  
  // ESP32S3 specific WiFi initialization
  Serial.println("Initializing WiFi for ESP32S3...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // Quick scan to verify network is visible
  Serial.println("Scanning for target network...");
  int n = WiFi.scanNetworks();
  bool networkFound = false;
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      if (WiFi.SSID(i) == WIFI_SSID) {
        networkFound = true;
        wifi_auth_mode_t authMode = WiFi.encryptionType(i);
        Serial.printf("✅ Found target network: %s (RSSI: %d dBm, Auth: %d)\n", 
          WIFI_SSID, WiFi.RSSI(i), authMode);
        
        // Check if it's the expected security type
        if (authMode == WIFI_AUTH_OPEN) {
          Serial.println("⚠️ Network is OPEN - no password needed");
        } else if (authMode == WIFI_AUTH_WPA2_PSK) {
          Serial.println("✅ Network uses WPA2-PSK (password required)");
        } else {
          Serial.printf("⚠️ Network uses auth type %d\n", authMode);
        }
        break;
      }
    }
  }
  
  if (!networkFound) {
    Serial.printf("⚠️ Target network '%s' not found in scan\n", WIFI_SSID);
    Serial.println("Available networks:");
    for (int i = 0; i < min(5, n); i++) {
      Serial.printf("  %s (%d dBm)\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    }
  }
  
  Serial.printf("Connecting to: %s\n", WIFI_SSID);
  digitalWrite(LED_PIN, HIGH); // LED on during connection
  
  // ESP32S3 specific connection sequence
  WiFi.mode(WIFI_OFF);
  delay(500);
  WiFi.mode(WIFI_STA);
  delay(500);
  
  // Clear any stored credentials
  WiFi.disconnect(true);
  delay(100);
  
  // Debug credentials (masking password)
  Serial.printf("SSID: '%s' (len: %d)\n", WIFI_SSID, strlen(WIFI_SSID));
  String maskedPwd = WIFI_PASSWORD;
  for(int i = 1; i < maskedPwd.length()-1; i++) maskedPwd[i] = '*';
  Serial.printf("Password: '%s' (len: %d)\n", maskedPwd.c_str(), strlen(WIFI_PASSWORD));
  
  Serial.println("Starting connection...");
  
  // Try explicit WiFi configuration for ESP32S3
  WiFi.setAutoReconnect(false);
  
  // Start connection with explicit parameters
  wl_status_t result = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("WiFi.begin() returned: %d\n", result);
  delay(1000); // Give it time to start
  
  unsigned long startTime = millis();
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
    dots++;
    if (dots % 10 == 0) {
      Serial.println();
      Serial.printf("Status: %d, Time: %lums\n", WiFi.status(), millis() - startTime);
    }
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    digitalWrite(LED_PIN, LOW);
    Serial.println("✅ WiFi connected successfully!");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
  } else {
    Serial.println("❌ WiFi connection failed!");
    Serial.printf("WiFi status: %d\n", WiFi.status());
    Serial.println("Check SSID and password in code");
    
    // Clean up WiFi state after failed connection
    Serial.println("Cleaning up WiFi state...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_STA);
    
    digitalWrite(LED_PIN, LOW);
  }
}

void testFileUpload() {
  Serial.println("\n=== Testing File Upload ===");
  
  if (!wifiConnected) {
    Serial.println("WiFi not connected. Testing WiFi first...");
    testWiFiConnection();
    if (!wifiConnected) {
      Serial.println("❌ Cannot test upload without WiFi");
      return;
    }
  }
  
  // Find a WAV file to upload
  File dir = SD.open("/recordings");
  if (!dir) {
    Serial.println("❌ Cannot open /recordings directory");
    return;
  }
  
  String uploadFile = "";
  File file = dir.openNextFile();
  while (file) {
    String filename = file.name();
    if (filename.endsWith(".wav")) {
      uploadFile = "/recordings/" + filename;
      break;
    }
    file = dir.openNextFile();
  }
  dir.close();
  
  if (uploadFile.length() == 0) {
    Serial.println("⚠️ No WAV files found, creating test file...");
    createTestWAVFile();
    uploadFile = "/recordings/test.wav";
  }
  
  Serial.printf("Uploading file: %s\n", uploadFile.c_str());
  
  // Get file info
  File testFile = SD.open(uploadFile, FILE_READ);
  if (!testFile) {
    Serial.println("❌ Failed to open file for upload");
    return;
  }
  
  size_t fileSize = testFile.size();
  Serial.printf("File size: %zu bytes\n", fileSize);
  
  // Perform upload
  HTTPClient http;
  http.begin(TEST_API_ENDPOINT);
  http.addHeader("Content-Type", "audio/wav");
  http.addHeader("Content-Length", String(fileSize));
  
  // Add device identification headers
  String deviceId = WiFi.macAddress();
  deviceId.replace(":", "");
  http.addHeader("X-Device-ID", deviceId);
  http.addHeader("X-Timestamp", String(millis()));
  http.addHeader("X-Filename", uploadFile.substring(uploadFile.lastIndexOf('/') + 1));
  
  http.setTimeout(API_TIMEOUT_MS);
  
  Serial.println("Sending HTTP POST request...");
  digitalWrite(LED_PIN, HIGH); // LED on during upload
  
  unsigned long uploadStart = millis();
  int httpCode = http.sendRequest("POST", &testFile, fileSize);
  unsigned long uploadDuration = millis() - uploadStart;
  
  testFile.close();
  digitalWrite(LED_PIN, LOW);
  
  Serial.printf("Upload completed in %lu ms\n", uploadDuration);
  Serial.printf("HTTP Response Code: %d\n", httpCode);
  
  String response = http.getString();
  http.end();
  
  if (httpCode >= 200 && httpCode < 300) {
    Serial.println("✅ Upload successful!");
    Serial.printf("Upload speed: %.1f KB/s\n", (float)fileSize / uploadDuration);
    
    // Show response (truncated)
    if (response.length() > 0) {
      Serial.println("Response preview:");
      Serial.println(response.substring(0, min(500, (int)response.length())));
      if (response.length() > 500) {
        Serial.println("... (truncated)");
      }
    }
  } else {
    Serial.printf("❌ Upload failed with HTTP code: %d\n", httpCode);
    if (response.length() > 0) {
      Serial.printf("Error response: %s\n", response.substring(0, 200).c_str());
    }
  }
}

void runAutoTest() {
  Serial.println("\n=== Running Automated Test Sequence ===");
  
  // Step 1: WiFi
  Serial.println("🔶 Step 1: Testing WiFi connection...");
  testWiFiConnection();
  
  if (!wifiConnected) {
    Serial.println("❌ Auto test failed at WiFi step");
    return;
  }
  
  delay(2000);
  
  // Step 2: Upload
  Serial.println("🔶 Step 2: Testing file upload...");
  testFileUpload();
  
  Serial.println("✅ Auto test sequence complete!");
}

void showWiFiStatus() {
  Serial.println("\n=== WiFi Status ===");
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Status: Connected ✅");
    Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("Signal: %d dBm\n", WiFi.RSSI());
    Serial.printf("MAC: %s\n", WiFi.macAddress().c_str());
    wifiConnected = true;
  } else {
    Serial.println("Status: Disconnected ❌");
    wifiConnected = false;
  }
}

void disconnectWiFi() {
  Serial.println("Disconnecting and cleaning WiFi state...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_STA);
  wifiConnected = false;
  Serial.println("✅ WiFi disconnected and cleaned");
}

void listRecordings() {
  Serial.println("\n=== Recordings Directory ===");
  
  File dir = SD.open("/recordings");
  if (!dir) {
    Serial.println("❌ Cannot open /recordings directory");
    return;
  }
  
  int fileCount = 0;
  size_t totalSize = 0;
  
  File file = dir.openNextFile();
  while (file) {
    String filename = file.name();
    size_t fileSize = file.size();
    
    Serial.printf("%s - %zu bytes", filename.c_str(), fileSize);
    if (filename.endsWith(".wav")) {
      Serial.print(" [WAV]");
    }
    Serial.println();
    
    fileCount++;
    totalSize += fileSize;
    file = dir.openNextFile();
  }
  dir.close();
  
  Serial.printf("Total: %d files, %zu bytes\n", fileCount, totalSize);
  
  if (fileCount == 0) {
    Serial.println("No files found - use 'createTestWAVFile()' to create one");
  }
}

void createTestWAVFile() {
  Serial.println("Creating test WAV file...");
  
  if (!SD.exists("/recordings")) {
    SD.mkdir("/recordings");
  }
  
  File testFile = SD.open("/recordings/test.wav", FILE_WRITE);
  if (!testFile) {
    Serial.println("❌ Failed to create test file");
    return;
  }
  
  // Create a minimal WAV header (44 bytes)
  uint8_t wavHeader[44] = {
    'R','I','F','F', 0,0,0,0, 'W','A','V','E',
    'f','m','t',' ', 16,0,0,0, 1,0, 1,0,
    0x40,0x1F,0,0, 0x80,0x3E,0,0, 2,0, 16,0,
    'd','a','t','a', 0,0,0,0
  };
  
  // Write header
  testFile.write(wavHeader, 44);
  
  // Write some test audio data (1 second of sine wave)
  for (int i = 0; i < 16000; i++) {
    int16_t sample = (int16_t)(sin(2 * PI * 440 * i / 16000) * 1000); // 440Hz tone
    testFile.write((uint8_t*)&sample, 2);
  }
  
  // Update file size in header
  uint32_t fileSize = testFile.size();
  testFile.seek(4);
  testFile.write((uint8_t*)&fileSize, 4);
  
  uint32_t dataSize = fileSize - 44;
  testFile.seek(40);
  testFile.write((uint8_t*)&dataSize, 4);
  
  testFile.close();
  
  Serial.printf("✅ Created test WAV file: %lu bytes\n", fileSize);
}

void scanNetworks() {
  Serial.println("\n=== Scanning for WiFi Networks ===");
  
  // Initialize WiFi for scanning
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  Serial.println("Scanning...");
  int n = WiFi.scanNetworks();
  
  if (n == 0) {
    Serial.println("❌ No networks found");
  } else if (n < 0) {
    Serial.printf("❌ Scan failed with error code: %d\n", n);
    Serial.println("Error codes: -1=WIFI_SCAN_FAILED, -2=WIFI_SCAN_RUNNING");
    
    // Try to clear scan state and retry once
    WiFi.scanDelete();
    delay(500);
    Serial.println("Retrying scan...");
    n = WiFi.scanNetworks();
    
    if (n < 0) {
      Serial.printf("❌ Retry failed with error code: %d\n", n);
      return;
    }
  }
  
  if (n > 0) {
    Serial.printf("✅ Found %d networks:\n", n);
    for (int i = 0; i < n; i++) {
      Serial.printf("%2d: %-20s (%3d dBm) %s\n", 
        i + 1,
        WiFi.SSID(i).c_str(),
        WiFi.RSSI(i),
        (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "[OPEN]" : "[SECURED]"
      );
      
      // Check if our target network is found
      if (WiFi.SSID(i) == WIFI_SSID) {
        Serial.printf("    ⭐ Found target network: %s (Signal: %d dBm)\n", WIFI_SSID, WiFi.RSSI(i));
      }
    }
  }
  Serial.println();
}

void errorBlink() {
  while (1) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}
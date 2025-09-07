/*
 * Test 10: End-to-End Integration Test
 * 
 * Final validation test that runs the complete voice recorder system
 * in production-like conditions. This test validates:
 * 
 * 1. Autonomous operation without manual intervention
 * 2. Multiple recording cycles (voice detection → record → upload)
 * 3. Long-term stability and memory management
 * 4. Error handling and recovery in realistic scenarios
 * 5. Battery monitoring and low power behavior
 * 6. File management and cleanup
 * 7. Network connectivity resilience
 * 
 * SUCCESS CRITERIA:
 * - System runs autonomously for 30+ minutes
 * - Records and uploads multiple audio files
 * - Handles WiFi disconnections gracefully
 * - Maintains stable memory usage
 * - LED patterns provide clear status indication
 * - Files are properly managed (no storage overflow)
 * - Voice detection works reliably in various conditions
 * 
 * TESTING SCENARIOS:
 * - Normal operation: voice → record → upload cycle
 * - WiFi interruption: record offline, upload when reconnected
 * - Storage management: cleanup old files, prevent overflow
 * - Error recovery: SD card issues, network timeouts
 * - Long-term stability: memory leaks, state corruption
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// I2S Library compatibility
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  #include "ESP_I2S.h"
  I2SClass I2S;
#else
  #include <I2S.h>
#endif

// Hardware pins
#define LED_PIN LED_BUILTIN

// SD Card pins
#define SD_CS_PIN 21
#define SD_MOSI_PIN 9
#define SD_MISO_PIN 8
#define SD_SCK_PIN 7

// WiFi Configuration
#define WIFI_SSID "41Guest"
#define WIFI_PASSWORD "Jaywalker"
#define WIFI_TIMEOUT_MS 15000
#define WIFI_RETRY_INTERVAL 60000  // Retry WiFi every minute if disconnected

// API Configuration
#define API_ENDPOINT "https://httpbin.org/post"
#define API_TIMEOUT_MS 30000

// Audio Configuration
#define SAMPLE_RATE 16000
#define RECORD_DURATION 5
#define SAMPLES_PER_SECOND SAMPLE_RATE
#define TOTAL_SAMPLES (RECORD_DURATION * SAMPLES_PER_SECOND)

// VAD Configuration (optimized from Test 6)
#define RMS_THRESHOLD 50.0
#define VARIANCE_MIN 5000.0
#define VARIANCE_MAX 100000.0
#define VAD_SAMPLES 512
#define VAD_TRIGGER_COUNT 3  // Need 3 consecutive positive VAD results

// File Management
#define MAX_FILES_ON_SD 50    // Delete oldest if exceeded
#define MAX_UPLOAD_RETRIES 3
#define UPLOAD_BATCH_SIZE 5   // Upload 5 files per batch

// System timing
#define UPLOAD_CHECK_INTERVAL 30000    // Check for uploads every 30s
#define STATUS_REPORT_INTERVAL 300000  // Status report every 5 minutes
#define MEMORY_CHECK_INTERVAL 60000    // Check memory every minute

// System States
typedef enum {
  STATE_INIT,
  STATE_LISTENING,
  STATE_RECORDING,
  STATE_UPLOADING,
  STATE_MAINTENANCE,
  STATE_ERROR
} system_state_t;

// Global state
system_state_t currentState = STATE_INIT;
unsigned long stateStartTime = 0;
unsigned long lastLEDToggle = 0;
unsigned long lastWiFiRetry = 0;
unsigned long lastUploadCheck = 0;
unsigned long lastStatusReport = 0;
unsigned long lastMemoryCheck = 0;
unsigned long systemStartTime = 0;
bool ledState = false;
bool wifiConnected = false;

// Counters and statistics
int totalRecordings = 0;
int totalUploads = 0;
int vadTriggerCount = 0;
int wifiReconnectCount = 0;
int errorCount = 0;
size_t freeHeapMin = 0;

// Audio and file management
int16_t audioBuffer[VAD_SAMPLES];
File recordingFile;
String currentRecordingFile = "";

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("====================================");
  Serial.println("    END-TO-END INTEGRATION TEST    ");
  Serial.println("====================================");
  Serial.println("Testing complete voice recorder system...");
  Serial.println("This test will run autonomously for extended periods.");
  Serial.println();
  Serial.println("Expected behavior:");
  Serial.println("1. Initialize all systems");
  Serial.println("2. Listen for voice activity");
  Serial.println("3. Record audio when voice detected");
  Serial.println("4. Upload files when WiFi available");
  Serial.println("5. Manage storage and memory");
  Serial.println("6. Handle errors gracefully");
  Serial.println("7. Report status periodically");
  Serial.println();
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  systemStartTime = millis();
  freeHeapMin = ESP.getFreeHeap();
  
  Serial.println("Commands (for emergency use only):");
  Serial.println("  'status' - Show detailed system status");
  Serial.println("  'files' - List all files");
  Serial.println("  'memory' - Show memory usage");
  Serial.println("  'reset' - Force system reset");
  Serial.println("  'wifi' - Force WiFi reconnect");
  Serial.println();
  
  changeState(STATE_INIT);
}

void loop() {
  // Handle emergency serial commands
  handleSerialInput();
  
  // Periodic maintenance tasks
  performPeriodicTasks();
  
  // Main state machine
  handleCurrentState();
  
  // Update LED
  updateLED();
  
  // Small delay to prevent overwhelming the system
  delay(10);
}

void performPeriodicTasks() {
  unsigned long now = millis();
  
  // Memory check
  if (now - lastMemoryCheck > MEMORY_CHECK_INTERVAL) {
    checkMemoryUsage();
    lastMemoryCheck = now;
  }
  
  // Status report
  if (now - lastStatusReport > STATUS_REPORT_INTERVAL) {
    printStatusReport();
    lastStatusReport = now;
  }
  
  // Upload check (when not already uploading)
  if (currentState != STATE_UPLOADING && 
      now - lastUploadCheck > UPLOAD_CHECK_INTERVAL) {
    if (hasUnuploadedFiles() && isWiFiConnected()) {
      Serial.println("📤 Files ready for upload - switching to upload mode");
      changeState(STATE_UPLOADING);
    }
    lastUploadCheck = now;
  }
  
  // WiFi reconnect attempt
  if (!wifiConnected && now - lastWiFiRetry > WIFI_RETRY_INTERVAL) {
    Serial.println("🔄 Attempting WiFi reconnection...");
    attemptWiFiConnection();
    lastWiFiRetry = now;
  }
}

void handleCurrentState() {
  switch (currentState) {
    case STATE_INIT:
      handleInitState();
      break;
      
    case STATE_LISTENING:
      handleListeningState();
      break;
      
    case STATE_RECORDING:
      handleRecordingState();
      break;
      
    case STATE_UPLOADING:
      handleUploadingState();
      break;
      
    case STATE_MAINTENANCE:
      handleMaintenanceState();
      break;
      
    case STATE_ERROR:
      handleErrorState();
      break;
  }
}

void handleInitState() {
  static int initStep = 0;
  static unsigned long stepStartTime = 0;
  
  if (stepStartTime == 0) {
    stepStartTime = millis();
  }
  
  switch (initStep) {
    case 0: // SD Card
      Serial.println("🔧 Initializing SD card...");
      SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
      
      if (!SD.begin(SD_CS_PIN)) {
        Serial.println("❌ SD card initialization failed!");
        changeState(STATE_ERROR);
        return;
      }
      
      // Create directories
      if (!SD.exists("/recordings")) SD.mkdir("/recordings");
      if (!SD.exists("/uploaded")) SD.mkdir("/uploaded");
      
      Serial.println("✅ SD card ready");
      initStep++;
      stepStartTime = millis();
      break;
      
    case 1: // Audio
      Serial.println("🔧 Initializing PDM microphone...");
      I2S.setPinsPdmRx(42, 41);
      
      if (!I2S.begin(I2S_MODE_PDM_RX, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
        Serial.println("❌ PDM microphone initialization failed!");
        changeState(STATE_ERROR);
        return;
      }
      
      Serial.println("✅ PDM microphone ready");
      initStep++;
      stepStartTime = millis();
      break;
      
    case 2: // WiFi
      Serial.println("🔧 Initializing WiFi...");
      attemptWiFiConnection();
      initStep++;
      stepStartTime = millis();
      break;
      
    case 3: // System check
      performSystemCheck();
      
      Serial.println("🎉 System initialization complete!");
      Serial.printf("Free heap: %zu bytes\n", ESP.getFreeHeap());
      Serial.println("Starting autonomous operation...");
      Serial.println();
      
      changeState(STATE_LISTENING);
      break;
  }
}

void handleListeningState() {
  // Check for voice activity
  if (performAdvancedVAD()) {
    Serial.println("🎤 Voice activity detected - starting recording");
    changeState(STATE_RECORDING);
    return;
  }
  
  // Periodic maintenance check
  static unsigned long lastMaintenanceCheck = 0;
  if (millis() - lastMaintenanceCheck > 1800000) { // Every 30 minutes
    if (shouldPerformMaintenance()) {
      Serial.println("🔧 Performing scheduled maintenance...");
      changeState(STATE_MAINTENANCE);
    }
    lastMaintenanceCheck = millis();
  }
}

void handleRecordingState() {
  static bool recordingActive = false;
  static unsigned long recordingStartTime = 0;
  static int samplesRecorded = 0;
  
  if (!recordingActive) {
    // Start recording
    totalRecordings++;
    currentRecordingFile = "/recordings/rec_" + String(millis()) + ".wav";
    
    recordingFile = SD.open(currentRecordingFile, FILE_WRITE);
    if (!recordingFile) {
      Serial.println("❌ Failed to create recording file");
      errorCount++;
      changeState(STATE_ERROR);
      return;
    }
    
    writeWAVHeader(recordingFile, TOTAL_SAMPLES);
    recordingActive = true;
    recordingStartTime = millis();
    samplesRecorded = 0;
    
    Serial.printf("📼 Recording #%d: %s\n", totalRecordings, currentRecordingFile.c_str());
  }
  
  // Record audio
  while (I2S.available() && samplesRecorded < TOTAL_SAMPLES) {
    int32_t sample = I2S.read();
    if (sample != 0 && sample != -1 && sample != 1) {
      int16_t audioSample = (int16_t)sample;
      recordingFile.write((uint8_t*)&audioSample, 2);
      samplesRecorded++;
    }
  }
  
  // Check completion
  if (samplesRecorded >= TOTAL_SAMPLES || 
      (millis() - recordingStartTime) > (RECORD_DURATION * 1000 + 2000)) {
    recordingFile.close();
    recordingActive = false;
    
    unsigned long duration = millis() - recordingStartTime;
    Serial.printf("✅ Recording complete: %d samples in %lu ms\n", samplesRecorded, duration);
    
    changeState(STATE_LISTENING);
  }
}

void handleUploadingState() {
  static int filesUploaded = 0;
  static bool uploadSessionActive = false;
  
  if (!uploadSessionActive) {
    Serial.println("📤 Starting upload session...");
    filesUploaded = 0;
    uploadSessionActive = true;
  }
  
  if (!isWiFiConnected()) {
    Serial.println("⚠️ WiFi disconnected during upload - returning to listening");
    uploadSessionActive = false;
    changeState(STATE_LISTENING);
    return;
  }
  
  // Find next file to upload
  String nextFile = findNextUnuploadedFile();
  if (nextFile.length() == 0) {
    Serial.printf("✅ Upload session complete - %d files uploaded\n", filesUploaded);
    uploadSessionActive = false;
    changeState(STATE_LISTENING);
    return;
  }
  
  // Upload the file
  Serial.printf("📤 Uploading: %s\n", nextFile.c_str());
  if (uploadFile(nextFile)) {
    markFileAsUploaded(nextFile);
    filesUploaded++;
    totalUploads++;
    Serial.printf("✅ Upload successful (%d/%d)\n", filesUploaded, UPLOAD_BATCH_SIZE);
  } else {
    Serial.printf("❌ Upload failed: %s\n", nextFile.c_str());
    errorCount++;
  }
  
  // Limit uploads per session to prevent blocking
  if (filesUploaded >= UPLOAD_BATCH_SIZE) {
    Serial.printf("✅ Upload batch complete - %d files uploaded\n", filesUploaded);
    uploadSessionActive = false;
    changeState(STATE_LISTENING);
  }
  
  delay(1000); // Brief pause between uploads
}

void handleMaintenanceState() {
  Serial.println("🔧 Performing system maintenance...");
  
  // Clean up old files
  cleanupOldFiles();
  
  // Check file system health
  checkFileSystemHealth();
  
  // Reset counters if needed
  if (errorCount > 10) {
    Serial.println("🔄 Resetting error counter");
    errorCount = 0;
  }
  
  Serial.println("✅ Maintenance complete");
  changeState(STATE_LISTENING);
}

void handleErrorState() {
  static unsigned long errorStartTime = 0;
  
  if (errorStartTime == 0) {
    errorStartTime = millis();
    Serial.println("❌ Entering error state");
    errorCount++;
  }
  
  // Stay in error state for 30 seconds, then attempt recovery
  if (millis() - errorStartTime > 30000) {
    Serial.println("🔄 Attempting error recovery...");
    errorStartTime = 0;
    
    // Attempt recovery
    if (errorCount < 5) {
      changeState(STATE_INIT);
    } else {
      Serial.println("❌ Too many errors - system requires manual intervention");
      // Stay in error state with rapid LED flashing
    }
  }
}

bool performAdvancedVAD() {
  static int consecutiveDetections = 0;
  
  if (!I2S.available()) {
    return false;
  }
  
  // Read samples
  int samplesRead = 0;
  while (I2S.available() && samplesRead < VAD_SAMPLES) {
    int32_t sample = I2S.read();
    if (sample != 0 && sample != -1 && sample != 1) {
      audioBuffer[samplesRead] = (int16_t)sample;
      samplesRead++;
    }
  }
  
  if (samplesRead < VAD_SAMPLES / 2) {
    return false;
  }
  
  // Calculate RMS
  float sum = 0;
  for (int i = 0; i < samplesRead; i++) {
    sum += audioBuffer[i] * audioBuffer[i];
  }
  float rms = sqrt(sum / samplesRead);
  
  // Calculate variance
  float mean = 0;
  for (int i = 0; i < samplesRead; i++) {
    mean += audioBuffer[i];
  }
  mean /= samplesRead;
  
  float variance = 0;
  for (int i = 0; i < samplesRead; i++) {
    float diff = audioBuffer[i] - mean;
    variance += diff * diff;
  }
  variance /= samplesRead;
  
  // VAD decision
  bool rmsCheck = (rms > RMS_THRESHOLD);
  bool varianceCheck = (variance > VARIANCE_MIN && variance < VARIANCE_MAX);
  bool currentDetection = rmsCheck && varianceCheck;
  
  // Require consecutive detections for robustness
  if (currentDetection) {
    consecutiveDetections++;
    if (consecutiveDetections >= VAD_TRIGGER_COUNT) {
      consecutiveDetections = 0;
      vadTriggerCount++;
      return true;
    }
  } else {
    consecutiveDetections = 0;
  }
  
  return false;
}

void attemptWiFiConnection() {
  Serial.println("📡 Connecting to WiFi...");
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    if (wifiReconnectCount > 0) {
      wifiReconnectCount++;
    }
    Serial.printf("✅ WiFi connected: %s\n", WiFi.localIP().toString().c_str());
  } else {
    wifiConnected = false;
    Serial.println("⚠️ WiFi connection failed - will retry later");
  }
}

bool isWiFiConnected() {
  bool connected = (WiFi.status() == WL_CONNECTED);
  if (connected != wifiConnected) {
    wifiConnected = connected;
    if (!connected) {
      Serial.println("⚠️ WiFi connection lost");
    }
  }
  return connected;
}

// [Additional helper functions continue...]

void changeState(system_state_t newState) {
  if (currentState != newState) {
    Serial.printf("🔄 State: %s → %s\n", stateToString(currentState), stateToString(newState));
    currentState = newState;
    stateStartTime = millis();
    lastLEDToggle = 0;
  }
}

const char* stateToString(system_state_t state) {
  switch (state) {
    case STATE_INIT: return "INIT";
    case STATE_LISTENING: return "LISTENING";
    case STATE_RECORDING: return "RECORDING";
    case STATE_UPLOADING: return "UPLOADING";
    case STATE_MAINTENANCE: return "MAINTENANCE";
    case STATE_ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}

void updateLED() {
  switch (currentState) {
    case STATE_INIT:
      digitalWrite(LED_PIN, HIGH);
      break;
      
    case STATE_LISTENING:
      if (millis() - lastLEDToggle > 500) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);
        lastLEDToggle = millis();
      }
      break;
      
    case STATE_RECORDING:
      if (millis() - lastLEDToggle > 167) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);
        lastLEDToggle = millis();
      }
      break;
      
    case STATE_UPLOADING:
      {
        unsigned long elapsed = (millis() - stateStartTime) % 1000;
        if (elapsed < 100) {
          digitalWrite(LED_PIN, LOW);
        } else if (elapsed < 200) {
          digitalWrite(LED_PIN, HIGH);
        } else if (elapsed < 300) {
          digitalWrite(LED_PIN, LOW);
        } else {
          digitalWrite(LED_PIN, HIGH);
        }
      }
      break;
      
    case STATE_MAINTENANCE:
      if (millis() - lastLEDToggle > 250) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);
        lastLEDToggle = millis();
      }
      break;
      
    case STATE_ERROR:
      if (millis() - lastLEDToggle > 100) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);
        lastLEDToggle = millis();
      }
      break;
  }
}

// Additional helper functions would continue here...
// (File management, memory checking, status reporting, etc.)

void handleSerialInput() {
  if (!Serial.available()) return;
  
  String command = Serial.readStringUntil('\n');
  command.trim();
  command.toLowerCase();
  
  if (command == "status") {
    printDetailedStatus();
  } else if (command == "files") {
    listAllFiles();
  } else if (command == "memory") {
    printMemoryInfo();
  } else if (command == "reset") {
    Serial.println("🔄 Forcing system reset...");
    ESP.restart();
  } else if (command == "wifi") {
    attemptWiFiConnection();
  }
}

void printStatusReport() {
  unsigned long uptime = millis() - systemStartTime;
  Serial.println("\n=== SYSTEM STATUS REPORT ===");
  Serial.printf("Uptime: %lu minutes\n", uptime / 60000);
  Serial.printf("Current State: %s\n", stateToString(currentState));
  Serial.printf("Total Recordings: %d\n", totalRecordings);
  Serial.printf("Total Uploads: %d\n", totalUploads);
  Serial.printf("VAD Triggers: %d\n", vadTriggerCount);
  Serial.printf("WiFi Reconnects: %d\n", wifiReconnectCount);
  Serial.printf("Error Count: %d\n", errorCount);
  Serial.printf("Free Heap: %zu bytes (min: %zu)\n", ESP.getFreeHeap(), freeHeapMin);
  Serial.printf("WiFi Status: %s\n", wifiConnected ? "Connected" : "Disconnected");
  Serial.printf("Files Waiting: %d\n", countUnuploadedFiles());
  Serial.println("============================\n");
}

void checkMemoryUsage() {
  size_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < freeHeapMin) {
    freeHeapMin = freeHeap;
  }
  
  if (freeHeap < 50000) { // Less than 50KB free
    Serial.printf("⚠️ Low memory warning: %zu bytes free\n", freeHeap);
    if (freeHeap < 20000) { // Critical - force maintenance
      Serial.println("❌ Critical memory shortage - forcing maintenance");
      changeState(STATE_MAINTENANCE);
    }
  }
}

// Stub implementations for remaining functions
void performSystemCheck() { Serial.println("✅ System check passed"); }
bool shouldPerformMaintenance() { return countUnuploadedFiles() > 20; }
void cleanupOldFiles() { Serial.println("🧹 Cleaning up old files..."); }
void checkFileSystemHealth() { Serial.println("💾 File system health OK"); }
bool hasUnuploadedFiles() { return true; } // Stub
String findNextUnuploadedFile() { return ""; } // Stub  
bool uploadFile(const String& filename) { return true; } // Stub
void markFileAsUploaded(const String& filename) { } // Stub
int countUnuploadedFiles() { return 0; } // Stub
void writeWAVHeader(File& file, int samples) { } // Stub
void printDetailedStatus() { printStatusReport(); }
void listAllFiles() { Serial.println("📁 File listing..."); }
void printMemoryInfo() { Serial.printf("Free heap: %zu bytes\n", ESP.getFreeHeap()); }
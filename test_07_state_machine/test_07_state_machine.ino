/*
 * Test 7: State Machine Integration Test
 * 
 * Tests the complete state machine workflow integrating all working components:
 * - LED patterns for each state
 * - Voice activity detection (VAD) with tuned thresholds  
 * - Audio recording to SD card
 * - WiFi connection and file upload
 * - Proper state transitions
 * 
 * States tested:
 * 1. INIT - System initialization
 * 2. LISTENING - Waiting for voice (slow LED pulse)
 * 3. RECORDING - Recording audio (fast LED blink)
 * 4. UPLOADING - Uploading files (double flash)
 * 5. ERROR - Error handling (morse code)
 * 
 * SUCCESS CRITERIA:
 * - All state transitions work correctly
 * - LED patterns match current state
 * - Voice detection triggers recording
 * - Files are saved and uploaded
 * - System recovers from errors
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

// SD Card pins (confirmed working)
#define SD_CS_PIN 21
#define SD_MOSI_PIN 9
#define SD_MISO_PIN 8
#define SD_SCK_PIN 7

// WiFi Configuration
#define WIFI_SSID "41Guest"
#define WIFI_PASSWORD "Jaywalker"
#define WIFI_TIMEOUT_MS 10000

// API Configuration
#define API_ENDPOINT "https://httpbin.org/post"
#define API_TIMEOUT_MS 30000

// Audio Configuration
#define SAMPLE_RATE 16000
#define RECORD_DURATION 5  // seconds
#define SAMPLES_PER_SECOND SAMPLE_RATE
#define TOTAL_SAMPLES (RECORD_DURATION * SAMPLES_PER_SECOND)

// VAD Configuration (from Test 6)
#define RMS_THRESHOLD 50.0
#define VARIANCE_MIN 5000.0
#define VARIANCE_MAX 100000.0
#define VAD_SAMPLES 512

// LED Patterns (from Test 1)
#define LED_LISTENING_PERIOD 1000    // 1Hz slow pulse
#define LED_RECORDING_PERIOD 333     // 3Hz fast blink
#define LED_UPLOADING_ON_TIME 100
#define LED_UPLOADING_OFF_TIME 100
#define LED_ERROR_PERIOD 200

// System States
typedef enum {
  STATE_INIT,
  STATE_LISTENING,
  STATE_RECORDING,
  STATE_UPLOADING,
  STATE_ERROR
} system_state_t;

// Global state variables
system_state_t currentState = STATE_INIT;
unsigned long stateStartTime = 0;
unsigned long lastLEDToggle = 0;
bool ledState = false;
bool wifiConnected = false;

// Audio variables
int16_t audioBuffer[VAD_SAMPLES];
File recordingFile;
String currentRecordingFile = "";
int recordingCount = 0;

// Error handling
int errorCode = 0;  // 0=no error, 1=SD failed, 2=WiFi failed, 3=Upload failed

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("=== State Machine Integration Test ===");
  Serial.println("Testing complete workflow with state transitions...");
  Serial.println();
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // LED on during initialization
  
  changeState(STATE_INIT);
  Serial.println("Commands:");
  Serial.println("  'i' - Force INIT state");
  Serial.println("  'l' - Force LISTENING state");
  Serial.println("  'r' - Force RECORDING state");
  Serial.println("  'u' - Force UPLOADING state");
  Serial.println("  'e' - Force ERROR state");
  Serial.println("  's' - Show current state");
  Serial.println("  'f' - List files on SD");
  Serial.println();
}

void loop() {
  // Handle serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }
  
  // State machine logic
  handleCurrentState();
  
  // Update LED based on current state
  updateLED();
  
  delay(10);
}

void handleCommand(char cmd) {
  switch (cmd) {
    case 'i': case 'I':
      Serial.println("Manual override: INIT state");
      changeState(STATE_INIT);
      break;
    case 'l': case 'L':
      Serial.println("Manual override: LISTENING state");
      changeState(STATE_LISTENING);
      break;
    case 'r': case 'R':
      Serial.println("Manual override: RECORDING state");
      changeState(STATE_RECORDING);
      break;
    case 'u': case 'U':
      Serial.println("Manual override: UPLOADING state");
      changeState(STATE_UPLOADING);
      break;
    case 'e': case 'E':
      Serial.println("Manual override: ERROR state");
      errorCode = 1;  // SD error
      changeState(STATE_ERROR);
      break;
    case 's': case 'S':
      showCurrentState();
      break;
    case 'f': case 'F':
      listFiles();
      break;
    default:
      Serial.println("Unknown command. Use 'i', 'l', 'r', 'u', 'e', 's', or 'f'");
      break;
  }
}

void handleCurrentState() {
  unsigned long stateTime = millis() - stateStartTime;
  
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
      
    case STATE_ERROR:
      handleErrorState();
      break;
  }
}

void handleInitState() {
  static bool sdInitialized = false;
  static bool audioInitialized = false;
  static bool wifiInitialized = false;
  
  // Initialize SD card
  if (!sdInitialized) {
    Serial.println("Initializing SD card...");
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    if (!SD.begin(SD_CS_PIN)) {
      Serial.println("❌ SD card initialization failed!");
      errorCode = 1;
      changeState(STATE_ERROR);
      return;
    }
    
    // Ensure recordings directory exists
    if (!SD.exists("/recordings")) {
      SD.mkdir("/recordings");
    }
    
    Serial.println("✅ SD card initialized");
    sdInitialized = true;
  }
  
  // Initialize audio (PDM microphone)
  if (!audioInitialized) {
    Serial.println("Initializing PDM microphone...");
    I2S.setPinsPdmRx(42, 41);  // Clock, Data
    
    if (!I2S.begin(I2S_MODE_PDM_RX, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
      Serial.println("❌ PDM microphone initialization failed!");
      errorCode = 1;
      changeState(STATE_ERROR);
      return;
    }
    
    Serial.println("✅ PDM microphone initialized");
    audioInitialized = true;
  }
  
  // Initialize WiFi (optional - can work without it)
  if (!wifiInitialized) {
    Serial.println("Initializing WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    // Try for 5 seconds
    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 5000) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      Serial.printf("✅ WiFi connected: %s\n", WiFi.localIP().toString().c_str());
    } else {
      wifiConnected = false;
      Serial.println("⚠️ WiFi connection failed - will work offline");
    }
    
    wifiInitialized = true;
  }
  
  // All initialization complete - move to listening
  if (sdInitialized && audioInitialized && wifiInitialized) {
    Serial.println("✅ Initialization complete - starting listening");
    changeState(STATE_LISTENING);
  }
}

void handleListeningState() {
  // Perform voice activity detection
  if (performVAD()) {
    Serial.println("🎤 Voice detected - starting recording");
    changeState(STATE_RECORDING);
  }
  
  // Auto-transition to upload if files are waiting (every 30 seconds)
  unsigned long stateTime = millis() - stateStartTime;
  if (stateTime > 30000 && hasUnuploadedFiles() && wifiConnected) {
    Serial.println("📤 Files waiting for upload - switching to upload mode");
    changeState(STATE_UPLOADING);
  }
}

void handleRecordingState() {
  static bool recordingStarted = false;
  static unsigned long recordingStartTime = 0;
  static int samplesRecorded = 0;
  
  if (!recordingStarted) {
    // Start new recording
    recordingCount++;
    currentRecordingFile = "/recordings/rec_" + String(recordingCount) + ".wav";
    
    recordingFile = SD.open(currentRecordingFile, FILE_WRITE);
    if (!recordingFile) {
      Serial.println("❌ Failed to create recording file");
      errorCode = 1;
      changeState(STATE_ERROR);
      return;
    }
    
    // Write WAV header (44 bytes)
    writeWAVHeader(recordingFile, TOTAL_SAMPLES);
    
    recordingStarted = true;
    recordingStartTime = millis();
    samplesRecorded = 0;
    
    Serial.printf("📼 Recording to: %s\n", currentRecordingFile.c_str());
  }
  
  // Record audio samples
  while (I2S.available() && samplesRecorded < TOTAL_SAMPLES) {
    int32_t sample = I2S.read();
    if (sample != 0 && sample != -1 && sample != 1) {
      int16_t audioSample = (int16_t)sample;
      recordingFile.write((uint8_t*)&audioSample, 2);
      samplesRecorded++;
    }
  }
  
  // Check if recording is complete
  unsigned long recordingTime = millis() - recordingStartTime;
  if (samplesRecorded >= TOTAL_SAMPLES || recordingTime > (RECORD_DURATION * 1000 + 1000)) {
    // Complete the recording
    recordingFile.close();
    recordingStarted = false;
    
    Serial.printf("✅ Recording complete: %d samples in %lu ms\n", 
      samplesRecorded, recordingTime);
    
    // Transition to upload if WiFi available, otherwise back to listening
    if (wifiConnected) {
      changeState(STATE_UPLOADING);
    } else {
      Serial.println("⚠️ No WiFi - returning to listening mode");
      changeState(STATE_LISTENING);
    }
  }
}

void handleUploadingState() {
  static bool uploadInProgress = false;
  static unsigned long uploadStartTime = 0;
  
  if (!uploadInProgress) {
    uploadStartTime = millis();
    uploadInProgress = true;
    Serial.println("📤 Starting upload process...");
  }
  
  // Find files to upload
  File dir = SD.open("/recordings");
  if (!dir) {
    Serial.println("❌ Cannot access recordings directory");
    errorCode = 1;
    changeState(STATE_ERROR);
    return;
  }
  
  bool filesUploaded = false;
  File file = dir.openNextFile();
  while (file) {
    String filename = file.name();
    if (filename.endsWith(".wav")) {
      String fullPath = "/recordings/" + filename;
      
      Serial.printf("📤 Uploading: %s\n", filename.c_str());
      if (uploadFile(fullPath)) {
        Serial.printf("✅ Upload successful: %s\n", filename.c_str());
        filesUploaded = true;
        
        // Move to uploaded directory or delete
        String uploadedPath = "/uploaded/" + filename;
        if (!SD.exists("/uploaded")) {
          SD.mkdir("/uploaded");
        }
        
        // For now, just rename to mark as uploaded
        String uploadedMarker = fullPath + ".uploaded";
        SD.rename(fullPath.c_str(), uploadedMarker.c_str());
        
      } else {
        Serial.printf("❌ Upload failed: %s\n", filename.c_str());
        errorCode = 3;
        changeState(STATE_ERROR);
        return;
      }
      
      break;  // Upload one file per cycle
    }
    file = dir.openNextFile();
  }
  dir.close();
  
  uploadInProgress = false;
  
  if (filesUploaded) {
    Serial.println("✅ Upload cycle complete - returning to listening");
    changeState(STATE_LISTENING);
  } else {
    Serial.println("ℹ️ No files to upload - returning to listening");
    changeState(STATE_LISTENING);
  }
}

void handleErrorState() {
  static unsigned long errorStartTime = 0;
  if (errorStartTime == 0) {
    errorStartTime = millis();
    Serial.printf("❌ ERROR STATE - Code: %d\n", errorCode);
    
    switch (errorCode) {
      case 1:
        Serial.println("SD card or file system error");
        break;
      case 2:
        Serial.println("WiFi connection error");
        break;
      case 3:
        Serial.println("Upload error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }
  
  // Stay in error state for 10 seconds, then try to recover
  if (millis() - errorStartTime > 10000) {
    Serial.println("🔄 Attempting error recovery...");
    errorCode = 0;
    errorStartTime = 0;
    changeState(STATE_INIT);
  }
}

void changeState(system_state_t newState) {
  if (currentState != newState) {
    Serial.printf("State: %s -> %s\n", stateToString(currentState), stateToString(newState));
    currentState = newState;
    stateStartTime = millis();
    lastLEDToggle = 0;  // Reset LED timing
  }
}

const char* stateToString(system_state_t state) {
  switch (state) {
    case STATE_INIT: return "INIT";
    case STATE_LISTENING: return "LISTENING";
    case STATE_RECORDING: return "RECORDING";
    case STATE_UPLOADING: return "UPLOADING";
    case STATE_ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}

void updateLED() {
  switch (currentState) {
    case STATE_INIT:
      digitalWrite(LED_PIN, HIGH);  // Solid on during init
      break;
      
    case STATE_LISTENING:
      // Slow pulse (1Hz)
      if (millis() - lastLEDToggle > LED_LISTENING_PERIOD / 2) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);  // Inverted for XIAO
        lastLEDToggle = millis();
      }
      break;
      
    case STATE_RECORDING:
      // Fast blink (3Hz)
      if (millis() - lastLEDToggle > LED_RECORDING_PERIOD / 2) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);  // Inverted for XIAO
        lastLEDToggle = millis();
      }
      break;
      
    case STATE_UPLOADING:
      // Double flash pattern
      {
        unsigned long elapsed = (millis() - stateStartTime) % 1000;
        if (elapsed < LED_UPLOADING_ON_TIME) {
          digitalWrite(LED_PIN, LOW);  // First flash
        } else if (elapsed < LED_UPLOADING_ON_TIME + LED_UPLOADING_OFF_TIME) {
          digitalWrite(LED_PIN, HIGH);
        } else if (elapsed < LED_UPLOADING_ON_TIME * 2 + LED_UPLOADING_OFF_TIME) {
          digitalWrite(LED_PIN, LOW);  // Second flash
        } else {
          digitalWrite(LED_PIN, HIGH);
        }
      }
      break;
      
    case STATE_ERROR:
      // Rapid flash for error
      if (millis() - lastLEDToggle > LED_ERROR_PERIOD / 2) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);  // Inverted for XIAO
        lastLEDToggle = millis();
      }
      break;
  }
}

bool performVAD() {
  if (!I2S.available()) {
    return false;
  }
  
  // Read audio samples for VAD analysis
  int samplesRead = 0;
  while (I2S.available() && samplesRead < VAD_SAMPLES) {
    int32_t sample = I2S.read();
    if (sample != 0 && sample != -1 && sample != 1) {
      audioBuffer[samplesRead] = (int16_t)sample;
      samplesRead++;
    }
  }
  
  if (samplesRead < VAD_SAMPLES / 2) {
    return false;  // Not enough samples
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
  
  // Apply VAD thresholds (from Test 6)
  bool rmsCheck = (rms > RMS_THRESHOLD);
  bool varianceCheck = (variance > VARIANCE_MIN && variance < VARIANCE_MAX);
  bool voiceDetected = rmsCheck && varianceCheck;
  
  // Debug output (occasional)
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 2000) {
    Serial.printf("VAD: RMS=%.1f, Var=%.1f, Voice=%s\n", 
      rms, variance, voiceDetected ? "YES" : "NO");
    lastDebug = millis();
  }
  
  return voiceDetected;
}

bool hasUnuploadedFiles() {
  File dir = SD.open("/recordings");
  if (!dir) return false;
  
  File file = dir.openNextFile();
  while (file) {
    String filename = file.name();
    if (filename.endsWith(".wav") && !filename.endsWith(".uploaded")) {
      dir.close();
      return true;
    }
    file = dir.openNextFile();
  }
  dir.close();
  return false;
}

bool uploadFile(const String& filename) {
  if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
    return false;
  }
  
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    return false;
  }
  
  size_t fileSize = file.size();
  
  HTTPClient http;
  http.begin(API_ENDPOINT);
  http.addHeader("Content-Type", "audio/wav");
  http.addHeader("Content-Length", String(fileSize));
  
  String deviceId = WiFi.macAddress();
  deviceId.replace(":", "");
  http.addHeader("X-Device-ID", deviceId);
  http.addHeader("X-Timestamp", String(millis()));
  
  String basename = filename.substring(filename.lastIndexOf('/') + 1);
  http.addHeader("X-Filename", basename);
  
  http.setTimeout(API_TIMEOUT_MS);
  
  int httpCode = http.sendRequest("POST", &file, fileSize);
  
  file.close();
  http.end();
  
  return (httpCode >= 200 && httpCode < 300);
}

void writeWAVHeader(File& file, int totalSamples) {
  uint32_t fileSize = 44 + (totalSamples * 2) - 8;
  uint32_t dataSize = totalSamples * 2;
  
  // WAV header
  file.write((uint8_t*)"RIFF", 4);
  file.write((uint8_t*)&fileSize, 4);
  file.write((uint8_t*)"WAVE", 4);
  
  // Format chunk
  file.write((uint8_t*)"fmt ", 4);
  uint32_t fmtSize = 16;
  file.write((uint8_t*)&fmtSize, 4);
  uint16_t audioFormat = 1;  // PCM
  file.write((uint8_t*)&audioFormat, 2);
  uint16_t channels = 1;  // Mono
  file.write((uint8_t*)&channels, 2);
  uint32_t sampleRate = SAMPLE_RATE;
  file.write((uint8_t*)&sampleRate, 4);
  uint32_t byteRate = sampleRate * channels * 2;
  file.write((uint8_t*)&byteRate, 4);
  uint16_t blockAlign = channels * 2;
  file.write((uint8_t*)&blockAlign, 2);
  uint16_t bitsPerSample = 16;
  file.write((uint8_t*)&bitsPerSample, 2);
  
  // Data chunk
  file.write((uint8_t*)"data", 4);
  file.write((uint8_t*)&dataSize, 4);
}

void showCurrentState() {
  Serial.printf("\n=== Current State: %s ===\n", stateToString(currentState));
  Serial.printf("State time: %lu ms\n", millis() - stateStartTime);
  Serial.printf("WiFi connected: %s\n", wifiConnected ? "Yes" : "No");
  Serial.printf("Files waiting: %s\n", hasUnuploadedFiles() ? "Yes" : "No");
  Serial.printf("Error code: %d\n", errorCode);
  Serial.println();
}

void listFiles() {
  Serial.println("\n=== SD Card Files ===");
  
  File dir = SD.open("/recordings");
  if (!dir) {
    Serial.println("❌ Cannot open /recordings");
    return;
  }
  
  int fileCount = 0;
  File file = dir.openNextFile();
  while (file) {
    String filename = file.name();
    size_t fileSize = file.size();
    
    Serial.printf("%s - %zu bytes", filename.c_str(), fileSize);
    if (filename.endsWith(".wav")) {
      Serial.print(" [WAV]");
    }
    if (filename.endsWith(".uploaded")) {
      Serial.print(" [UPLOADED]");
    }
    Serial.println();
    
    fileCount++;
    file = dir.openNextFile();
  }
  dir.close();
  
  Serial.printf("Total: %d files\n", fileCount);
  Serial.println();
}
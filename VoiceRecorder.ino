#include "config.h"
#include "audio_recorder.h"
#include "voice_detection.h"
#include "power_management.h"
#include "led_control.h"
#include "sd_manager.h"
#include "wifi_sync.h"
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <driver/i2s.h>
#include <esp_sleep.h>

typedef enum {
  STATE_IDLE,
  STATE_LISTENING,
  STATE_RECORDING,
  STATE_UPLOADING,
  STATE_LOW_BATTERY,
  STATE_ERROR
} system_state_t;

system_state_t currentState = STATE_IDLE;
bool usbConnected = false;
unsigned long lastActivityTime = 0;
unsigned long recordingStartTime = 0;
String currentRecordingFile = "";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("XIAO ESP32S3 Voice Recorder v1.0");
  Serial.println("Initializing...");
  
  if (!initializeSystem()) {
    Serial.println("System initialization failed!");
    currentState = STATE_ERROR;
    return;
  }
  
  Serial.println("System ready");
  currentState = STATE_LISTENING;
  setLEDMode(LED_LISTENING);
}

void loop() {
  usbConnected = isUSBConnected();
  
  switch (currentState) {
    case STATE_LISTENING:
      handleListeningState();
      break;
      
    case STATE_RECORDING:
      handleRecordingState();
      break;
      
    case STATE_UPLOADING:
      handleUploadingState();
      break;
      
    case STATE_LOW_BATTERY:
      handleLowBatteryState();
      break;
      
    case STATE_ERROR:
      handleErrorState();
      break;
      
    case STATE_IDLE:
    default:
      handleIdleState();
      break;
  }
  
  updateLED();
  
  if (usbConnected && currentState != STATE_UPLOADING && currentState != STATE_RECORDING) {
    if (shouldStartUpload()) {
      currentState = STATE_UPLOADING;
      setLEDMode(LED_UPLOADING);
    }
  }
  
  if (getBatteryPercentage() < LOW_BATTERY_THRESHOLD && !usbConnected) {
    currentState = STATE_LOW_BATTERY;
    setLEDMode(LED_LOW_BATTERY);
  }
  
  delay(10);
}

bool initializeSystem() {
  initializeLED();
  setLEDMode(LED_SOLID);
  
  if (!initializeSDCard()) {
    Serial.println("SD card initialization failed");
    setLEDMode(LED_ERROR);
    return false;
  }
  
  if (!initializeAudio()) {
    Serial.println("Audio initialization failed");
    setLEDMode(LED_ERROR);
    return false;
  }
  
  if (!initializePowerManagement()) {
    Serial.println("Power management initialization failed");
    setLEDMode(LED_ERROR);
    return false;
  }
  
  initializeVAD();
  
  Serial.println("All systems initialized successfully");
  return true;
}

void handleListeningState() {
  if (detectVoiceActivity()) {
    Serial.println("Voice detected, starting recording");
    currentRecordingFile = generateRecordingFilename();
    
    if (startRecording(currentRecordingFile)) {
      currentState = STATE_RECORDING;
      setLEDMode(LED_RECORDING);
      recordingStartTime = millis();
      lastActivityTime = millis();
    } else {
      Serial.println("Failed to start recording");
      currentState = STATE_ERROR;
      setLEDMode(LED_ERROR);
    }
  } else {
    if (millis() - lastActivityTime > SLEEP_TIMEOUT_MS) {
      enterLightSleep();
      lastActivityTime = millis();
    }
  }
}

void handleRecordingState() {
  if (!continueRecording()) {
    Serial.println("Recording error occurred");
    stopRecording();
    currentState = STATE_ERROR;
    setLEDMode(LED_ERROR);
    return;
  }
  
  if (millis() - recordingStartTime > MAX_RECORDING_DURATION_MS) {
    Serial.println("Maximum recording duration reached");
    stopRecording();
    Serial.printf("Recording saved: %s\n", currentRecordingFile.c_str());
    currentState = STATE_LISTENING;
    setLEDMode(LED_LISTENING);
    return;
  }
  
  if (!detectVoiceActivity()) {
    static unsigned long silenceStartTime = 0;
    
    if (silenceStartTime == 0) {
      silenceStartTime = millis();
    } else if (millis() - silenceStartTime > SILENCE_TIMEOUT_MS) {
      Serial.println("Silence detected, stopping recording");
      stopRecording();
      Serial.printf("Recording saved: %s\n", currentRecordingFile.c_str());
      currentState = STATE_LISTENING;
      setLEDMode(LED_LISTENING);
      silenceStartTime = 0;
    }
  } else {
    lastActivityTime = millis();
  }
}

void handleUploadingState() {
  if (!usbConnected) {
    Serial.println("USB disconnected, stopping upload");
    currentState = STATE_LISTENING;
    setLEDMode(LED_LISTENING);
    return;
  }
  
  if (performUpload()) {
    Serial.println("Upload completed successfully");
    currentState = STATE_LISTENING;
    setLEDMode(LED_LISTENING);
  } else {
    Serial.println("Upload failed, will retry later");
    currentState = STATE_LISTENING;
    setLEDMode(LED_LISTENING);
  }
}

void handleLowBatteryState() {
  if (usbConnected) {
    Serial.println("USB connected, exiting low battery mode");
    currentState = STATE_LISTENING;
    setLEDMode(LED_LISTENING);
    return;
  }
  
  if (getBatteryPercentage() < CRITICAL_BATTERY_THRESHOLD) {
    Serial.println("Critical battery level, entering deep sleep");
    enterDeepSleep();
  }
  
  delay(5000);
}

void handleErrorState() {
  Serial.println("System in error state");
  
  if (millis() % 10000 == 0) {
    Serial.println("Attempting system recovery...");
    if (initializeSystem()) {
      Serial.println("System recovery successful");
      currentState = STATE_LISTENING;
      setLEDMode(LED_LISTENING);
    }
  }
  
  delay(1000);
}

void handleIdleState() {
  if (millis() - lastActivityTime > DEEP_SLEEP_TIMEOUT_MS) {
    enterDeepSleep();
  }
  
  currentState = STATE_LISTENING;
  setLEDMode(LED_LISTENING);
}
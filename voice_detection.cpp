#include "voice_detection.h"
#include "config.h"
#include <math.h>

static float noiseFloor = VAD_NOISE_FLOOR;
static float currentAudioLevel = 0.0;
static int16_t vadBuffer[VAD_SAMPLE_WINDOW];
static bool vadInitialized = false;
static unsigned long lastNoiseUpdate = 0;
static float runningAverage = 0.0;
static int sampleCount = 0;

void initializeVAD() {
  noiseFloor = VAD_NOISE_FLOOR;
  currentAudioLevel = 0.0;
  runningAverage = 0.0;
  sampleCount = 0;
  lastNoiseUpdate = millis();
  vadInitialized = true;
  
  DEBUG_PRINTLN("VAD initialized");
  
  calibrateVAD();
}

float calculateRMS(int16_t* samples, int count) {
  if (count <= 0) return 0.0;
  
  float sum = 0.0;
  for (int i = 0; i < count; i++) {
    float sample = (float)samples[i];
    sum += sample * sample;
  }
  
  return sqrt(sum / count);
}

float calculateZeroCrossingRate(int16_t* samples, int count) {
  if (count <= 1) return 0.0;
  
  int crossings = 0;
  for (int i = 1; i < count; i++) {
    if ((samples[i-1] >= 0 && samples[i] < 0) || (samples[i-1] < 0 && samples[i] >= 0)) {
      crossings++;
    }
  }
  
  return (float)crossings / (count - 1);
}

bool detectVoiceActivity() {
  if (!vadInitialized) {
    return false;
  }

  size_t bytesRead = 0;
  esp_err_t err = i2s_read(I2S_PORT, vadBuffer, sizeof(vadBuffer), &bytesRead, 0);
  
  if (err != ESP_OK || bytesRead == 0) {
    return false;
  }
  
  int sampleCount = bytesRead / sizeof(int16_t);
  
  float rms = calculateRMS(vadBuffer, sampleCount);
  float zcr = calculateZeroCrossingRate(vadBuffer, sampleCount);
  
  currentAudioLevel = rms;
  
  updateNoiseFloor();
  
  float threshold = noiseFloor * VAD_SENSITIVITY;
  
  bool voiceDetected = (rms > threshold) && (zcr > 0.1) && (zcr < 0.8);
  
  if (DEBUG_ENABLED && millis() % 1000 == 0) {
    DEBUG_PRINTF("VAD - RMS: %.2f, ZCR: %.3f, Noise: %.2f, Threshold: %.2f, Voice: %s\n", 
                 rms, zcr, noiseFloor, threshold, voiceDetected ? "YES" : "NO");
  }
  
  return voiceDetected;
}

void updateNoiseFloor() {
  if (millis() - lastNoiseUpdate < 100) {
    return;
  }
  
  runningAverage = (runningAverage * 0.95) + (currentAudioLevel * 0.05);
  
  if (currentAudioLevel < noiseFloor * 1.5) {
    noiseFloor = (noiseFloor * 0.99) + (currentAudioLevel * 0.01);
  }
  
  noiseFloor = MAX(noiseFloor, VAD_NOISE_FLOOR);
  
  lastNoiseUpdate = millis();
  sampleCount++;
}

float getAudioLevel() {
  return currentAudioLevel;
}

void calibrateVAD() {
  DEBUG_PRINTLN("Calibrating VAD - please remain quiet for 3 seconds...");
  
  float samples[30];
  int sampleIndex = 0;
  
  for (int i = 0; i < 30; i++) {
    size_t bytesRead = 0;
    esp_err_t err = i2s_read(I2S_PORT, vadBuffer, sizeof(vadBuffer), &bytesRead, portMAX_DELAY);
    
    if (err == ESP_OK && bytesRead > 0) {
      int count = bytesRead / sizeof(int16_t);
      float rms = calculateRMS(vadBuffer, count);
      samples[sampleIndex++] = rms;
    }
    
    delay(100);
  }
  
  if (sampleIndex > 0) {
    float sum = 0.0;
    for (int i = 0; i < sampleIndex; i++) {
      sum += samples[i];
    }
    
    float averageNoise = sum / sampleIndex;
    noiseFloor = MAX(averageNoise * 1.2, VAD_NOISE_FLOOR);
    
    DEBUG_PRINTF("VAD calibration complete. Noise floor: %.2f\n", noiseFloor);
  } else {
    DEBUG_PRINTLN("VAD calibration failed, using default values");
  }
}
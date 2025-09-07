/*
 * Voice Activity Detection Test - XIAO ESP32S3 Sense
 * 
 * Tests VAD algorithm with adjustable sensitivity
 * Uses working PDM microphone code
 */

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  #include "ESP_I2S.h"
  I2SClass I2S;
#else
  #include <I2S.h>
#endif

#include <math.h>

// Hardware pins
#define I2S_CLK_PIN 42    // PDM Clock
#define I2S_DIN_PIN 41    // PDM Data
#define LED_PIN LED_BUILTIN

// Audio settings
#define SAMPLE_RATE 16000
#define BITS_PER_SAMPLE 16
#define CHANNELS 1

// VAD Configuration - adjustable for testing
#define VAD_SAMPLE_WINDOW 128     // Smaller window for faster response
#define VAD_NOISE_FLOOR 50        // Minimum noise floor (reduced from 100)
#define VAD_SENSITIVITY 2.0       // Sensitivity multiplier
#define VAD_MIN_ZCR 0.05          // Minimum zero crossing rate
#define VAD_MAX_ZCR 0.8           // Maximum zero crossing rate

// VAD state
float noiseFloor = VAD_NOISE_FLOOR;
float currentAudioLevel = 0.0;
int16_t vadBuffer[VAD_SAMPLE_WINDOW];
bool vadInitialized = false;
unsigned long lastNoiseUpdate = 0;
float runningAverage = 0.0;

// Test modes
int currentSensitivity = 0;
float sensitivityLevels[] = {1.0, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0};
int numSensitivityLevels = sizeof(sensitivityLevels) / sizeof(sensitivityLevels[0]);
unsigned long lastSensitivityChange = 0;

// Manual thresholds (no auto-adaptation)
bool useManualThreshold = true;  // Always use manual control
float manualRMSThreshold = 50.0;  // Manual RMS threshold

// ZCR toggle for testing (now controls variance)
bool useZCR = true;
float zcrMin = 0.02;
float zcrMax = 0.5;

// Variance thresholds (min and max for voice detection)
float varianceThresholdMin = 5000.0;
float varianceThresholdMax = 100000.0;  // Filter out cloth rubbing, etc.

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("=== Voice Activity Detection Test (XIAO ESP32S3 Sense) ===");
  Serial.println("Testing VAD algorithm with different sensitivity levels...");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  // Initialize PDM microphone
  Serial.println("Step 1: Initializing PDM microphone...");
  I2S.setPinsPdmRx(I2S_CLK_PIN, I2S_DIN_PIN);
  
  if (!I2S.begin(I2S_MODE_PDM_RX, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println("❌ Failed to initialize PDM microphone!");
    errorBlink();
    return;
  }
  Serial.println("✅ PDM microphone initialized successfully");
  
  // Initialize VAD
  Serial.println("Step 2: Initializing VAD...");
  initializeVAD();
  
  digitalWrite(LED_PIN, LOW);
  Serial.println();
  Serial.println("VAD Test Ready!");
  Serial.println("Commands:");
  Serial.println("  'r' - Increase RMS threshold (+10)");
  Serial.println("  'e' - Decrease RMS threshold (-10)"); 
  Serial.println("  'v' - Increase VAR min threshold (+1000)");
  Serial.println("  'b' - Decrease VAR min threshold (-1000)");
  Serial.println("  'm' - Increase VAR max threshold (+10000)");
  Serial.println("  'l' - Decrease VAR max threshold (-10000)");
  Serial.println("  'z' - Toggle RMS+VAR vs RMS-only mode");
  Serial.println("  'n' - Show detailed noise analysis");
  Serial.println("  'x' - Reset thresholds to defaults");
  Serial.println();
  Serial.println("Make noise to test voice detection...");
  
  printCurrentSettings();
}

void loop() {
  // Handle serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }
  
  // No auto-cycling - manual control only
  
  // Perform VAD detection
  static bool lastVoiceDetected = false;
  bool voiceDetected = detectVoiceActivity();
  
  // LED indicates voice detection with different patterns
  updateVoiceLED(voiceDetected);
  
  // Store last state for LED timing
  lastVoiceDetected = voiceDetected;
  
  delay(10); // Small delay to prevent overwhelming output
}

void initializeVAD() {
  noiseFloor = VAD_NOISE_FLOOR;
  currentAudioLevel = 0.0;
  runningAverage = 0.0;
  lastNoiseUpdate = millis();
  vadInitialized = true;
  
  Serial.println("✅ VAD initialized");
  calibrateVAD();
}

bool detectVoiceActivity() {
  if (!vadInitialized) {
    return false;
  }
  
  // Continuous streaming approach - always collect samples
  static int bufferIndex = 0;
  static float runningRMS = 0.0;
  static int sampleCount = 0;
  static bool currentDetection = false;
  
  // Read one sample per call
  int sample = I2S.read();
  
  // Skip invalid samples
  if (sample == 0 || sample == -1 || sample == 1) {
    return currentDetection; // Return current state
  }
  
  // Add sample to circular buffer
  vadBuffer[bufferIndex] = (int16_t)sample;
  bufferIndex = (bufferIndex + 1) % VAD_SAMPLE_WINDOW;
  sampleCount++;
  
  // Analyze every 16 samples for faster response (1ms at 16kHz)
  if (sampleCount % 16 == 0) {
    // Calculate RMS of current buffer
    float rms = calculateRMS(vadBuffer, VAD_SAMPLE_WINDOW);
    
    // Calculate Zero Crossing Rate (for debug only, not used in detection)
    float zcr = calculateZeroCrossingRate(vadBuffer, VAD_SAMPLE_WINDOW);
    
    currentAudioLevel = rms;
    // No auto noise floor updates - use manual control only
    
    // Use manual RMS threshold (no sensitivity multiplier)
    float threshold = manualRMSThreshold;
    
    // Voice detection with RMS and optional energy variance check
    bool rmsCheck = (rms > threshold);
    
    // Alternative to ZCR: Check if signal variance is in voice range
    float variance = calculateVariance(vadBuffer, VAD_SAMPLE_WINDOW);
    bool varianceCheck = (variance > varianceThresholdMin && variance < varianceThresholdMax);
    
    // Use variance instead of ZCR for now
    currentDetection = useZCR ? (rmsCheck && varianceCheck) : rmsCheck;
    
    // Debug every second with all thresholds clearly shown
    static unsigned long lastDebugPrint = 0;
    if (millis() - lastDebugPrint > 1000) {
      Serial.printf("=== VAD STATUS ===\n");
      Serial.printf("RMS: %.1f (thresh: %.1f) %s\n", rms, manualRMSThreshold, rmsCheck?"✓":"✗");
      Serial.printf("VAR: %.1f (range: %.1f-%.1f) %s\n", variance, varianceThresholdMin, varianceThresholdMax, varianceCheck?"✓":"✗");
      Serial.printf("Mode: %s | Voice: %s\n", useZCR ? "RMS+VAR" : "RMS-only", currentDetection ? "YES" : "NO");
      Serial.printf("Raw Sample: %d\n", sample);
      Serial.printf("==================\n");
      lastDebugPrint = millis();
    }
    
    // Show real-time activity only when both pass
    static unsigned long lastActivity = 0;
    if (currentDetection && millis() - lastActivity > 200) {
      Serial.printf(">>> VOICE: RMS=%.1f ZCR=%.3f\n", rms, zcr);
      lastActivity = millis();
    }
  }
  
  return currentDetection;
}

float calculateRMS(int16_t* samples, int count) {
  if (count <= 0) return 0.0;
  
  // Calculate DC offset (average)
  float dcOffset = 0.0;
  for (int i = 0; i < count; i++) {
    dcOffset += (float)samples[i];
  }
  dcOffset /= count;
  
  // Calculate RMS with DC offset removed
  float sum = 0.0;
  for (int i = 0; i < count; i++) {
    float sample = (float)samples[i] - dcOffset;  // Remove DC bias
    sum += sample * sample;
  }
  
  return sqrt(sum / count);
}

float calculateZeroCrossingRate(int16_t* samples, int count) {
  if (count <= 1) return 0.0;
  
  // Calculate DC offset (same as RMS function)
  float dcOffset = 0.0;
  for (int i = 0; i < count; i++) {
    dcOffset += (float)samples[i];
  }
  dcOffset /= count;
  
  // Calculate RMS for threshold (minimum signal needed for valid crossing)
  float rmsValue = 0.0;
  for (int i = 0; i < count; i++) {
    float sample = (float)samples[i] - dcOffset;
    rmsValue += sample * sample;
  }
  rmsValue = sqrt(rmsValue / count);
  
  // Only count zero crossings if signal is significantly above noise floor
  float crossingThreshold = fmax(rmsValue * 0.5, 20.0); // 50% of RMS or minimum 20
  
  // Count zero crossings using DC-offset-removed samples
  int crossings = 0;
  float prevSample = (float)samples[0] - dcOffset;
  
  for (int i = 1; i < count; i++) {
    float currentSample = (float)samples[i] - dcOffset;
    
    // Only count as crossing if both samples are above threshold
    if (abs(prevSample) > crossingThreshold || abs(currentSample) > crossingThreshold) {
      // Check for zero crossing
      if ((prevSample >= 0.0 && currentSample < 0.0) || (prevSample < 0.0 && currentSample >= 0.0)) {
        crossings++;
      }
    }
    
    prevSample = currentSample;
  }
  
  return (float)crossings / (count - 1);
}

float calculateVariance(int16_t* samples, int count) {
  if (count <= 1) return 0.0;
  
  // Calculate mean (DC offset)
  float mean = 0.0;
  for (int i = 0; i < count; i++) {
    mean += (float)samples[i];
  }
  mean /= count;
  
  // Calculate variance 
  float variance = 0.0;
  for (int i = 0; i < count; i++) {
    float diff = (float)samples[i] - mean;
    variance += diff * diff;
  }
  
  return variance / count;
}

void updateNoiseFloor() {
  if (millis() - lastNoiseUpdate < 100) {
    return;
  }
  
  // Adaptive noise floor using exponential moving average
  runningAverage = (runningAverage * 0.95) + (currentAudioLevel * 0.05);
  
  // Only update noise floor if current level is close to existing floor
  if (currentAudioLevel < noiseFloor * 1.5) {
    noiseFloor = (noiseFloor * 0.99) + (currentAudioLevel * 0.01);
  }
  
  // Enforce minimum noise floor
  noiseFloor = fmax(noiseFloor, (float)VAD_NOISE_FLOOR);
  
  lastNoiseUpdate = millis();
}

void calibrateVAD() {
  Serial.println("Calibrating VAD - please remain quiet for 3 seconds...");
  
  float samples[30];
  int sampleIndex = 0;
  
  for (int i = 0; i < 30; i++) {
    // Collect samples for RMS calculation
    int samplesCollected = 0;
    while (samplesCollected < VAD_SAMPLE_WINDOW) {
      int sample = I2S.read();
      if (sample != 0 && sample != -1 && sample != 1) {
        vadBuffer[samplesCollected] = (int16_t)sample;
        samplesCollected++;
      }
    }
    
    float rms = calculateRMS(vadBuffer, samplesCollected);
    samples[sampleIndex++] = rms;
    
    Serial.printf("Calibration sample %d/30: %.1f\n", i + 1, rms);
    delay(100);
  }
  
  if (sampleIndex > 0) {
    float sum = 0.0;
    for (int i = 0; i < sampleIndex; i++) {
      sum += samples[i];
    }
    
    float averageNoise = sum / sampleIndex;
    noiseFloor = fmax(averageNoise * 1.2, (float)VAD_NOISE_FLOOR);
    
    Serial.printf("✅ VAD calibration complete. Noise floor: %.2f\n", noiseFloor);
  } else {
    Serial.println("❌ VAD calibration failed, using default values");
  }
}

void handleCommand(char cmd) {
  switch (cmd) {
    case 'r':
    case 'R':
      manualRMSThreshold += 10.0;
      Serial.printf("\n>>> RMS threshold: %.1f <<<\n", manualRMSThreshold);
      break;
      
    case 'e':
    case 'E':
      manualRMSThreshold -= 10.0;
      if (manualRMSThreshold < 5.0) manualRMSThreshold = 5.0;
      Serial.printf("\n>>> RMS threshold: %.1f <<<\n", manualRMSThreshold);
      break;
      
    case 'v':
    case 'V':
      varianceThresholdMin += 1000.0;
      Serial.printf("\n>>> Variance min threshold: %.1f <<<\n", varianceThresholdMin);
      break;
      
    case 'b':
    case 'B':
      varianceThresholdMin -= 1000.0;
      if (varianceThresholdMin < 100.0) varianceThresholdMin = 100.0;
      Serial.printf("\n>>> Variance min threshold: %.1f <<<\n", varianceThresholdMin);
      break;
      
    case 'm':
    case 'M':
      varianceThresholdMax += 10000.0;
      Serial.printf("\n>>> Variance max threshold: %.1f <<<\n", varianceThresholdMax);
      break;
      
    case 'l':
    case 'L':
      varianceThresholdMax -= 10000.0;
      if (varianceThresholdMax < varianceThresholdMin + 1000.0) {
        varianceThresholdMax = varianceThresholdMin + 1000.0;
      }
      Serial.printf("\n>>> Variance max threshold: %.1f <<<\n", varianceThresholdMax);
      break;
      
    case 'z':
    case 'Z':
      useZCR = !useZCR;
      Serial.printf("\n>>> Mode: %s <<<\n", useZCR ? "RMS+VAR" : "RMS-only");
      break;
      
    case 'n':
    case 'N':
      performNoiseAnalysis();
      break;
      
    case 'x':
    case 'X':
      manualRMSThreshold = 50.0;
      varianceThresholdMin = 5000.0;
      varianceThresholdMax = 100000.0;
      useZCR = true;
      Serial.printf("\n>>> Reset to defaults: RMS=%.1f, VAR=%.1f-%.1f <<<\n", 
                    manualRMSThreshold, varianceThresholdMin, varianceThresholdMax);
      break;
      
    default:
      Serial.println("Unknown command. Use 'r', 'e', 'v', 'b', 'm', 'l', 'z', 'n', or 'x'");
      break;
  }
}

void printCurrentSettings() {
  Serial.println("========================================");
  Serial.printf("RMS Threshold: %.1f\n", manualRMSThreshold);
  Serial.printf("Variance Range: %.1f - %.1f\n", varianceThresholdMin, varianceThresholdMax);
  Serial.printf("Mode: %s\n", useZCR ? "RMS+VAR" : "RMS-only");
  Serial.println("Commands: r/e (RMS ±10), v/b (VAR min ±1000), m/l (VAR max ±10000)");
  Serial.println("         z (toggle), n (analyze), x (reset)");
  Serial.println("========================================");
}

void updateVoiceLED(bool voiceDetected) {
  static unsigned long lastLEDToggle = 0;
  static bool ledState = false;
  static unsigned long voiceStartTime = 0;
  static bool wasVoiceDetected = false;
  
  if (voiceDetected) {
    // Voice detected - fast blink pattern (like recording mode)
    if (millis() - lastLEDToggle > 166) { // 3Hz fast blink (333ms / 2)
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? LOW : HIGH); // Invert for active LOW LED
      lastLEDToggle = millis();
    }
    
    if (!wasVoiceDetected) {
      voiceStartTime = millis(); // Mark when voice started
    }
    wasVoiceDetected = true;
    
  } else {
    // No voice - stay on briefly after voice ends, then heartbeat blink
    unsigned long timeSinceVoice = millis() - voiceStartTime;
    
    if (wasVoiceDetected && timeSinceVoice < 1000) {
      // Stay on for 1 second after voice ends
      digitalWrite(LED_PIN, LOW); // LED ON (active LOW)
    } else {
      // Heartbeat blink - short blink every 5 seconds when idle
      unsigned long currentTime = millis();
      unsigned long heartbeatCycle = currentTime % 5000; // 5 second cycle
      
      if (heartbeatCycle < 10) {
        // Short 100ms blink at start of each 5-second cycle
        digitalWrite(LED_PIN, LOW);  // LED ON for the brief blink (active LOW)
      } else {
        // Off for the remaining 4.9 seconds  
        digitalWrite(LED_PIN, HIGH); // LED OFF most of the time (active LOW)
      }
    }
    
    if (wasVoiceDetected && timeSinceVoice >= 1000) {
      wasVoiceDetected = false; // Reset voice detected flag
    }
  }
}

void performNoiseAnalysis() {
  Serial.println("\n=== PDM NOISE ANALYSIS ===");
  Serial.println("Collecting 1000 samples for analysis...");
  
  int16_t analysisBuffer[1000];
  int samplesCollected = 0;
  
  // Collect samples
  while (samplesCollected < 1000) {
    int sample = I2S.read();
    if (sample != 0 && sample != -1 && sample != 1) {
      analysisBuffer[samplesCollected] = (int16_t)sample;
      samplesCollected++;
    }
  }
  
  // Calculate statistics
  int32_t sum = 0;
  int16_t minVal = 32767;
  int16_t maxVal = -32768;
  
  for (int i = 0; i < 1000; i++) {
    sum += analysisBuffer[i];
    if (analysisBuffer[i] < minVal) minVal = analysisBuffer[i];
    if (analysisBuffer[i] > maxVal) maxVal = analysisBuffer[i];
  }
  
  float mean = (float)sum / 1000.0;
  
  // Calculate variance and std deviation
  float variance = 0.0;
  for (int i = 0; i < 1000; i++) {
    float diff = (float)analysisBuffer[i] - mean;
    variance += diff * diff;
  }
  variance /= 1000.0;
  float stdDev = sqrt(variance);
  
  // Calculate RMS
  float rmsSum = 0.0;
  for (int i = 0; i < 1000; i++) {
    float dcRemoved = (float)analysisBuffer[i] - mean;
    rmsSum += dcRemoved * dcRemoved;
  }
  float rms = sqrt(rmsSum / 1000.0);
  
  // Print results
  Serial.println("--- PDM Signal Characteristics ---");
  Serial.printf("DC Offset (Mean): %.1f\n", mean);
  Serial.printf("Min Value: %d\n", minVal);
  Serial.printf("Max Value: %d\n", maxVal);
  Serial.printf("Range: %d\n", maxVal - minVal);
  Serial.printf("Standard Deviation: %.1f\n", stdDev);
  Serial.printf("Variance: %.1f\n", variance);
  Serial.printf("RMS (DC removed): %.1f\n", rms);
  
  // Show sample distribution
  Serial.println("\n--- First 20 samples ---");
  for (int i = 0; i < 20; i++) {
    Serial.printf("%d ", analysisBuffer[i]);
    if ((i + 1) % 10 == 0) Serial.println();
  }
  
  // Show last 20 samples  
  Serial.println("\n--- Last 20 samples ---");
  for (int i = 980; i < 1000; i++) {
    Serial.printf("%d ", analysisBuffer[i]);
    if ((i - 979) % 10 == 0) Serial.println();
  }
  
  Serial.println("\n=== Analysis Complete ===\n");
}

void errorBlink() {
  while (1) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}
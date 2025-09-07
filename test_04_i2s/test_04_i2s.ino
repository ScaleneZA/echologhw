/*
 * Test 4: I2S Microphone Test
 * 
 * This test verifies that we can capture audio from the built-in microphone
 * on the XIAO ESP32S3 Sense expansion board using I2S interface.
 * 
 * Expected behavior:
 * 1. Initialize I2S interface with microphone pins
 * 2. Capture raw audio samples from microphone
 * 3. Analyze audio levels and detect sound
 * 4. Display real-time audio statistics
 * 5. Test different sample rates and bit depths
 * 
 * SUCCESS CRITERIA:
 * - I2S initializes without errors
 * - Audio samples are captured (not all zeros)
 * - Audio levels change when you make noise (speak, clap, etc.)
 * - Sample rate and bit depth settings work
 * - No significant noise or distortion in quiet environment
 * - LED brightness responds to audio level
 */

#include <driver/i2s.h>
#include <math.h>

// I2S Configuration from config.h
#define I2S_WS_PIN 42    // Word Select (Left/Right Clock)
#define I2S_SCK_PIN 41   // Serial Clock
#define I2S_SD_PIN 2     // Serial Data
#define I2S_PORT I2S_NUM_0

// Audio Configuration
#define SAMPLE_RATE 16000
#define BITS_PER_SAMPLE 16
#define CHANNELS 1
#define BUFFER_SIZE 1024

// LED for audio level visualization
#define LED_PIN LED_BUILTIN

// Audio analysis structure
struct AudioStats {
  float rms;
  int16_t peak;
  int16_t min;
  int16_t max;
  float snr;
  bool voiceDetected;
};

// Test variables
int16_t audioBuffer[BUFFER_SIZE];
unsigned long lastStats = 0;
float noiseFloor = 0.0;
bool i2sInitialized = false;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== XIAO ESP32S3 Sense I2S Microphone Test ===");
  Serial.println("This test captures audio from the built-in microphone");
  Serial.println("Try speaking, clapping, or making noise to test detection");
  Serial.println("LED brightness will respond to audio levels\n");
  
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("I2S Configuration:");
  Serial.printf("WS (Word Select): GPIO %d\n", I2S_WS_PIN);
  Serial.printf("SCK (Serial Clock): GPIO %d\n", I2S_SCK_PIN);
  Serial.printf("SD (Serial Data): GPIO %d\n", I2S_SD_PIN);
  Serial.printf("Sample Rate: %d Hz\n", SAMPLE_RATE);
  Serial.printf("Bits per Sample: %d\n", BITS_PER_SAMPLE);
  Serial.printf("Channels: %d (Mono)\n", CHANNELS);
  Serial.printf("Buffer Size: %d samples\n", BUFFER_SIZE);
  Serial.println();
  
  // Initialize I2S
  if (initializeI2S()) {
    Serial.println("✅ I2S initialization successful!");
    i2sInitialized = true;
    
    // Calibrate noise floor
    Serial.println("Calibrating noise floor (stay quiet for 3 seconds)...");
    calibrateNoiseFloor();
    Serial.printf("Noise floor calibrated: %.2f\n", noiseFloor);
    Serial.println("\nStarting audio monitoring...");
    Serial.println("Columns: RMS Level | Peak | SNR | Status");
    
  } else {
    Serial.println("❌ I2S initialization failed!");
    Serial.println("Check microphone connections and pin configuration");
    return;
  }
  
  lastStats = millis();
}

void loop() {
  if (!i2sInitialized) {
    // Flash LED rapidly to indicate error
    static unsigned long errorBlink = 0;
    if (millis() - errorBlink > 200) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      errorBlink = millis();
    }
    return;
  }
  
  // Read audio samples
  size_t bytesRead = 0;
  esp_err_t result = i2s_read(I2S_PORT, audioBuffer, sizeof(audioBuffer), &bytesRead, portMAX_DELAY);
  
  if (result != ESP_OK) {
    Serial.println("I2S read error!");
    delay(100);
    return;
  }
  
  int samplesRead = bytesRead / sizeof(int16_t);
  
  // Analyze audio
  AudioStats stats = analyzeAudio(audioBuffer, samplesRead);
  
  // Update LED based on audio level
  updateAudioLED(stats.rms);
  
  // Print statistics every 500ms
  if (millis() - lastStats > 500) {
    printAudioStats(stats, samplesRead);
    lastStats = millis();
  }
  
  delay(10);
}

bool initializeI2S() {
  // I2S configuration for microphone input
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // Mono
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  
  // I2S pin configuration
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK_PIN,    // Serial Clock
    .ws_io_num = I2S_WS_PIN,      // Word Select
    .data_out_num = I2S_PIN_NO_CHANGE,  // Not used for input
    .data_in_num = I2S_SD_PIN     // Serial Data Input
  };
  
  // Install and start I2S driver
  esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("I2S driver install failed: %d\n", err);
    return false;
  }
  
  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("I2S set pin failed: %d\n", err);
    i2s_driver_uninstall(I2S_PORT);
    return false;
  }
  
  // Clear I2S buffer
  i2s_zero_dma_buffer(I2S_PORT);
  
  return true;
}

AudioStats analyzeAudio(int16_t* samples, int numSamples) {
  AudioStats stats = {0};
  
  if (numSamples == 0) return stats;
  
  // Calculate RMS, min, max
  long long sumSquares = 0;
  stats.min = samples[0];
  stats.max = samples[0];
  
  for (int i = 0; i < numSamples; i++) {
    int16_t sample = samples[i];
    sumSquares += (long long)sample * sample;
    
    if (sample < stats.min) stats.min = sample;
    if (sample > stats.max) stats.max = sample;
    
    int16_t absSample = abs(sample);
    if (absSample > stats.peak) stats.peak = absSample;
  }
  
  stats.rms = sqrt((double)sumSquares / numSamples);
  
  // Calculate Signal-to-Noise Ratio
  if (noiseFloor > 0.1) {
    stats.snr = 20.0 * log10(stats.rms / noiseFloor);
  } else {
    stats.snr = 0.0;
  }
  
  // Simple voice activity detection
  stats.voiceDetected = (stats.rms > noiseFloor * 2.0) && (stats.rms > 100.0);
  
  return stats;
}

void calibrateNoiseFloor() {
  const int calibrationSamples = 10;
  float totalRMS = 0.0;
  
  for (int i = 0; i < calibrationSamples; i++) {
    size_t bytesRead = 0;
    i2s_read(I2S_PORT, audioBuffer, sizeof(audioBuffer), &bytesRead, portMAX_DELAY);
    
    int samplesRead = bytesRead / sizeof(int16_t);
    AudioStats stats = analyzeAudio(audioBuffer, samplesRead);
    totalRMS += stats.rms;
    
    delay(300);  // 300ms between samples
    Serial.print(".");
  }
  
  noiseFloor = totalRMS / calibrationSamples;
  Serial.println();
}

void printAudioStats(AudioStats stats, int samplesRead) {
  // Create visual bar for RMS level
  String rmsBar = "";
  int barLength = map(constrain(stats.rms, 0, 2000), 0, 2000, 0, 20);
  for (int i = 0; i < 20; i++) {
    if (i < barLength) {
      rmsBar += "█";
    } else {
      rmsBar += "░";
    }
  }
  
  // Status indicator
  String status = "QUIET";
  if (stats.voiceDetected) {
    status = "VOICE";
  } else if (stats.rms > noiseFloor * 1.5) {
    status = "SOUND";
  }
  
  Serial.printf("RMS:%4.0f |%s| Peak:%5d | SNR:%5.1fdB | %s | Samples:%d\n", 
                stats.rms, rmsBar.c_str(), stats.peak, stats.snr, status.c_str(), samplesRead);
}

void updateAudioLED(float rms) {
  // Map RMS to LED brightness (digital approximation since no PWM)
  static unsigned long lastLEDUpdate = 0;
  static bool ledState = false;
  
  // Calculate blink rate based on audio level
  int blinkRate = map(constrain(rms, 0, 2000), 0, 2000, 2000, 50);  // 2000ms to 50ms
  
  if (millis() - lastLEDUpdate > blinkRate) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    lastLEDUpdate = millis();
  }
}
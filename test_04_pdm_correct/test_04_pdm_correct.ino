/*
 * PDM Microphone Test - XIAO ESP32S3 Sense (Working Version)
 * 
 * Uses Arduino I2S library instead of ESP-IDF direct calls
 * Compatible with ESP32 Arduino Core 2.0.x and 3.0.x
 */

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  #include "ESP_I2S.h"
  I2SClass I2S;
#else
  #include <I2S.h>
#endif

#define I2S_CLK_PIN 42    // PDM Clock
#define I2S_DIN_PIN 41    // PDM Data 
#define LED_PIN LED_BUILTIN
#define SAMPLE_RATE 16000
#define SAMPLE_BITS 16

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("=== PDM Microphone Test (XIAO ESP32S3 Sense) ===");
  Serial.println("Using Arduino I2S library...");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  Serial.printf("PDM Pins - CLK:%d, DIN:%d\n", I2S_CLK_PIN, I2S_DIN_PIN);
  Serial.printf("Sample Rate: %d Hz, Bits: %d\n", SAMPLE_RATE, SAMPLE_BITS);
  
  // Configure PDM pins
  Serial.println("Step 1: Setting PDM pins...");
  
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  // ESP32 Arduino Core 3.0.x
  I2S.setPinsPdmRx(I2S_CLK_PIN, I2S_DIN_PIN);
  
  if (!I2S.begin(I2S_MODE_PDM_RX, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println("❌ Failed to initialize I2S PDM!");
    while(1) { 
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(200);
    }
  }
#else
  // ESP32 Arduino Core 2.0.x
  I2S.setPinsPdmRx(I2S_CLK_PIN, I2S_DIN_PIN);
  
  if (!I2S.begin(I2S_MODE_PDM_RX, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println("❌ Failed to initialize I2S PDM!");
    while(1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(200);
    }
  }
#endif
  
  Serial.println("✅ PDM microphone initialized successfully");
  
  digitalWrite(LED_PIN, LOW);
  Serial.println("PDM microphone ready! Try making some noise...");
  Serial.println("Monitoring audio levels (try whistling, talking, clapping):");
}

void loop() {
  static int sample_count = 0;
  static int32_t sum = 0;
  static int16_t max_val = 0;
  static int16_t min_val = 32767;
  static int zero_count = 0;
  
  // Read audio sample
  int sample = I2S.read();
  
  // Skip invalid samples
  if (sample == 0 || sample == -1 || sample == 1) {
    return;
  }
  
  // Convert to signed 16-bit
  int16_t audio_sample = (int16_t)sample;
  
  // Accumulate statistics
  sum += abs(audio_sample);
  if (abs(audio_sample) > max_val) max_val = abs(audio_sample);
  if (audio_sample < min_val) min_val = audio_sample;
  if (audio_sample == 0) zero_count++;
  sample_count++;
  
  // Print stats every 1000 samples (~62ms at 16kHz)
  if (sample_count >= 1000) {
    float avg_level = (float)sum / sample_count;
    
    Serial.printf("PDM | Samples:%d | Avg:%.1f | Max:%d | Min:%d | Zeros:%d | Raw:%d\n", 
                  sample_count, avg_level, max_val, min_val, zero_count, audio_sample);
    
    // LED indicates audio activity (threshold may need adjustment)
    digitalWrite(LED_PIN, avg_level > 100 ? HIGH : LOW);
    
    // Show some sample values for debugging
    if (sample_count > 0) {
      Serial.printf("Recent samples: ");
      for (int i = 0; i < 5; i++) {
        int recent_sample = I2S.read();
        if (recent_sample != 0 && recent_sample != -1 && recent_sample != 1) {
          Serial.printf("%d ", recent_sample);
        }
      }
      Serial.println();
    }
    
    // Reset counters
    sample_count = 0;
    sum = 0;
    max_val = 0;
    min_val = 32767;
    zero_count = 0;
  }
  
  delay(1); // Small delay to prevent overwhelming the serial output
}
/*
 * I2S Microphone Debug Test - Simplified
 * 
 * Basic test to debug I2S initialization issues
 */

#include <driver/i2s.h>

#define I2S_WS_PIN 42
#define I2S_SCK_PIN 41  
#define I2S_SD_PIN 2
#define I2S_PORT I2S_NUM_0
#define LED_PIN LED_BUILTIN

void setup() {
  Serial.begin(115200);
  delay(3000);  // Longer delay to ensure serial is ready
  
  Serial.println("=== I2S Debug Test ===");
  Serial.println("Starting I2S microphone debug...");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  Serial.printf("I2S Pins - WS:%d, SCK:%d, SD:%d\n", I2S_WS_PIN, I2S_SCK_PIN, I2S_SD_PIN);
  
  // Test basic I2S driver install
  Serial.println("Step 1: Installing I2S driver...");
  
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  
  esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("❌ I2S driver install FAILED: %d\n", err);
    Serial.println("Error codes:");
    Serial.println("ESP_ERR_INVALID_ARG = 0x102");
    Serial.println("ESP_ERR_NO_MEM = 0x101");
    while(1) { 
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(200);
    }
  }
  Serial.println("✅ I2S driver installed successfully");
  
  // Test pin configuration
  Serial.println("Step 2: Configuring I2S pins...");
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD_PIN
  };
  
  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("❌ I2S pin config FAILED: %d\n", err);
    while(1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(100);
    }
  }
  Serial.println("✅ I2S pins configured successfully");
  
  // Clear buffer
  Serial.println("Step 3: Clearing I2S buffer...");
  i2s_zero_dma_buffer(I2S_PORT);
  Serial.println("✅ I2S buffer cleared");
  
  Serial.println("Step 4: Testing basic I2S read...");
  
  digitalWrite(LED_PIN, LOW);
  Serial.println("I2S initialization complete! Starting audio test...");
}

void loop() {
  static int16_t samples[512];
  size_t bytes_read = 0;
  
  // Read audio data
  esp_err_t result = i2s_read(I2S_PORT, samples, sizeof(samples), &bytes_read, 100);
  
  if (result != ESP_OK) {
    Serial.printf("I2S read error: %d\n", result);
    delay(1000);
    return;
  }
  
  // Calculate simple statistics
  int samples_read = bytes_read / sizeof(int16_t);
  int32_t sum = 0;
  int16_t max_val = 0;
  int zero_count = 0;
  
  for (int i = 0; i < samples_read; i++) {
    sum += abs(samples[i]);
    if (abs(samples[i]) > max_val) max_val = abs(samples[i]);
    if (samples[i] == 0) zero_count++;
  }
  
  float avg_level = (float)sum / samples_read;
  
  // Print stats every 1 second
  static unsigned long last_print = 0;
  if (millis() - last_print > 1000) {
    Serial.printf("Samples:%d | Avg:%.1f | Max:%d | Zeros:%d | Bytes:%d\n", 
                  samples_read, avg_level, max_val, zero_count, bytes_read);
    
    // LED blinks faster with more audio
    digitalWrite(LED_PIN, avg_level > 10 ? HIGH : LOW);
    
    last_print = millis();
  }
  
  delay(50);
}
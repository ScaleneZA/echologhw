  /*
 * WAV Recording Test - XIAO ESP32S3 Sense
 * 
 * Tests PDM microphone recording to WAV files on SD card
 * Combines working PDM microphone with WAV file format
 */

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  #include "ESP_I2S.h"
  I2SClass I2S;
#else
  #include <I2S.h>
#endif

#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Hardware pins
#define I2S_CLK_PIN 42    // PDM Clock
#define I2S_DIN_PIN 41    // PDM Data
#define LED_PIN LED_BUILTIN

// SD Card pins (confirmed working from test_03_sd_correct)
#define SD_CS_PIN 21
#define SD_MOSI_PIN 9
#define SD_MISO_PIN 8
#define SD_SCK_PIN 7

// Audio settings
#define SAMPLE_RATE 16000
#define BITS_PER_SAMPLE 16
#define CHANNELS 1
#define RECORDING_DURATION_SEC 5  // Record for 5 seconds

// WAV header structure
typedef struct {
  char chunkID[4];
  uint32_t chunkSize;
  char format[4];
  char subchunk1ID[4];
  uint32_t subchunk1Size;
  uint16_t audioFormat;
  uint16_t numChannels;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;
  char subchunk2ID[4];
  uint32_t subchunk2Size;
} wav_header_t;

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("=== WAV Recording Test (XIAO ESP32S3 Sense) ===");
  Serial.println("Testing PDM microphone recording to WAV files...");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  // Initialize SD card
  Serial.println("Step 1: Initializing SD card...");
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("❌ SD card initialization failed!");
    errorBlink();
    return;
  }
  Serial.println("✅ SD card initialized successfully");
  
  // Create recordings directory
  if (!SD.exists("/recordings")) {
    if (SD.mkdir("/recordings")) {
      Serial.println("✅ Created /recordings directory");
    } else {
      Serial.println("⚠️ Failed to create /recordings directory");
    }
  }
  
  // Initialize PDM microphone
  Serial.println("Step 2: Initializing PDM microphone...");
  I2S.setPinsPdmRx(I2S_CLK_PIN, I2S_DIN_PIN);
  
  if (!I2S.begin(I2S_MODE_PDM_RX, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println("❌ Failed to initialize PDM microphone!");
    errorBlink();
    return;
  }
  Serial.println("✅ PDM microphone initialized successfully");
  
  digitalWrite(LED_PIN, LOW);
  Serial.println();
  Serial.println("Setup complete! Starting WAV recording test...");
  Serial.printf("Will record for %d seconds and save to SD card\n", RECORDING_DURATION_SEC);
  Serial.println("Make some noise during recording!");
  delay(2000);
  
  // Perform the recording test
  recordWAVTest();
}

void loop() {
  // Test complete, just blink LED
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  delay(1000);
}

void recordWAVTest() {
  String filename = "/recordings/test_" + String(millis()) + ".wav";
  
  Serial.printf("Starting recording: %s\n", filename.c_str());
  Serial.println("Recording... Make some noise!");
  
  digitalWrite(LED_PIN, HIGH); // LED on during recording
  
  // Open file for writing
  File wavFile = SD.open(filename, FILE_WRITE);
  if (!wavFile) {
    Serial.println("❌ Failed to create WAV file!");
    return;
  }
  
  // Calculate expected data size
  uint32_t totalSamples = SAMPLE_RATE * RECORDING_DURATION_SEC;
  uint32_t dataSize = totalSamples * (BITS_PER_SAMPLE / 8);
  
  // Write WAV header
  wav_header_t header = createWAVHeader(dataSize);
  size_t headerWritten = wavFile.write((uint8_t*)&header, sizeof(header));
  if (headerWritten != sizeof(header)) {
    Serial.println("❌ Failed to write WAV header!");
    wavFile.close();
    return;
  }
  
  Serial.println("✅ WAV header written");
  
  // Record audio data
  unsigned long startTime = millis();
  uint32_t samplesRecorded = 0;
  uint32_t bytesWritten = 0;
  
  while (samplesRecorded < totalSamples) {
    int sample = I2S.read();
    
    // Skip invalid samples
    if (sample == 0 || sample == -1 || sample == 1) {
      continue;
    }
    
    // Write sample to file
    int16_t audioSample = (int16_t)sample;
    size_t written = wavFile.write((uint8_t*)&audioSample, sizeof(audioSample));
    if (written != sizeof(audioSample)) {
      Serial.println("❌ Failed to write audio data!");
      break;
    }
    
    samplesRecorded++;
    bytesWritten += written;
    
    // Progress indicator every second
    if (samplesRecorded % SAMPLE_RATE == 0) {
      int secondsRecorded = samplesRecorded / SAMPLE_RATE;
      Serial.printf("Progress: %d/%d seconds, Sample: %d\n", 
                    secondsRecorded, RECORDING_DURATION_SEC, audioSample);
    }
  }
  
  wavFile.close();
  digitalWrite(LED_PIN, LOW);
  
  unsigned long duration = millis() - startTime;
  
  // Verify the file was created and get its size
  File checkFile = SD.open(filename, FILE_READ);
  if (checkFile) {
    size_t fileSize = checkFile.size();
    checkFile.close();
    
    Serial.println();
    Serial.println("✅ Recording complete!");
    Serial.printf("File: %s\n", filename.c_str());
    Serial.printf("Duration: %lu ms\n", duration);
    Serial.printf("Samples recorded: %lu\n", samplesRecorded);
    Serial.printf("Data written: %lu bytes\n", bytesWritten);
    Serial.printf("File size: %d bytes\n", fileSize);
    Serial.printf("Expected size: %lu bytes\n", sizeof(header) + dataSize);
    
    // Calculate actual recording stats
    float actualDuration = (float)samplesRecorded / SAMPLE_RATE;
    Serial.printf("Actual duration: %.2f seconds\n", actualDuration);
    
  } else {
    Serial.println("❌ Failed to verify recorded file!");
  }
}

wav_header_t createWAVHeader(uint32_t dataSize) {
  wav_header_t header;
  
  // RIFF chunk descriptor
  memcpy(header.chunkID, "RIFF", 4);
  header.chunkSize = 36 + dataSize;
  memcpy(header.format, "WAVE", 4);
  
  // fmt sub-chunk
  memcpy(header.subchunk1ID, "fmt ", 4);
  header.subchunk1Size = 16;
  header.audioFormat = 1; // PCM
  header.numChannels = CHANNELS;
  header.sampleRate = SAMPLE_RATE;
  header.byteRate = SAMPLE_RATE * CHANNELS * (BITS_PER_SAMPLE / 8);
  header.blockAlign = CHANNELS * (BITS_PER_SAMPLE / 8);
  header.bitsPerSample = BITS_PER_SAMPLE;
  
  // data sub-chunk
  memcpy(header.subchunk2ID, "data", 4);
  header.subchunk2Size = dataSize;
  
  return header;
}

void errorBlink() {
  while (1) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}
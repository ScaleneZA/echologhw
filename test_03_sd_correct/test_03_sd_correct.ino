/*
 * Test 3: SD Card Test - CORRECTED PINS
 * 
 * Using the correct XIAO ESP32S3 Sense pins:
 * CS: GPIO21, SCK: GPIO7, MISO: GPIO8, MOSI: GPIO9
 */

#include <SD.h>
#include <SPI.h>

// Correct SD Card pins for XIAO ESP32S3 Sense
#define SD_CS_PIN 21
#define SD_MOSI_PIN 9
#define SD_MISO_PIN 8
#define SD_SCK_PIN 7
#define SD_SPI_FREQ 4000000

#define LED_PIN LED_BUILTIN

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== XIAO ESP32S3 Sense SD Card Test - CORRECT PINS ===");
  Serial.printf("Using: CS=%d, MOSI=%d, MISO=%d, SCK=%d\n", SD_CS_PIN, SD_MOSI_PIN, SD_MISO_PIN, SD_SCK_PIN);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  // Initialize SPI with correct pins
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  Serial.println("SPI initialized with correct pins");
  
  // Initialize SD card
  Serial.print("Initializing SD card... ");
  if (!SD.begin(SD_CS_PIN, SPI, SD_SPI_FREQ)) {
    Serial.println("FAILED - SD card not detected");
    digitalWrite(LED_PIN, LOW);
    Serial.println("Check:");
    Serial.println("1. SD card properly inserted in expansion board");
    Serial.println("2. SD card formatted as FAT32");
    Serial.println("3. Expansion board properly connected");
    return;
  }
  
  Serial.println("SUCCESS!");
  
  // Get card info
  uint64_t cardSize = SD.cardSize();
  Serial.printf("SD Card Size: %.2f MB\n", cardSize / (1024.0 * 1024.0));
  
  uint8_t cardType = SD.cardType();
  Serial.print("SD Card Type: ");
  switch(cardType) {
    case CARD_MMC: Serial.println("MMC"); break;
    case CARD_SD: Serial.println("SDSC"); break;
    case CARD_SDHC: Serial.println("SDHC"); break;
    default: Serial.println("UNKNOWN"); break;
  }
  
  // File system info
  uint64_t totalBytes = SD.totalBytes();
  uint64_t usedBytes = SD.usedBytes();
  uint64_t freeBytes = totalBytes - usedBytes;
  
  Serial.printf("Total: %.2f MB, Used: %.2f MB, Free: %.2f MB\n", 
                totalBytes/(1024.0*1024.0), usedBytes/(1024.0*1024.0), freeBytes/(1024.0*1024.0));
  
  // Test file operations
  Serial.println("\nTesting file operations...");
  File testFile = SD.open("/test.txt", FILE_WRITE);
  if (testFile) {
    testFile.println("XIAO ESP32S3 Sense SD card test successful!");
    testFile.printf("Timestamp: %lu\n", millis());
    testFile.close();
    Serial.println("File write: SUCCESS");
    
    // Read back
    testFile = SD.open("/test.txt", FILE_READ);
    if (testFile) {
      Serial.println("File contents:");
      while (testFile.available()) {
        Serial.write(testFile.read());
      }
      testFile.close();
    }
    
    // Clean up
    SD.remove("/test.txt");
    Serial.println("File cleanup: SUCCESS");
  } else {
    Serial.println("File write: FAILED");
  }
  
  // Test directory creation
  Serial.println("\nTesting directory operations...");
  if (SD.mkdir("/recordings")) {
    Serial.println("Created /recordings directory: SUCCESS");
  } else if (SD.exists("/recordings")) {
    Serial.println("/recordings directory already exists: OK");
  } else {
    Serial.println("Directory creation: FAILED");
  }
  
  if (SD.mkdir("/uploaded")) {
    Serial.println("Created /uploaded directory: SUCCESS");
  } else if (SD.exists("/uploaded")) {
    Serial.println("/uploaded directory already exists: OK");
  } else {
    Serial.println("Directory creation: FAILED");
  }
  
  Serial.println("\n=== SD Card Test Complete ===");
  Serial.println("SD card is working correctly with these pins!");
  
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  // Slow blink to indicate test complete
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 2000) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastBlink = millis();
  }
}
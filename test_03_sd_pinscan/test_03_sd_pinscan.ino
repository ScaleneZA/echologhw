/*
 * SD Card Pin Scanner for XIAO ESP32S3 Sense
 * 
 * This test tries different pin combinations to find the correct
 * SD card pins for the XIAO ESP32S3 Sense expansion board.
 */

#include <SD.h>
#include <SPI.h>

// Common pin configurations for ESP32 boards
struct PinConfig {
  const char* name;
  int cs;
  int mosi;
  int miso; 
  int sck;
};

PinConfig pinConfigs[] = {
  {"Config 1 (Current)", 10, 11, 13, 12},
  {"Config 3 (HSPI)", 15, 13, 12, 14},
  {"Config 4 (Alt 1)", 4, 7, 9, 8},
  {"Config 5 (Alt 2)", 21, 7, 9, 8},
  {"Config 6 (XIAO Alt)", 2, 10, 9, 8},
  {"Config 7 (XIAO Alt2)", 3, 4, 5, 6},
  {"Config 8 (XIAO Alt3)", 1, 2, 3, 4}
};

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== XIAO ESP32S3 Sense SD Card Pin Scanner ===");
  Serial.println("This will test different pin configurations to find the correct SD card pins");
  Serial.println("Make sure SD card is inserted in expansion board\n");
  
  int numConfigs = sizeof(pinConfigs) / sizeof(pinConfigs[0]);
  
  for (int i = 0; i < numConfigs; i++) {
    Serial.printf("Testing %s: CS=%d, MOSI=%d, MISO=%d, SCK=%d\n", 
                  pinConfigs[i].name,
                  pinConfigs[i].cs,
                  pinConfigs[i].mosi,
                  pinConfigs[i].miso,
                  pinConfigs[i].sck);
    
    // End any previous SPI
    SPI.end();
    delay(100);
    
    // Initialize with new pins
    SPI.begin(pinConfigs[i].sck, pinConfigs[i].miso, pinConfigs[i].mosi, pinConfigs[i].cs);
    
    // Try to initialize SD card
    if (SD.begin(pinConfigs[i].cs, SPI, 1000000)) {
      Serial.println("*** SUCCESS! Found working pin configuration! ***");
      
      // Get card info
      uint64_t cardSize = SD.cardSize();
      Serial.printf("SD Card Size: %.2f MB\n", cardSize / (1024.0 * 1024.0));
      
      uint8_t cardType = SD.cardType();
      Serial.print("SD Card Type: ");
      switch(cardType) {
        case CARD_MMC:
          Serial.println("MMC");
          break;
        case CARD_SD:
          Serial.println("SDSC");
          break;
        case CARD_SDHC:
          Serial.println("SDHC");
          break;
        default:
          Serial.println("UNKNOWN");
          break;
      }
      
      // Test basic file operation
      File testFile = SD.open("/pintest.txt", FILE_WRITE);
      if (testFile) {
        testFile.println("Pin test successful!");
        testFile.close();
        Serial.println("File write test: SUCCESS");
        
        // Read it back
        testFile = SD.open("/pintest.txt", FILE_READ);
        if (testFile) {
          String content = testFile.readString();
          testFile.close();
          Serial.printf("File content: %s", content.c_str());
        }
        
        // Clean up
        SD.remove("/pintest.txt");
      }
      
      Serial.println("\n*** Use these pins in your config.h: ***");
      Serial.printf("#define SD_CS_PIN %d\n", pinConfigs[i].cs);
      Serial.printf("#define SD_MOSI_PIN %d\n", pinConfigs[i].mosi);
      Serial.printf("#define SD_MISO_PIN %d\n", pinConfigs[i].miso);
      Serial.printf("#define SD_SCK_PIN %d\n", pinConfigs[i].sck);
      Serial.println("***************************************\n");
      
      SD.end();
      break;
    } else {
      Serial.println("FAILED");
    }
    
    delay(500);
  }
  
  Serial.println("\nPin scan complete.");
  Serial.println("If no configuration worked:");
  Serial.println("1. Check SD card is properly inserted");
  Serial.println("2. Try formatting SD card as FAT32");
  Serial.println("3. Try a different SD card");
  Serial.println("4. Check expansion board documentation for correct pins");
}

void loop() {
  // Blink LED to show we're alive
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    lastBlink = millis();
  }
}
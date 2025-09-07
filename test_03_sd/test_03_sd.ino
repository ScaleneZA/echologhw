/*
 * Test 3: SD Card Test
 * 
 * This test verifies SD card initialization, file operations, and directory management.
 * Critical for storing audio recordings and managing file organization.
 * 
 * Expected behavior:
 * 1. Initialize SD card and SPI interface
 * 2. Test basic file operations (create, write, read, delete)
 * 3. Test directory operations (create directories, list contents)
 * 4. Test file system info (total/free space)
 * 5. Create the directory structure used by main firmware
 * 6. Test large file operations (simulate audio files)
 * 
 * SUCCESS CRITERIA:
 * - SD card initializes successfully
 * - Can create/read/write/delete files
 * - Directory operations work correctly
 * - File system reports reasonable space amounts
 * - No file corruption during operations
 * - Directory structure matches expected layout
 */

#include <SD.h>
#include <SPI.h>

// SD Card pin definitions from config.h
#define SD_CS_PIN 10
#define SD_MOSI_PIN 11
#define SD_MISO_PIN 13
#define SD_SCK_PIN 12
#define SD_SPI_FREQ 4000000

// Directory structure from config.h
#define RECORDINGS_DIR "/recordings"
#define UPLOADED_DIR "/uploaded"

// LED for status indication
#define LED_PIN LED_BUILTIN

// Test files
#define TEST_FILE "/test.txt"
#define TEST_LARGE_FILE "/large_test.bin"
#define TEST_AUDIO_FILE "/recordings/2025-08-31/test_recording.wav"

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== SD Card Test ===");
  Serial.println("Insert a microSD card and ensure it's formatted as FAT32");
  Serial.println("This test will create/delete files on the card\n");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // LED on during testing
  
  // Test SD card initialization
  Serial.println("Phase 1: SD Card Initialization");
  Serial.println("================================");
  testSDInitialization();
  delay(1000);
  
  Serial.println("\nPhase 2: Basic File Operations");
  Serial.println("==============================");
  testBasicFileOperations();
  delay(1000);
  
  Serial.println("\nPhase 3: Directory Operations");
  Serial.println("=============================");
  testDirectoryOperations();
  delay(1000);
  
  Serial.println("\nPhase 4: File System Information");
  Serial.println("================================");
  testFileSystemInfo();
  delay(1000);
  
  Serial.println("\nPhase 5: Audio Directory Structure");
  Serial.println("==================================");
  testAudioDirectoryStructure();
  delay(1000);
  
  Serial.println("\nPhase 6: Large File Operations");
  Serial.println("==============================");
  testLargeFileOperations();
  delay(1000);
  
  Serial.println("\n=== SD Card Test Complete ===");
  Serial.println("Check results above for any FAILED tests");
  
  digitalWrite(LED_PIN, LOW);  // LED off when complete
}

void loop() {
  // Blink LED to show system is running
  static unsigned long ledTime = 0;
  if (millis() - ledTime > 2000) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    ledTime = millis();
  }
  
  delay(100);
}

void testSDInitialization() {
  Serial.printf("CS Pin: %d, MOSI: %d, MISO: %d, SCK: %d\n", SD_CS_PIN, SD_MOSI_PIN, SD_MISO_PIN, SD_SCK_PIN);
  Serial.printf("SPI Frequency: %d Hz\n", SD_SPI_FREQ);
  
  // Test CS pin first
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  delay(10);
  digitalWrite(SD_CS_PIN, LOW);
  delay(10);
  digitalWrite(SD_CS_PIN, HIGH);
  Serial.println("CS pin toggle test completed");
  
  // Initialize SPI with custom pins
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  Serial.println("SPI initialized with custom pins");
  
  // Try different initialization methods
  Serial.print("Method 1 - SD.begin() with custom pins... ");
  if (!SD.begin(SD_CS_PIN, SPI, SD_SPI_FREQ)) {
    Serial.println("FAILED");
    
    Serial.print("Method 2 - SD.begin() with lower frequency... ");
    if (!SD.begin(SD_CS_PIN, SPI, 1000000)) {  // 1MHz
      Serial.println("FAILED");
      
      Serial.print("Method 3 - SD.begin() default frequency... ");
      if (!SD.begin(SD_CS_PIN)) {
        Serial.println("FAILED");
        Serial.println("Troubleshooting steps:");
        Serial.println("1. Ensure SD card is properly inserted in expansion board");
        Serial.println("2. Try reformatting SD card as FAT32");
        Serial.println("3. Try a different SD card (Class 10 recommended)");
        Serial.println("4. Check expansion board is properly connected");
        Serial.println("5. Verify pin connections match expansion board schematic");
        return;
      } else {
        Serial.println("SUCCESS (default frequency)");
      }
    } else {
      Serial.println("SUCCESS (1MHz frequency)");
    }
  } else {
    Serial.println("SUCCESS (4MHz frequency)");
  }
  
  Serial.println("SUCCESS");
  
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
}

void testBasicFileOperations() {
  Serial.println("Testing file create/write...");
  
  // Create and write test file
  File testFile = SD.open(TEST_FILE, FILE_WRITE);
  if (!testFile) {
    Serial.println("FAILED - Could not create test file");
    return;
  }
  
  String testData = "Hello from XIAO ESP32S3 Sense!\n";
  testData += "Timestamp: ";
  testData += millis();
  testData += "\n";
  testData += "This is a test file for SD card verification.";
  
  int bytesWritten = testFile.print(testData);
  testFile.close();
  
  Serial.printf("Wrote %d bytes to %s\n", bytesWritten, TEST_FILE);
  
  // Read back and verify
  Serial.println("Testing file read...");
  testFile = SD.open(TEST_FILE, FILE_READ);
  if (!testFile) {
    Serial.println("FAILED - Could not read test file");
    return;
  }
  
  String readData = testFile.readString();
  testFile.close();
  
  if (readData == testData) {
    Serial.println("SUCCESS - File read/write verification passed");
  } else {
    Serial.println("FAILED - File content mismatch");
    Serial.println("Expected:");
    Serial.println(testData);
    Serial.println("Got:");
    Serial.println(readData);
  }
  
  // Test file deletion
  Serial.println("Testing file deletion...");
  if (SD.remove(TEST_FILE)) {
    Serial.println("SUCCESS - File deleted");
  } else {
    Serial.println("FAILED - Could not delete file");
  }
}

void testDirectoryOperations() {
  Serial.println("Testing directory creation...");
  
  // Create test directory
  if (SD.mkdir("/testdir")) {
    Serial.println("SUCCESS - Directory created");
  } else {
    Serial.println("FAILED - Could not create directory (may already exist)");
  }
  
  // Create file in directory
  File dirFile = SD.open("/testdir/subtest.txt", FILE_WRITE);
  if (dirFile) {
    dirFile.println("File in subdirectory");
    dirFile.close();
    Serial.println("SUCCESS - File created in directory");
  } else {
    Serial.println("FAILED - Could not create file in directory");
  }
  
  // List directory contents
  Serial.println("Directory listing:");
  File root = SD.open("/");
  if (root) {
    File entry = root.openNextFile();
    while (entry) {
      if (entry.isDirectory()) {
        Serial.printf("  DIR:  %s\n", entry.name());
      } else {
        Serial.printf("  FILE: %s (%d bytes)\n", entry.name(), entry.size());
      }
      entry.close();
      entry = root.openNextFile();
    }
    root.close();
  }
  
  // Cleanup
  SD.remove("/testdir/subtest.txt");
  SD.rmdir("/testdir");
}

void testFileSystemInfo() {
  uint64_t totalBytes = SD.totalBytes();
  uint64_t usedBytes = SD.usedBytes();
  uint64_t freeBytes = totalBytes - usedBytes;
  
  Serial.printf("Total space: %.2f MB\n", totalBytes / (1024.0 * 1024.0));
  Serial.printf("Used space:  %.2f MB\n", usedBytes / (1024.0 * 1024.0));
  Serial.printf("Free space:  %.2f MB\n", freeBytes / (1024.0 * 1024.0));
  Serial.printf("Usage: %.1f%%\n", (usedBytes * 100.0) / totalBytes);
  
  if (freeBytes < 10 * 1024 * 1024) {  // Less than 10MB free
    Serial.println("WARNING - Low disk space, may affect recording");
  } else {
    Serial.println("SUCCESS - Adequate free space available");
  }
}

void testAudioDirectoryStructure() {
  Serial.println("Creating audio recording directory structure...");
  
  // Create main directories
  bool recordingsOK = SD.mkdir(RECORDINGS_DIR);
  bool uploadedOK = SD.mkdir(UPLOADED_DIR);
  
  if (!recordingsOK && !SD.exists(RECORDINGS_DIR)) {
    Serial.println("FAILED - Could not create recordings directory");
    return;
  }
  
  if (!uploadedOK && !SD.exists(UPLOADED_DIR)) {
    Serial.println("FAILED - Could not create uploaded directory");
    return;
  }
  
  Serial.println("SUCCESS - Main directories created");
  
  // Create date-based subdirectory (like main firmware does)
  String dateDir = String(RECORDINGS_DIR) + "/2025-08-31";
  if (SD.mkdir(dateDir.c_str())) {
    Serial.printf("SUCCESS - Date directory created: %s\n", dateDir.c_str());
  } else if (SD.exists(dateDir.c_str())) {
    Serial.printf("INFO - Date directory already exists: %s\n", dateDir.c_str());
  } else {
    Serial.println("FAILED - Could not create date directory");
    return;
  }
  
  // Test creating a mock audio file
  File audioFile = SD.open(TEST_AUDIO_FILE, FILE_WRITE);
  if (audioFile) {
    // Write mock WAV header (simplified)
    audioFile.write((const uint8_t*)"RIFF", 4);
    audioFile.write((const uint8_t*)"WAVEfmt ", 8);
    audioFile.println("Mock audio file for testing");
    audioFile.close();
    Serial.printf("SUCCESS - Mock audio file created: %s\n", TEST_AUDIO_FILE);
  } else {
    Serial.println("FAILED - Could not create mock audio file");
  }
  
  // Verify directory structure
  Serial.println("Directory structure verification:");
  listDirectory(RECORDINGS_DIR, 2);
  listDirectory(UPLOADED_DIR, 1);
}

void testLargeFileOperations() {
  Serial.println("Testing large file operations (simulating audio files)...");
  
  const int bufferSize = 1024;
  const int totalSize = 50 * 1024;  // 50KB test file
  uint8_t buffer[bufferSize];
  
  // Fill buffer with test pattern
  for (int i = 0; i < bufferSize; i++) {
    buffer[i] = i % 256;
  }
  
  Serial.printf("Creating %d byte test file...\n", totalSize);
  unsigned long startTime = millis();
  
  File largeFile = SD.open(TEST_LARGE_FILE, FILE_WRITE);
  if (!largeFile) {
    Serial.println("FAILED - Could not create large test file");
    return;
  }
  
  int bytesWritten = 0;
  for (int chunk = 0; chunk < totalSize / bufferSize; chunk++) {
    int written = largeFile.write(buffer, bufferSize);
    bytesWritten += written;
    if (written != bufferSize) {
      Serial.printf("WARNING - Write incomplete: %d/%d bytes\n", written, bufferSize);
    }
  }
  
  largeFile.close();
  unsigned long writeTime = millis() - startTime;
  
  Serial.printf("Write complete: %d bytes in %lu ms\n", bytesWritten, writeTime);
  Serial.printf("Write speed: %.2f KB/s\n", (bytesWritten / 1024.0) / (writeTime / 1000.0));
  
  // Read back and verify
  startTime = millis();
  largeFile = SD.open(TEST_LARGE_FILE, FILE_READ);
  if (!largeFile) {
    Serial.println("FAILED - Could not read large test file");
    return;
  }
  
  int errors = 0;
  uint8_t readBuffer[bufferSize];
  for (int chunk = 0; chunk < totalSize / bufferSize; chunk++) {
    int bytesRead = largeFile.read(readBuffer, bufferSize);
    if (bytesRead != bufferSize) {
      Serial.printf("WARNING - Read incomplete: %d/%d bytes\n", bytesRead, bufferSize);
    }
    
    // Verify data
    for (int i = 0; i < bytesRead; i++) {
      if (readBuffer[i] != (i % 256)) {
        errors++;
      }
    }
  }
  
  largeFile.close();
  unsigned long readTime = millis() - startTime;
  
  Serial.printf("Read complete in %lu ms\n", readTime);
  Serial.printf("Read speed: %.2f KB/s\n", (totalSize / 1024.0) / (readTime / 1000.0));
  
  if (errors == 0) {
    Serial.println("SUCCESS - Large file integrity verified");
  } else {
    Serial.printf("FAILED - %d data errors found\n", errors);
  }
  
  // Cleanup
  SD.remove(TEST_LARGE_FILE);
  SD.remove(TEST_AUDIO_FILE);
}

void listDirectory(const char* dirname, int maxLevels) {
  File root = SD.open(dirname);
  if (!root || !root.isDirectory()) {
    Serial.printf("Failed to open directory: %s\n", dirname);
    return;
  }
  
  Serial.printf("Contents of %s:\n", dirname);
  File entry = root.openNextFile();
  while (entry) {
    if (entry.isDirectory()) {
      Serial.printf("  DIR:  %s/\n", entry.name());
      if (maxLevels > 1) {
        String subPath = String(dirname) + "/" + entry.name();
        listDirectory(subPath.c_str(), maxLevels - 1);
      }
    } else {
      Serial.printf("  FILE: %s (%d bytes)\n", entry.name(), entry.size());
    }
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
}
#include "sd_manager.h"
#include "config.h"
#include <time.h>

static bool sdInitialized = false;

bool initializeSDCard() {
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  
  if (!SD.begin(SD_CS_PIN, SPI, SD_SPI_FREQ)) {
    DEBUG_PRINTLN("SD card initialization failed");
    return false;
  }
  
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    DEBUG_PRINTLN("No SD card attached");
    return false;
  }
  
  DEBUG_PRINT("SD Card Type: ");
  if (cardType == CARD_MMC) {
    DEBUG_PRINTLN("MMC");
  } else if (cardType == CARD_SD) {
    DEBUG_PRINTLN("SDSC");
  } else if (cardType == CARD_SDHC) {
    DEBUG_PRINTLN("SDHC");
  } else {
    DEBUG_PRINTLN("UNKNOWN");
  }
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  DEBUG_PRINTF("SD Card Size: %llu MB\n", cardSize);
  
  if (!createDirectoryStructure()) {
    DEBUG_PRINTLN("Failed to create directory structure");
    return false;
  }
  
  sdInitialized = true;
  DEBUG_PRINTLN("SD card initialized successfully");
  return true;
}

bool createDirectoryStructure() {
  if (!SD.mkdir(RECORDINGS_DIR)) {
    if (!SD.exists(RECORDINGS_DIR)) {
      DEBUG_PRINTLN("Failed to create recordings directory");
      return false;
    }
  }
  
  if (!SD.mkdir(UPLOADED_DIR)) {
    if (!SD.exists(UPLOADED_DIR)) {
      DEBUG_PRINTLN("Failed to create uploaded directory");
      return false;
    }
  }
  
  time_t now;
  time(&now);
  struct tm* timeinfo = localtime(&now);
  
  char dateDir[32];
  snprintf(dateDir, sizeof(dateDir), "%s/%04d-%02d-%02d", 
           RECORDINGS_DIR, 
           timeinfo->tm_year + 1900, 
           timeinfo->tm_mon + 1, 
           timeinfo->tm_mday);
  
  if (!SD.mkdir(dateDir)) {
    if (!SD.exists(dateDir)) {
      DEBUG_PRINTF("Failed to create date directory: %s\n", dateDir);
    }
  }
  
  return true;
}

String generateRecordingFilename() {
  time_t now;
  time(&now);
  struct tm* timeinfo = localtime(&now);
  
  char filename[MAX_FILENAME_LENGTH];
  snprintf(filename, sizeof(filename), 
           "%s/%04d-%02d-%02d/REC_%04d%02d%02d_%02d%02d%02d.wav",
           RECORDINGS_DIR,
           timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
           timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  
  return String(filename);
}

bool markFileAsUploaded(const String& filename) {
  if (!sdInitialized) {
    return false;
  }
  
  String uploadedPath = filename;
  uploadedPath.replace(RECORDINGS_DIR, UPLOADED_DIR);
  
  String uploadedDir = uploadedPath.substring(0, uploadedPath.lastIndexOf('/'));
  
  if (!SD.exists(uploadedDir)) {
    if (!createDirectoryPath(uploadedDir)) {
      DEBUG_PRINTF("Failed to create uploaded directory: %s\n", uploadedDir.c_str());
      return false;
    }
  }
  
  if (SD.rename(filename, uploadedPath)) {
    DEBUG_PRINTF("File marked as uploaded: %s\n", uploadedPath.c_str());
    return true;
  } else {
    DEBUG_PRINTF("Failed to mark file as uploaded: %s\n", filename.c_str());
    return false;
  }
}

bool createDirectoryPath(const String& path) {
  String currentPath = "";
  int start = 0;
  int end = path.indexOf('/', start + 1);
  
  while (end != -1) {
    currentPath = path.substring(0, end);
    if (!SD.exists(currentPath)) {
      if (!SD.mkdir(currentPath)) {
        return false;
      }
    }
    start = end;
    end = path.indexOf('/', start + 1);
  }
  
  if (!SD.exists(path)) {
    return SD.mkdir(path);
  }
  
  return true;
}

int getUnuploadedFileCount() {
  if (!sdInitialized) {
    return 0;
  }
  
  return countFilesInDirectory(RECORDINGS_DIR);
}

int countFilesInDirectory(const String& dirPath) {
  File dir = SD.open(dirPath);
  if (!dir || !dir.isDirectory()) {
    return 0;
  }
  
  int count = 0;
  File file = dir.openNextFile();
  
  while (file) {
    if (!file.isDirectory()) {
      String filename = file.name();
      if (filename.endsWith(".wav")) {
        count++;
      }
    } else {
      String subdirPath = String(dirPath) + "/" + String(file.name());
      count += countFilesInDirectory(subdirPath);
    }
    file.close();
    file = dir.openNextFile();
  }
  
  dir.close();
  return count;
}

bool getUnuploadedFiles(String* files, int maxFiles) {
  if (!sdInitialized || !files) {
    return false;
  }
  
  int fileCount = 0;
  return getFilesFromDirectory(RECORDINGS_DIR, files, maxFiles, &fileCount);
}

bool getFilesFromDirectory(const String& dirPath, String* files, int maxFiles, int* fileCount) {
  File dir = SD.open(dirPath);
  if (!dir || !dir.isDirectory()) {
    return false;
  }
  
  File file = dir.openNextFile();
  
  while (file && *fileCount < maxFiles) {
    if (!file.isDirectory()) {
      String filename = file.name();
      if (filename.endsWith(".wav")) {
        files[*fileCount] = String(dirPath) + "/" + filename;
        (*fileCount)++;
      }
    } else {
      String subdirPath = String(dirPath) + "/" + String(file.name());
      getFilesFromDirectory(subdirPath, files, maxFiles, fileCount);
    }
    file.close();
    file = dir.openNextFile();
  }
  
  dir.close();
  return true;
}

bool isSDCardAvailable() {
  return sdInitialized && SD.cardType() != CARD_NONE;
}

uint64_t getSDCardFreeSpace() {
  if (!sdInitialized) {
    return 0;
  }
  
  return SD.totalBytes() - SD.usedBytes();
}

bool cleanupOldFiles() {
  if (!sdInitialized) {
    return false;
  }
  
  uint64_t freeSpace = getSDCardFreeSpace();
  uint64_t minFreeSpace = 100 * 1024 * 1024; // 100MB
  
  if (freeSpace > minFreeSpace) {
    return true;
  }
  
  DEBUG_PRINTLN("Low disk space, cleaning up old uploaded files");
  
  return deleteOldUploadedFiles();
}

bool createDirectoryPath(const String& path);
int countFilesInDirectory(const String& dirPath);
bool getFilesFromDirectory(const String& dirPath, String* files, int maxFiles, int* fileCount);
bool deleteOldUploadedFiles();

bool createDirectoryPath(const String& path) {
  String currentPath = "";
  int start = 0;
  int end = path.indexOf('/', start + 1);
  
  while (end != -1) {
    currentPath = path.substring(0, end);
    if (!SD.exists(currentPath)) {
      if (!SD.mkdir(currentPath)) {
        return false;
      }
    }
    start = end;
    end = path.indexOf('/', start + 1);
  }
  
  if (!SD.exists(path)) {
    return SD.mkdir(path);
  }
  
  return true;
}

int countFilesInDirectory(const String& dirPath) {
  File dir = SD.open(dirPath);
  if (!dir || !dir.isDirectory()) {
    return 0;
  }
  
  int count = 0;
  File file = dir.openNextFile();
  
  while (file) {
    if (!file.isDirectory()) {
      String filename = file.name();
      if (filename.endsWith(".wav")) {
        count++;
      }
    } else {
      String subdirPath = String(dirPath) + "/" + String(file.name());
      count += countFilesInDirectory(subdirPath);
    }
    file.close();
    file = dir.openNextFile();
  }
  
  dir.close();
  return count;
}

bool getFilesFromDirectory(const String& dirPath, String* files, int maxFiles, int* fileCount) {
  File dir = SD.open(dirPath);
  if (!dir || !dir.isDirectory()) {
    return false;
  }
  
  File file = dir.openNextFile();
  
  while (file && *fileCount < maxFiles) {
    if (!file.isDirectory()) {
      String filename = file.name();
      if (filename.endsWith(".wav")) {
        files[*fileCount] = String(dirPath) + "/" + filename;
        (*fileCount)++;
      }
    } else {
      String subdirPath = String(dirPath) + "/" + String(file.name());
      getFilesFromDirectory(subdirPath, files, maxFiles, fileCount);
    }
    file.close();
    file = dir.openNextFile();
  }
  
  dir.close();
  return true;
}

bool deleteOldUploadedFiles() {
  File dir = SD.open(UPLOADED_DIR);
  if (!dir || !dir.isDirectory()) {
    return false;
  }
  
  String oldestFile = "";
  time_t oldestTime = LONG_MAX;
  
  File file = dir.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      time_t fileTime = file.getLastWrite();
      if (fileTime < oldestTime) {
        oldestTime = fileTime;
        oldestFile = String(UPLOADED_DIR) + "/" + String(file.name());
      }
    }
    file.close();
    file = dir.openNextFile();
  }
  
  dir.close();
  
  if (oldestFile.length() > 0) {
    if (SD.remove(oldestFile)) {
      DEBUG_PRINTF("Deleted old file: %s\n", oldestFile.c_str());
      return true;
    }
  }
  
  return false;
}
#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include "config.h"
#include <SD.h>

bool initializeSDCard();
String generateRecordingFilename();
bool createDirectoryStructure();
bool markFileAsUploaded(const String& filename);
bool deleteUploadedFiles();
bool getUnuploadedFiles(String* files, int maxFiles);
int getUnuploadedFileCount();
bool isSDCardAvailable();
uint64_t getSDCardFreeSpace();
bool cleanupOldFiles();

#endif // SD_MANAGER_H
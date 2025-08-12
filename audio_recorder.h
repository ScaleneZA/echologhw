#ifndef AUDIO_RECORDER_H
#define AUDIO_RECORDER_H

#include "config.h"
#include <driver/i2s.h>
#include <SD.h>

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

bool initializeAudio();
bool startRecording(const String& filename);
bool continueRecording();
bool stopRecording();
bool isRecording();
uint32_t getRecordingDuration();

#endif // AUDIO_RECORDER_H
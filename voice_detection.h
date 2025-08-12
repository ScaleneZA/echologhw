#ifndef VOICE_DETECTION_H
#define VOICE_DETECTION_H

#include "config.h"
#include <driver/i2s.h>

void initializeVAD();
bool detectVoiceActivity();
void updateNoiseFloor();
float getAudioLevel();
void calibrateVAD();

#endif // VOICE_DETECTION_H
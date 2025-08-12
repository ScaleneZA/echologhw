#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "config.h"

typedef enum {
  LED_OFF,
  LED_LISTENING,
  LED_RECORDING,
  LED_UPLOADING,
  LED_LOW_BATTERY,
  LED_SOLID,
  LED_ERROR
} led_mode_t;

void initializeLED();
void setLEDMode(led_mode_t mode);
void updateLED();
void setLEDBrightness(uint8_t brightness);
void morseCodeError(uint8_t errorCode);

#endif // LED_CONTROL_H
#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

#include "config.h"
#include <esp_sleep.h>

bool initializePowerManagement();
bool isUSBConnected();
float getBatteryVoltage();
float getBatteryPercentage();
void enterLightSleep();
void enterDeepSleep();
void enableWakeOnVoice();
void disableWakeOnVoice();
void updatePowerStatus();

#endif // POWER_MANAGEMENT_H
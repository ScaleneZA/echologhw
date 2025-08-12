#include "power_management.h"
#include "config.h"
#include <driver/gpio.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

static bool powerInitialized = false;
static esp_adc_cal_characteristics_t* adcChars = NULL;
static float lastBatteryVoltage = 0.0;
static float lastBatteryPercentage = 0.0;
static unsigned long lastBatteryCheck = 0;

bool initializePowerManagement() {
  pinMode(USB_DETECT_PIN, INPUT_PULLUP);
  
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
  
  adcChars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, adcChars);
  
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_21, 0);
  
  updatePowerStatus();
  
  powerInitialized = true;
  DEBUG_PRINTLN("Power management initialized");
  return true;
}

bool isUSBConnected() {
  return digitalRead(USB_DETECT_PIN) == LOW;
}

float getBatteryVoltage() {
  if (millis() - lastBatteryCheck < 1000) {
    return lastBatteryVoltage;
  }
  
  uint32_t adcReading = 0;
  for (int i = 0; i < 10; i++) {
    adcReading += adc1_get_raw(ADC1_CHANNEL_0);
  }
  adcReading /= 10;
  
  uint32_t voltage = esp_adc_cal_raw_to_voltage(adcReading, adcChars);
  lastBatteryVoltage = (voltage / 1000.0) * BATTERY_VOLTAGE_DIVIDER;
  lastBatteryCheck = millis();
  
  return lastBatteryVoltage;
}

float getBatteryPercentage() {
  float voltage = getBatteryVoltage();
  
  float minVoltage = 3.2;
  float maxVoltage = 4.2;
  
  if (voltage >= maxVoltage) {
    lastBatteryPercentage = 100.0;
  } else if (voltage <= minVoltage) {
    lastBatteryPercentage = 0.0;
  } else {
    lastBatteryPercentage = ((voltage - minVoltage) / (maxVoltage - minVoltage)) * 100.0;
  }
  
  return lastBatteryPercentage;
}

void enterLightSleep() {
  DEBUG_PRINTLN("Entering light sleep mode");
  
  esp_sleep_enable_timer_wakeup(5 * 1000000ULL);
  
  esp_light_sleep_start();
  
  DEBUG_PRINTLN("Woke from light sleep");
}

void enterDeepSleep() {
  DEBUG_PRINTLN("Entering deep sleep mode");
  
  esp_sleep_enable_timer_wakeup(60 * 1000000ULL);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_21, 0);
  
  esp_deep_sleep_start();
}

void enableWakeOnVoice() {
  esp_sleep_enable_ext0_wakeup((gpio_num_t)I2S_WS_PIN, 1);
  DEBUG_PRINTLN("Wake on voice enabled");
}

void disableWakeOnVoice() {
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
  DEBUG_PRINTLN("Wake on voice disabled");
}

void updatePowerStatus() {
  float voltage = getBatteryVoltage();
  float percentage = getBatteryPercentage();
  bool usbConnected = isUSBConnected();
  
  if (DEBUG_ENABLED && millis() % 10000 == 0) {
    DEBUG_PRINTF("Power Status - Battery: %.2fV (%.1f%%), USB: %s\n", 
                 voltage, percentage, usbConnected ? "Connected" : "Disconnected");
  }
}
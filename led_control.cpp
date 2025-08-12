#include "led_control.h"
#include "config.h"

static led_mode_t currentMode = LED_OFF;
static unsigned long lastUpdate = 0;
static bool ledState = false;
static uint8_t brightness = LED_BRIGHTNESS;
static bool morseActive = false;
static uint8_t morseErrorCode = 0;
static int morseStep = 0;

void initializeLED() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  currentMode = LED_OFF;
  DEBUG_PRINTLN("LED control initialized");
}

void setLEDMode(led_mode_t mode) {
  if (mode != currentMode) {
    currentMode = mode;
    lastUpdate = 0;
    ledState = false;
    morseActive = false;
    DEBUG_PRINTF("LED mode changed to: %d\n", mode);
  }
}

void setLEDBrightness(uint8_t newBrightness) {
  brightness = newBrightness;
}

void updateLED() {
  unsigned long currentTime = millis();
  
  if (morseActive) {
    updateMorseCode(currentTime);
    return;
  }
  
  switch (currentMode) {
    case LED_OFF:
      digitalWrite(LED_PIN, LOW);
      break;
      
    case LED_SOLID:
      digitalWrite(LED_PIN, HIGH);
      break;
      
    case LED_LISTENING:
      updatePulsingLED(currentTime, LED_LISTENING_PERIOD);
      break;
      
    case LED_RECORDING:
      updateBlinkingLED(currentTime, LED_RECORDING_PERIOD);
      break;
      
    case LED_LOW_BATTERY:
      updateBlinkingLED(currentTime, LED_LOW_BATTERY_PERIOD);
      break;
      
    case LED_UPLOADING:
      updateDoubleFlashLED(currentTime);
      break;
      
    case LED_ERROR:
      if (morseErrorCode > 0) {
        startMorseCode(morseErrorCode);
      } else {
        updateBlinkingLED(currentTime, 100);
      }
      break;
  }
}

void updatePulsingLED(unsigned long currentTime, unsigned long period) {
  float phase = (float)(currentTime % period) / period;
  float intensity = (sin(phase * 2 * PI) + 1) / 2;
  
  if (currentTime - lastUpdate > 10) {
    int pwmValue = (int)(intensity * brightness);
    analogWrite(LED_PIN, pwmValue);
    lastUpdate = currentTime;
  }
}

void updateBlinkingLED(unsigned long currentTime, unsigned long period) {
  if (currentTime - lastUpdate >= period) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastUpdate = currentTime;
  }
}

void updateDoubleFlashLED(unsigned long currentTime) {
  unsigned long cycleTime = currentTime % 1000;
  
  bool shouldBeOn = (cycleTime < LED_UPLOADING_ON_TIME) || 
                    (cycleTime >= 200 && cycleTime < 200 + LED_UPLOADING_ON_TIME);
  
  if (currentTime - lastUpdate > 10) {
    digitalWrite(LED_PIN, shouldBeOn ? HIGH : LOW);
    lastUpdate = currentTime;
  }
}

void morseCodeError(uint8_t errorCode) {
  morseErrorCode = errorCode;
  setLEDMode(LED_ERROR);
}

void startMorseCode(uint8_t errorCode) {
  morseActive = true;
  morseStep = 0;
  lastUpdate = millis();
  
  DEBUG_PRINTF("Starting morse code for error: 0x%02X\n", errorCode);
}

void updatePulsingLED(unsigned long currentTime, unsigned long period);
void updateBlinkingLED(unsigned long currentTime, unsigned long period);
void updateDoubleFlashLED(unsigned long currentTime);
void updateMorseCode(unsigned long currentTime);
void startMorseCode(uint8_t errorCode);
String getMorsePattern(uint8_t errorCode);

void updatePulsingLED(unsigned long currentTime, unsigned long period) {
  float phase = (float)(currentTime % period) / period;
  float intensity = (sin(phase * 2 * PI) + 1) / 2;
  
  if (currentTime - lastUpdate > 10) {
    int pwmValue = (int)(intensity * brightness);
    analogWrite(LED_PIN, pwmValue);
    lastUpdate = currentTime;
  }
}

void updateBlinkingLED(unsigned long currentTime, unsigned long period) {
  if (currentTime - lastUpdate >= period) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastUpdate = currentTime;
  }
}

void updateDoubleFlashLED(unsigned long currentTime) {
  unsigned long cycleTime = currentTime % 1000;
  
  bool shouldBeOn = (cycleTime < LED_UPLOADING_ON_TIME) || 
                    (cycleTime >= 200 && cycleTime < 200 + LED_UPLOADING_ON_TIME);
  
  if (currentTime - lastUpdate > 10) {
    digitalWrite(LED_PIN, shouldBeOn ? HIGH : LOW);
    lastUpdate = currentTime;
  }
}

void updateMorseCode(unsigned long currentTime) {
  static bool inSymbol = false;
  static unsigned long symbolStartTime = 0;
  static String morsePattern = "";
  
  if (morsePattern.length() == 0) {
    morsePattern = getMorsePattern(morseErrorCode);
  }
  
  if (!inSymbol) {
    if (morseStep >= morsePattern.length()) {
      if (currentTime - lastUpdate > 2000) {
        morseStep = 0;
        lastUpdate = currentTime;
      }
      digitalWrite(LED_PIN, LOW);
      return;
    }
    
    inSymbol = true;
    symbolStartTime = currentTime;
    digitalWrite(LED_PIN, HIGH);
  } else {
    char symbol = morsePattern.charAt(morseStep);
    unsigned long duration = (symbol == '.') ? MORSE_DOT_DURATION : MORSE_DASH_DURATION;
    
    if (currentTime - symbolStartTime >= duration) {
      digitalWrite(LED_PIN, LOW);
      inSymbol = false;
      morseStep++;
      lastUpdate = currentTime;
    }
  }
}

String getMorsePattern(uint8_t errorCode) {
  switch (errorCode) {
    case ERROR_SD_INIT_FAILED:
      return "..."; // S
    case ERROR_AUDIO_INIT_FAILED:
      return ".-"; // A
    case ERROR_WIFI_CONNECT_FAILED:
      return ".--"; // W
    case ERROR_RECORDING_FAILED:
      return ".-."; // R
    case ERROR_UPLOAD_FAILED:
      return "..-"; // U
    case ERROR_LOW_BATTERY:
      return "-..."; // B
    case ERROR_SD_WRITE_FAILED:
      return "-.."; // D
    default:
      return "."; // E (generic error)
  }
}
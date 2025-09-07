# Hardware Testing Key Findings

This document tracks important discoveries and limitations found during hardware testing that affect the main firmware implementation.

## Test Results Summary

### ✅ Test 1: LED Control (PASSED)
**Date:** 2025-08-31  
**Status:** Complete

**Key Findings:**
- ⚠️ **CRITICAL:** XIAO ESP32S3 built-in LED does NOT support PWM/analogWrite()
- ✅ Digital on/off patterns work correctly (digitalWrite)
- ✅ All timing patterns work as expected
- ✅ Morse code error indication works properly

**Impact on Main Firmware:**
- Must update `led_control.cpp` `slowPulse()` function to use digital patterns instead of PWM
- Remove any `analogWrite()` calls for built-in LED
- Use on/off timing patterns for smooth visual effects

**Code Changes Needed:**
```cpp
// Replace this PWM approach:
analogWrite(LED_PIN, brightness);

// With this digital approach:
if (millis() - lastToggle > period/2) {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
  lastToggle = millis();
}
```

### ⚠️ Test 2: Power Management (PARTIAL)
**Date:** 2025-08-31  
**Status:** Issues found

**Key Findings:**
- ⚠️ **CRITICAL ISSUE:** Battery voltage readings are extremely low (0.37V) - indicates wrong pin or circuit issue
- ⚠️ **ISSUE:** USB detection (GPIO 21) fluctuates but debouncing helps
- ✅ Voltage readings are stable (good ADC performance)
- ⚠️ **ISSUE:** Battery showing 0.0% due to low voltage readings

**Impact on Main Firmware:**
- ⚠️ **CRITICAL:** Battery voltage circuit/pin needs verification - 0.37V is too low for working device
- Need to implement USB detection debouncing/filtering
- May need to verify correct ADC pin for battery reading
- Voltage divider ratio may be incorrect

**Code Changes Needed:**
```cpp
// Instead of single read:
bool usbConnected = digitalRead(USB_DETECT_PIN) == HIGH;

// Need debounced reading:
bool usbConnected = debouncedUSBRead(USB_DETECT_PIN);
```

### ✅ Test 1: LED Control (PASSED)
**Date:** 2025-08-31  
**Status:** Complete

**Key Findings:**
- ⚠️ **CRITICAL:** XIAO ESP32S3 built-in LED does NOT support PWM/analogWrite()
- ✅ Digital on/off patterns work correctly (digitalWrite)
- ✅ All timing patterns work as expected
- ✅ Morse code error indication works properly

**Impact on Main Firmware:**
- Must update `led_control.cpp` `slowPulse()` function to use digital patterns instead of PWM
- Remove any `analogWrite()` calls for built-in LED
- Use on/off timing patterns for smooth visual effects

**Code Changes Needed:**
```cpp
// Replace this PWM approach:
analogWrite(LED_PIN, brightness);

// With this digital approach:
if (millis() - lastToggle > period/2) {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
  lastToggle = millis();
}
```

---

## Pending Tests

### 🔄 Test 2: Power Management
**Status:** Not started
**Purpose:** Verify battery voltage reading and USB detection

### ✅ Test 3: SD Card (PASSED)
**Date:** 2025-08-31  
**Status:** Complete

**Key Findings:**
- ✅ SD card initialization works with correct pins
- ✅ File read/write operations work correctly
- ✅ Directory creation works (`/recordings`, `/uploaded`)
- ✅ File system info reporting works
- ⚠️ **CRITICAL:** Original pin configuration was completely wrong

**Correct Pin Configuration:**
- **CS: GPIO21** (not GPIO10)
- **SCK: GPIO7** (not GPIO12) 
- **MISO: GPIO8** (not GPIO13)
- **MOSI: GPIO9** (not GPIO11)

**Impact on Main Firmware:**
- Must update all SD card pin references in main firmware
- Audio recording and file storage should now work correctly

### ✅ Test 4: PDM Microphone (PASSED)
**Date:** 2025-08-31  
**Status:** Complete - Working correctly

**Key Findings:**
- ✅ **SOLUTION FOUND:** Built-in microphone is PDM, requires Arduino I2S library
- ✅ **CORRECT PINS:** PDM Clock=GPIO42, PDM Data=GPIO41
- ✅ **WORKING CONFIG:** Uses `I2S.setPinsPdmRx(42, 41)` and `I2S_MODE_PDM_RX`
- ✅ **AUDIO CAPTURE:** Successfully reads audio samples with ~1280 DC offset
- ⚠️ **LIBRARY REQUIREMENT:** Must use Arduino I2S library, NOT ESP-IDF direct calls

**Impact on Main Firmware:**
- Must update microphone initialization to use PDM mode
- Remove standard I2S configuration 
- Update pin configuration for PDM interface
- Audio capture requires PDM-specific setup

**Code Changes Needed:**
```cpp
// Replace ESP-IDF direct calls with Arduino I2S library:
#include <I2S.h>  // or "ESP_I2S.h" for 3.0.x

// Setup:
I2S.setPinsPdmRx(42, 41);  // Clock, Data
if (!I2S.begin(I2S_MODE_PDM_RX, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
  // Handle error
}

// Reading:
int sample = I2S.read();
if (sample != 0 && sample != -1 && sample != 1) {
  // Valid audio sample
}
```

### ✅ Test 5: WAV Recording (PASSED)
**Date:** 2025-08-31  
**Status:** Complete - Working perfectly

**Key Findings:**
- ✅ **WAV FILE CREATION:** Successfully creates proper WAV files with correct headers
- ✅ **SD CARD STORAGE:** Files written to `/recordings/` directory correctly  
- ✅ **PERFECT FILE SIZE:** Expected vs actual bytes match exactly (160,044 bytes)
- ✅ **AUDIO CAPTURE:** Records 5 seconds at 16kHz, 16-bit mono PCM
- ✅ **REAL-TIME RECORDING:** 80,000 samples in 4.966ms ≈ exactly 5.00 seconds

**Impact on Main Firmware:**
- Audio recording functionality is working correctly
- WAV format implementation is solid
- File I/O performance is good (no buffer issues)
- Ready for integration with voice activity detection

### ✅ Test 6: Voice Activity Detection (PASSED)
**Date:** 2025-08-31  
**Status:** Complete - Working with tuned thresholds

**Key Findings:**
- ✅ **SOLUTION FOUND:** RMS + Variance range detection works well for PDM microphone
- ✅ **OPTIMAL THRESHOLDS:** RMS=50, Variance Min=5000, Variance Max=100000
- ✅ **VARIANCE RANGE:** Using min/max variance filters both silence and non-voice sounds (cloth rubbing)
- ⚠️ **LIBRARY ISSUE:** ZCR (Zero Crossing Rate) unreliable with PDM noise floor
- ✅ **MANUAL CONTROL:** User-adjustable thresholds work better than auto-adaptive for POC

**Voice Detection Algorithm:**
- **RMS Threshold:** 50 (detects sound energy above noise floor)
- **Variance Min:** 5000 (filters out silence/steady tones)  
- **Variance Max:** 100000 (filters out extreme sounds like cloth rubbing)
- **Detection Logic:** `(RMS > 50) && (5000 < Variance < 100000)`

**Impact on Main Firmware:**
- Implement RMS + variance range detection in voice_detection.cpp
- Remove ZCR calculation (unreliable with PDM)
- Use manual thresholds as starting point, may add auto-calibration later
- LED patterns work well for visual feedback during detection

**Code Changes Needed:**
```cpp
// Voice detection logic:
bool rmsCheck = (rms > 50.0);
bool varianceCheck = (variance > 5000.0 && variance < 100000.0);
bool voiceDetected = rmsCheck && varianceCheck;

// Remove ZCR calculation - unreliable with PDM noise
```

### ✅ Test 7: State Machine Integration (PASSED)
**Date:** 2025-08-31  
**Status:** Complete - Full workflow working

**Key Findings:**
- ✅ **STATE TRANSITIONS:** All state transitions work correctly (INIT → LISTENING → RECORDING → UPLOADING → back to LISTENING)
- ✅ **LED PATTERNS:** Visual feedback matches each state perfectly
- ✅ **VOICE DETECTION:** VAD triggers recording automatically with tuned thresholds
- ✅ **AUDIO RECORDING:** 5-second WAV files created and saved properly
- ✅ **FILE UPLOAD:** Automatic upload when WiFi available, with file marking system
- ✅ **ERROR RECOVERY:** System recovers from errors and returns to INIT state
- ✅ **MANUAL OVERRIDES:** All manual state controls work for testing

**Integrated Components Working:**
- PDM microphone (I2S compatibility for ESP32 Core 3.x)
- SD card read/write operations
- WiFi connection and HTTP upload
- Voice Activity Detection (RMS + Variance thresholds)
- LED patterns for all states
- Error handling and recovery

**Impact on Main Firmware:**
- State machine architecture is proven and ready for integration
- All hardware components work together seamlessly  
- Error recovery patterns are validated
- Manual controls useful for debugging production firmware

**State Flow Validated:**
```
INIT (solid LED) → LISTENING (slow pulse) → 
RECORDING (fast blink) → UPLOADING (double flash) → 
LISTENING (cycle repeats)
ERROR (rapid flash) → recovery after 10s → INIT
```

### ✅ Test 8 & 9: WiFi Connection & HTTP Upload (PASSED)
**Date:** 2025-08-31  
**Status:** Complete - Working with hardware fix

**Key Findings:**
- ✅ **HARDWARE FIX:** WiFi antenna connection was faulty - fixed by replacing antenna
- ✅ **WIFI CONNECTION:** Connects successfully with proper ESP32S3 initialization sequence
- ✅ **HTTP UPLOAD:** File upload to test endpoint working correctly
- ✅ **STATE CLEANUP:** WiFi state cleanup prevents scan corruption after failed attempts
- ✅ **NETWORK SCANNING:** Successfully scans and finds networks

**WiFi Issues Discovered:**
- Getting scan error -2 (WIFI_SCAN_RUNNING) after failed connection attempts
- WiFi state becomes corrupted after connection failures
- Requires full WiFi state reset: `WiFi.disconnect(true)` → `WiFi.mode(WIFI_OFF)` → `WiFi.mode(WIFI_STA)`

**Impact on Main Firmware:**
- Must implement ESP32S3-specific WiFi initialization in wifi_sync.cpp
- Need proper WiFi state cleanup after failed connections
- Connection retry logic must include full state reset

**Code Changes Needed:**
```cpp
// ESP32S3 WiFi Connection Pattern:
WiFi.mode(WIFI_STA);
WiFi.disconnect();
delay(100);
WiFi.mode(WIFI_STA);  // Set mode again before begin
WiFi.begin(SSID, PASSWORD);

// After failed connection - cleanup:
WiFi.disconnect(true);
WiFi.mode(WIFI_OFF);
delay(100);
WiFi.mode(WIFI_STA);
```

### 🔄 Test 10: End-to-End Integration
**Status:** Not started
**Purpose:** Full system workflow test

---

## Hardware Specifications Confirmed

### XIAO ESP32S3 Sense Pin Mapping
- **Built-in LED:** LED_BUILTIN (digital only, no PWM support)
- **PDM Microphone:** Clock=GPIO42, Data=GPIO41 (PDM mode required)
- **SD Card SPI:** CS=GPIO21, SCK=GPIO7, MISO=GPIO8, MOSI=GPIO9 ✅
- **Power:** USB_DETECT=21, BATTERY_ADC=A0 (voltage reading issues)

### Power Characteristics
- (To be determined in Test 2)

### Audio Characteristics  
- (To be determined in Tests 4-5)

---

## Critical Issues Found

1. **LED PWM Not Supported** - Must use digital patterns for visual feedback
2. **Microphone is PDM, not I2S** - Must use PDM mode and correct pin configuration
3. **Battery Voltage Reading Issues** - Getting 0.37V readings, circuit verification needed
4. **Original SD/Microphone Pin Config Wrong** - Had to discover correct pins through testing
5. **ESP32S3 WiFi Initialization Issues** - Requires specific sequence and state cleanup after failures

## Recommendations for Main Firmware

1. **Immediate:** Update LED control to use digital patterns only
2. **Immediate:** Update microphone config to use PDM mode with pins 42/41
3. **Immediate:** Implement ESP32S3-specific WiFi initialization and cleanup in wifi_sync.cpp
4. **Before Production:** Fix battery voltage reading circuit/pin configuration
5. **Testing:** Run full test suite on any hardware changes

---

*This document is updated after each test phase. Always refer to latest version.*
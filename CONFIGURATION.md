# Configuration Guide

## Quick Start Configuration

### 1. WiFi Setup (Required for Cloud Sync)

Edit `config.h` and update these lines:

```cpp
#define WIFI_SSID "YourWiFiNetwork"
#define WIFI_PASSWORD "YourWiFiPassword"
```

**Important Notes:**
- ESP32 only supports 2.4GHz networks
- Use WPA2 security (WEP and open networks not recommended)
- Ensure strong signal strength where device will operate

### 2. API Endpoint Configuration

Update the upload endpoint in `config.h`:

```cpp
#define API_ENDPOINT "https://api.example.com/upload"
```

The API should accept HTTP POST requests with:
- Content-Type: audio/wav
- Raw WAV file data in request body
- Return 200/201 for success

### 3. Audio Quality Settings (Optional)

Default settings provide good balance of quality and storage:

```cpp
#define SAMPLE_RATE 16000        // 16kHz sampling
#define BITS_PER_SAMPLE 16       // 16-bit depth
#define CHANNELS 1               // Mono recording
```

## Advanced Configuration

### Voice Activity Detection Tuning

For different environments, adjust these parameters:

#### Quiet Indoor Environment
```cpp
#define VAD_THRESHOLD 300
#define VAD_SENSITIVITY 1.5
#define VAD_NOISE_FLOOR 50
```

#### Normal Office Environment (Default)
```cpp
#define VAD_THRESHOLD 500
#define VAD_SENSITIVITY 2.0
#define VAD_NOISE_FLOOR 100
```

#### Noisy Environment
```cpp
#define VAD_THRESHOLD 800
#define VAD_SENSITIVITY 3.0
#define VAD_NOISE_FLOOR 200
```

### Recording Behavior

#### Maximum Recording Length
```cpp
#define MAX_RECORDING_DURATION_MS (5 * 60 * 1000)  // 5 minutes
```
- Prevents excessively long recordings
- Helps manage storage space
- Typical range: 2-10 minutes

#### Silence Detection
```cpp
#define SILENCE_TIMEOUT_MS (3 * 1000)  // 3 seconds
```
- How long to wait before stopping recording
- Shorter = more sensitive to pauses
- Longer = captures longer pauses in speech
- Typical range: 2-5 seconds

### Power Management

#### Sleep Timeouts
```cpp
#define SLEEP_TIMEOUT_MS (30 * 1000)           // Light sleep after 30s
#define DEEP_SLEEP_TIMEOUT_MS (5 * 60 * 1000)  // Deep sleep after 5min
```

#### Battery Thresholds
```cpp
#define LOW_BATTERY_THRESHOLD 10.0      // Warning at 10%
#define CRITICAL_BATTERY_THRESHOLD 5.0  // Deep sleep at 5%
```

#### LED Brightness (affects power consumption)
```cpp
#define LED_BRIGHTNESS 128  // 0-255, lower = less power
```

### Storage Configuration

#### File Organization
```cpp
#define RECORDINGS_DIR "/recordings"
#define UPLOADED_DIR "/uploaded"
```

#### Cleanup Settings
Files are automatically moved to `/uploaded` after successful upload.
Old uploaded files are deleted when storage space runs low.

## Environment-Specific Configurations

### Conference Room Recording
```cpp
// Optimized for capturing meetings
#define VAD_THRESHOLD 400
#define VAD_SENSITIVITY 2.5
#define SILENCE_TIMEOUT_MS (5 * 1000)  // Longer pauses
#define MAX_RECORDING_DURATION_MS (30 * 60 * 1000)  // 30 minutes
```

### Personal Voice Notes
```cpp
// Optimized for individual use
#define VAD_THRESHOLD 300
#define VAD_SENSITIVITY 1.8
#define SILENCE_TIMEOUT_MS (2 * 1000)  // Shorter pauses
#define MAX_RECORDING_DURATION_MS (2 * 60 * 1000)  // 2 minutes
```

### Outdoor/Vehicle Use
```cpp
// Higher thresholds for noisy environments
#define VAD_THRESHOLD 700
#define VAD_SENSITIVITY 3.5
#define VAD_NOISE_FLOOR 250
#define SILENCE_TIMEOUT_MS (4 * 1000)
```

## Debug Configuration

### Enable Debug Output
```cpp
#define DEBUG_ENABLED true
```

This enables detailed serial output for troubleshooting:
- VAD analysis values
- Power management status
- WiFi connection details
- File operations
- Upload progress

### Serial Monitor Settings
- Baud Rate: 115200
- Line Ending: Both NL & CR

### Debug Output Examples

#### Normal Operation
```
VAD - RMS: 234.56, ZCR: 0.234, Noise: 123.45, Threshold: 246.90, Voice: NO
Power Status - Battery: 3.85V (75.2%), USB: Disconnected
```

#### Voice Detection
```
Voice detected, starting recording
Recording started: /recordings/2024-01-15/REC_20240115_143022.wav
VAD - RMS: 678.90, ZCR: 0.456, Noise: 123.45, Threshold: 246.90, Voice: YES
```

#### Upload Process
```
USB connected, exiting low battery mode
Connecting to WiFi: MyNetwork
WiFi connected! IP: 192.168.1.100
Uploading file 1: /recordings/2024-01-15/REC_20240115_143022.wav
Upload response - Code: 200
Upload successful
File marked as uploaded: /uploaded/2024-01-15/REC_20240115_143022.wav
```

## Performance Tuning

### Optimizing for Battery Life
```cpp
// Reduce power consumption
#define LED_BRIGHTNESS 64
#define SLEEP_TIMEOUT_MS (15 * 1000)
#define VAD_SAMPLE_WINDOW 128  // Smaller buffer
```

### Optimizing for Audio Quality
```cpp
// Higher quality settings
#define SAMPLE_RATE 22050
#define BUFFER_SIZE 2048
#define VAD_SAMPLE_WINDOW 512
```

### Optimizing for Responsiveness
```cpp
// Faster response to voice
#define VAD_SAMPLE_WINDOW 128
#define SILENCE_TIMEOUT_MS (1 * 1000)
```

## Troubleshooting Configuration Issues

### Voice Detection Problems
1. **Too Sensitive**: Increase `VAD_THRESHOLD`
2. **Not Sensitive Enough**: Decrease `VAD_THRESHOLD`
3. **False Triggers**: Increase `VAD_SENSITIVITY`
4. **Missing Speech**: Decrease `VAD_SENSITIVITY`

### Power Issues
1. **Battery Drains Quickly**: Reduce `LED_BRIGHTNESS`, increase sleep timeouts
2. **Won't Wake**: Check `SLEEP_TIMEOUT_MS` settings
3. **False Low Battery**: Adjust `BATTERY_VOLTAGE_DIVIDER`

### Storage Issues
1. **SD Card Errors**: Check card format (must be FAT32)
2. **Files Not Found**: Verify directory paths in config
3. **Upload Failures**: Check `API_ENDPOINT` URL

### WiFi Issues
1. **Won't Connect**: Verify 2.4GHz network and credentials
2. **Slow Uploads**: Check `API_TIMEOUT_MS` setting
3. **Connection Drops**: Ensure strong signal strength

## Factory Reset

To restore default configuration:

1. Set all `config.h` values to defaults shown in this guide
2. Format SD card as FAT32
3. Re-upload firmware
4. Power cycle device

The device will recreate directory structure and calibrate VAD on first boot.
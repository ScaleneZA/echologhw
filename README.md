# XIAO ESP32S3 Sense Smart Voice Recorder

An intelligent, battery-efficient voice-activated recorder for the XIAO ESP32S3 Sense that automatically detects and records conversations, manages power efficiently, and syncs recordings when charging.

## Features

- **Automatic Voice Detection**: Uses advanced Voice Activity Detection (VAD) to start recording when speech is detected
- **Power Efficient**: Implements light sleep and deep sleep modes for maximum battery life
- **Smart LED Indicators**: Visual feedback for different system states including morse code error indication
- **SD Card Storage**: Organized file structure with timestamp-based filenames
- **Cloud Sync**: Automatic upload of recordings when USB power is connected
- **WAV Audio Format**: High-quality 16kHz, 16-bit mono recordings
- **Battery Monitoring**: Real-time battery level tracking with low-battery warnings

## Hardware Requirements

- XIAO ESP32S3 Sense (with built-in microphone)
- MicroSD card (Class 10 or higher recommended)
- USB-C cable for charging and data sync
- Optional: External battery for extended operation

## Pin Configuration

The firmware uses the following pin assignments for the XIAO ESP32S3 Sense:

```
I2S Microphone:
- WS (Word Select): GPIO 42
- SCK (Serial Clock): GPIO 41  
- SD (Serial Data): GPIO 2

SD Card (SPI):
- CS (Chip Select): GPIO 10
- MOSI: GPIO 11
- MISO: GPIO 13
- SCK: GPIO 12

Power Management:
- USB Detect: GPIO 21
- Battery ADC: A0
- LED: Built-in LED
```

## Setup Instructions

### 1. Hardware Setup

1. Insert a formatted microSD card into the XIAO ESP32S3 Sense
2. Connect the device via USB-C for programming
3. Ensure the built-in microphone is not obstructed

### 2. Software Configuration

1. Open `config.h` and update the following settings:

```cpp
// WiFi Configuration
#define WIFI_SSID "YourWiFiNetwork"
#define WIFI_PASSWORD "YourWiFiPassword"

// API Configuration  
#define API_ENDPOINT "https://api.example.com/upload"
```

2. Adjust other parameters as needed:
   - `VAD_THRESHOLD`: Voice detection sensitivity (500 default)
   - `MAX_RECORDING_DURATION_MS`: Maximum recording length (5 minutes default)
   - `SILENCE_TIMEOUT_MS`: Silence before stopping recording (3 seconds default)

### 3. Programming

1. Install the ESP32 board package in Arduino IDE
2. Select "XIAO_ESP32S3" as your board
3. Install required libraries:
   - ESP32 Audio Library
   - WiFi library (built-in)
   - SD library (built-in)
4. Compile and upload the firmware

## Operation

### LED Status Indicators

| Pattern | Meaning |
|---------|---------|
| Slow pulse (1Hz) | Idle/listening mode |
| Fast blink (3Hz) | Recording active |
| Rapid flash (5Hz) | Low battery warning |
| Solid light | WiFi connection in progress |
| Double flash | File upload in progress |
| Morse code | Error indication (see error codes) |

### Error Codes (Morse Code)

| Code | Pattern | Meaning |
|------|---------|---------|
| S | `...` | SD card initialization failed |
| A | `.-` | Audio initialization failed |
| W | `.--` | WiFi connection failed |
| R | `.-.` | Recording failed |
| U | `..-` | Upload failed |
| B | `-...` | Low battery |
| D | `-..` | SD write failed |
| E | `.` | Generic error |

### Voice Activity Detection

The VAD algorithm uses the following techniques:
- **RMS Energy Analysis**: Calculates root mean square of audio samples
- **Zero Crossing Rate**: Analyzes frequency content to distinguish speech from noise
- **Adaptive Noise Floor**: Automatically adjusts to ambient noise levels
- **Calibration**: 3-second calibration period on startup for optimal performance

## File Organization

Recordings are organized on the SD card as follows:

```
/recordings/
  ├── YYYY-MM-DD/
  │   ├── REC_YYYYMMDD_HHMMSS.wav
  │   └── ...
  └── ...

/uploaded/
  ├── YYYY-MM-DD/
  │   ├── REC_YYYYMMDD_HHMMSS.wav
  │   └── ...
  └── ...
```

## Power Consumption Estimates

| State | Current Draw | Duration |
|-------|-------------|----------|
| Deep Sleep | ~10µA | Indefinite |
| Light Sleep | ~800µA | During silence |
| Listening (VAD) | ~20mA | Continuous |
| Recording | ~25mA | During speech |
| WiFi Upload | ~80mA | During sync |

**Estimated Battery Life**: 
- With 1000mAh battery: 20-30 hours of typical use
- Standby time: Several weeks in deep sleep mode

## API Integration

The device uploads recordings via HTTP POST to the configured endpoint. Each request includes:

**Headers**:
- `Content-Type: audio/wav`
- `X-Device-ID: [MAC Address]`
- `X-Timestamp: [Unix timestamp]`
- `X-Filename: [Original filename]`

**Body**: Raw WAV file data

Expected API response codes:
- `200/201`: Success
- `400`: Bad request (invalid file format)
- `401`: Unauthorized
- `413`: File too large
- `500`: Server error (will retry)

## Troubleshooting

### Common Issues

1. **SD Card Not Detected**
   - Ensure card is formatted as FAT32
   - Check card is properly inserted
   - Try a different card (Class 10 recommended)

2. **Poor Voice Detection**
   - Adjust `VAD_THRESHOLD` in config.h
   - Ensure microphone is not obstructed
   - Run calibration in quiet environment

3. **WiFi Connection Issues**
   - Verify SSID and password in config.h
   - Check signal strength
   - Ensure 2.4GHz network (ESP32 doesn't support 5GHz)

4. **Upload Failures**
   - Verify API endpoint is accessible
   - Check internet connectivity
   - Review server logs for errors

### Debug Output

Enable debug output by setting `DEBUG_ENABLED true` in config.h. Connect via serial monitor at 115200 baud to view detailed system information.

## Advanced Configuration

### Customizing VAD Parameters

For different environments, you may need to adjust VAD settings:

```cpp
// High noise environment
#define VAD_THRESHOLD 800
#define VAD_SENSITIVITY 3.0

// Quiet environment  
#define VAD_THRESHOLD 300
#define VAD_SENSITIVITY 1.5
```

### Power Optimization

For maximum battery life:

```cpp
// Increase sleep timeouts
#define SLEEP_TIMEOUT_MS (60 * 1000)      // 1 minute
#define DEEP_SLEEP_TIMEOUT_MS (10 * 60 * 1000)  // 10 minutes

// Reduce LED brightness
#define LED_BRIGHTNESS 64
```

## Contributing

When modifying the firmware:

1. Maintain the modular structure
2. Add comprehensive error handling
3. Update documentation
4. Test power consumption changes
5. Verify audio quality is maintained

## License

This project is provided as-is for educational and development purposes.
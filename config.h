#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "YourWiFiNetwork"
#define WIFI_PASSWORD "YourWiFiPassword"
#define WIFI_TIMEOUT_MS 30000

// API Configuration
#define API_ENDPOINT "https://api.example.com/upload"
#define API_TIMEOUT_MS 30000
#define MAX_UPLOAD_RETRIES 3

// Audio Configuration
#define SAMPLE_RATE 16000
#define BITS_PER_SAMPLE 16
#define CHANNELS 1
#define I2S_PORT I2S_NUM_0
#define I2S_WS_PIN 42
#define I2S_SCK_PIN 41
#define I2S_SD_PIN 2

// Recording Configuration
#define MAX_RECORDING_DURATION_MS (5 * 60 * 1000)  // 5 minutes
#define SILENCE_TIMEOUT_MS (3 * 1000)              // 3 seconds
#define BUFFER_SIZE 1024

// Voice Activity Detection
#define VAD_THRESHOLD 500
#define VAD_SAMPLE_WINDOW 256
#define VAD_NOISE_FLOOR 100
#define VAD_SENSITIVITY 2.0

// Power Management
#define LOW_BATTERY_THRESHOLD 10.0        // 10%
#define CRITICAL_BATTERY_THRESHOLD 5.0    // 5%
#define SLEEP_TIMEOUT_MS (30 * 1000)      // 30 seconds
#define DEEP_SLEEP_TIMEOUT_MS (5 * 60 * 1000)  // 5 minutes
#define BATTERY_ADC_PIN A0
#define USB_DETECT_PIN 21
#define BATTERY_VOLTAGE_DIVIDER 2.0

// LED Configuration
#define LED_PIN LED_BUILTIN
#define LED_BRIGHTNESS 128

// LED Patterns (in milliseconds)
#define LED_LISTENING_PERIOD 1000    // 1Hz slow pulse
#define LED_RECORDING_PERIOD 333     // 3Hz fast blink
#define LED_LOW_BATTERY_PERIOD 200   // 5Hz rapid flash
#define LED_UPLOADING_ON_TIME 100
#define LED_UPLOADING_OFF_TIME 100

// SD Card Configuration
#define SD_CS_PIN 10
#define SD_MOSI_PIN 11
#define SD_MISO_PIN 13
#define SD_SCK_PIN 12
#define SD_SPI_FREQ 4000000

// File Management
#define RECORDINGS_DIR "/recordings"
#define UPLOADED_DIR "/uploaded"
#define CONFIG_FILE "/config.txt"
#define MAX_FILENAME_LENGTH 64

// System Configuration
#define DEBUG_ENABLED true
#define SERIAL_BAUD_RATE 115200
#define WATCHDOG_TIMEOUT_SEC 30

// Error Codes
#define ERROR_SD_INIT_FAILED        0x01
#define ERROR_AUDIO_INIT_FAILED     0x02
#define ERROR_WIFI_CONNECT_FAILED   0x03
#define ERROR_RECORDING_FAILED      0x04
#define ERROR_UPLOAD_FAILED         0x05
#define ERROR_LOW_BATTERY           0x06
#define ERROR_SD_WRITE_FAILED       0x07

// Morse Code Patterns for LED Error Indication
#define MORSE_DOT_DURATION 200
#define MORSE_DASH_DURATION 600
#define MORSE_SYMBOL_GAP 200
#define MORSE_LETTER_GAP 600

// Debug Macros
#if DEBUG_ENABLED
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(format, ...) Serial.printf(format, ##__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(format, ...)
#endif

// Utility Macros
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#endif // CONFIG_H
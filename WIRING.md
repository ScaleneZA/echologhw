# Wiring Diagram and Pin Reference

## XIAO ESP32S3 Sense Pin Layout

```
                    XIAO ESP32S3 Sense
                    ┌─────────────────┐
                    │     USB-C       │
                    │    ┌─────┐      │
                    │    └─────┘      │
                    │                 │
                5V  │ 1  ●           │
               GND  │ 2  ●           │
              3V3   │ 3  ●           │
         (A0) GPIO1 │ 4  ●           │
         (A1) GPIO2 │ 5  ●  ● 14     │ GPIO43 (D0/RX)
         (A2) GPIO3 │ 6  ●  ● 13     │ GPIO44 (D1/TX)  
         (A3) GPIO4 │ 7  ●  ● 12     │ GPIO12 (SCK)
    (SDA) GPIO5     │ 8  ●  ● 11     │ GPIO11 (MOSI)
    (SCL) GPIO6     │ 9  ●  ● 10     │ GPIO13 (MISO)
         GPIO7      │10  ●  ● 9      │ GPIO10 (CS)
         GPIO15     │11  ●  ● 8      │ GPIO9
         GPIO16     │12  ●  ● 7      │ GPIO8
         GPIO17     │13  ●  ● 6      │ GPIO21 (USB_DETECT)
         GPIO18     │14  ●  ● 5      │ GPIO20
                    │                 │
                    │  Built-in MIC   │
                    │  Built-in LED   │
                    │  SD Card Slot   │
                    └─────────────────┘
```

## Internal Connections (Built-in)

The XIAO ESP32S3 Sense has several built-in components that are pre-wired:

### Built-in Microphone (I2S)
- **WS (Word Select)**: GPIO42 (internal)
- **SCK (Serial Clock)**: GPIO41 (internal)  
- **SD (Serial Data)**: GPIO2 (internal)

### Built-in LED
- **LED Pin**: Built-in LED connected to internal GPIO

### SD Card Slot
- **CS (Chip Select)**: GPIO21 (shared with USB detect)
- **MOSI**: GPIO11
- **MISO**: GPIO13
- **SCK**: GPIO12

## External Connections Required

### None Required for Basic Operation

The XIAO ESP32S3 Sense includes everything needed for the voice recorder:
- ✅ Built-in microphone
- ✅ Built-in LED for status
- ✅ Built-in SD card slot
- ✅ Built-in USB-C connector for power and programming

### Optional External Components

#### External LED (if additional indication needed)
```
GPIO8 ──┤ 330Ω ├──[LED]──┤ GND
```

#### External Push Button (for manual upload trigger)
```
GPIO7 ──┤ 10kΩ ├── 3V3
   │
   └──[Button]──┤ GND
```

#### External Battery Connector
```
Battery (+) ── 5V pin
Battery (-) ── GND pin
```

## Power Connections

### USB-C Charging
- Connect USB-C cable to built-in connector
- Provides 5V power and charges internal battery
- Data lines used for programming

### Battery Power
- Use built-in battery connector or 5V/GND pins
- Recommended: 3.7V LiPo battery (500-2000mAh)
- Built-in charging circuit handles battery management

## Signal Connections

### I2S Microphone (Internal)
```
ESP32S3          Built-in Mic
GPIO42    ←──    WS
GPIO41    ──→    SCK  
GPIO2     ←──    SD
```

### SPI SD Card (Internal)
```
ESP32S3          SD Card
GPIO10    ──→    CS
GPIO11    ──→    MOSI
GPIO13    ←──    MISO
GPIO12    ──→    SCK
3V3       ──→    VCC
GND       ──→    GND
```

### Power Management
```
ESP32S3          Function
GPIO21    ←──    USB Detect (shared with SD CS)
A0        ←──    Battery Voltage Monitor
```

## Voltage Levels

| Signal | Voltage | Notes |
|--------|---------|-------|
| Digital I/O | 3.3V | All GPIO pins |
| Analog Input | 0-3.3V | ADC reference |
| USB Power | 5V | USB-C connector |
| Battery | 3.7V | LiPo battery nominal |
| SD Card | 3.3V | SPI communication |
| I2S | 3.3V | Digital audio |

## Current Requirements

| Component | Current Draw | Notes |
|-----------|-------------|-------|
| ESP32S3 Core | 20-80mA | Depends on WiFi state |
| Microphone | 1-2mA | Continuous operation |
| SD Card | 5-25mA | During read/write |
| LED | 5-20mA | Depends on brightness |
| **Total Active** | **31-127mA** | Maximum during WiFi upload |
| **Total Sleep** | **<1mA** | Deep sleep mode |

## Assembly Notes

### 1. No External Wiring Required
The XIAO ESP32S3 Sense is a complete solution requiring no external components for basic operation.

### 2. SD Card Installation
- Insert microSD card into built-in slot
- Format as FAT32 before first use
- Recommended: Class 10 or higher for reliable recording

### 3. Battery Connection
- Use built-in battery connector for permanent installation
- Alternatively connect battery to 5V/GND pins
- Ensure proper polarity to avoid damage

### 4. Enclosure Considerations
- Ensure microphone port is not blocked
- LED should be visible for status indication
- SD card should be accessible for removal
- USB-C port accessible for programming/charging

### 5. Mechanical Mounting
- Use mounting holes if available on your specific board revision
- Consider vibration damping for audio quality
- Keep device stable during recording

## Testing Connections

### Power Test
1. Connect USB-C cable
2. Built-in LED should illuminate
3. Check serial output at 115200 baud

### Audio Test  
1. Upload firmware with debug enabled
2. Monitor serial output for VAD levels
3. Speak near device to trigger recording
4. Check LED changes to recording mode

### SD Card Test
1. Insert formatted microSD card
2. Power on device
3. Check serial output for successful initialization
4. Look for directory creation messages

### WiFi Test
1. Configure WiFi credentials in config.h
2. Connect USB power
3. Monitor serial for WiFi connection
4. LED should show solid during connection

This minimal wiring requirement makes the XIAO ESP32S3 Sense ideal for quick prototyping and deployment of voice recording applications.
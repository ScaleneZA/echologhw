/*
 * Test 2: Power Management Test
 * 
 * This test verifies battery voltage reading and USB power detection.
 * Critical for power management, low battery warnings, and upload triggering.
 * 
 * Expected behavior:
 * 1. Reads battery voltage from ADC
 * 2. Calculates battery percentage
 * 3. Detects USB power connection status
 * 4. Reports power states every 2 seconds
 * 5. Tests different voltage thresholds
 * 
 * SUCCESS CRITERIA:
 * - Battery voltage readings are stable and reasonable (3.0-4.2V range)
 * - USB detection changes when you plug/unplug USB cable
 * - Battery percentage calculation works correctly
 * - Threshold detection (low/critical battery) works
 * - Values don't fluctuate wildly
 */

// Pin definitions from config.h
#define BATTERY_ADC_PIN A0
#define USB_DETECT_PIN 21
#define BATTERY_VOLTAGE_DIVIDER 2.0

// Thresholds from config.h
#define LOW_BATTERY_THRESHOLD 10.0        // 10%
#define CRITICAL_BATTERY_THRESHOLD 5.0    // 5%

// LED for status indication
#define LED_PIN LED_BUILTIN

// Battery voltage range (typical Li-Po)
#define BATTERY_MIN_VOLTAGE 3.0   // 0% charge
#define BATTERY_MAX_VOLTAGE 4.2   // 100% charge

// Test variables
unsigned long lastReading = 0;
float voltageHistory[10];
int historyIndex = 0;
bool historyFilled = false;

// USB detection debouncing
bool usbHistory[10];
int usbHistoryIndex = 0;
bool usbHistoryFilled = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== Power Management Test ===");
  Serial.println("This test monitors battery voltage and USB connection");
  Serial.println("Try plugging/unplugging USB cable to test detection");
  Serial.println("Battery readings every 2 seconds...\n");
  
  // Configure pins
  pinMode(USB_DETECT_PIN, INPUT);
  pinMode(BATTERY_ADC_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // Initialize ADC
  analogReadResolution(12);  // 12-bit resolution (0-4095)
  
  Serial.println("Pin Configuration:");
  Serial.printf("Battery ADC Pin: %d\n", BATTERY_ADC_PIN);
  Serial.printf("USB Detect Pin: %d\n", USB_DETECT_PIN);
  Serial.printf("Voltage Divider: %.1f\n", BATTERY_VOLTAGE_DIVIDER);
  Serial.println();
  
  // Test basic pin reading
  Serial.println("Testing basic pin reads...");
  Serial.printf("Raw ADC: %d\n", analogRead(BATTERY_ADC_PIN));
  Serial.printf("USB Pin: %d\n", digitalRead(USB_DETECT_PIN));
  Serial.println();
  
  // Initialize voltage history
  for (int i = 0; i < 10; i++) {
    voltageHistory[i] = 0.0;
  }
  
  // Initialize USB history
  for (int i = 0; i < 10; i++) {
    usbHistory[i] = false;
  }
  
  lastReading = millis();
}

void loop() {
  if (millis() - lastReading >= 2000) {  // Every 2 seconds
    performPowerReading();
    lastReading = millis();
  }
  
  // Simple LED heartbeat
  static unsigned long ledTime = 0;
  if (millis() - ledTime > 500) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    ledTime = millis();
  }
  
  delay(10);
}

void performPowerReading() {
  // Read raw ADC value
  int rawADC = analogRead(BATTERY_ADC_PIN);
  
  // Convert to voltage
  float adcVoltage = (rawADC / 4095.0) * 3.3;  // ESP32 reference voltage
  float batteryVoltage = adcVoltage * BATTERY_VOLTAGE_DIVIDER;
  
  // Store in history for averaging
  voltageHistory[historyIndex] = batteryVoltage;
  historyIndex = (historyIndex + 1) % 10;
  if (historyIndex == 0) historyFilled = true;
  
  // Calculate average voltage
  float avgVoltage = calculateAverageVoltage();
  
  // Calculate battery percentage
  float batteryPercent = calculateBatteryPercentage(avgVoltage);
  
  // Read USB status with debouncing
  bool usbRaw = digitalRead(USB_DETECT_PIN) == HIGH;
  bool usbConnected = debouncedUSBRead(usbRaw);
  
  // Determine power status
  String powerStatus = determinePowerStatus(batteryPercent, usbConnected);
  
  // Print comprehensive report
  Serial.println("=== Power Reading ===");
  Serial.printf("Raw ADC: %d (0-4095)\n", rawADC);
  Serial.printf("ADC Voltage: %.3fV\n", adcVoltage);
  Serial.printf("Battery Voltage: %.3fV\n", batteryVoltage);
  Serial.printf("Avg Battery Voltage: %.3fV\n", avgVoltage);
  Serial.printf("Battery Percentage: %.1f%%\n", batteryPercent);
  Serial.printf("USB Raw Reading: %s\n", usbRaw ? "HIGH" : "LOW");
  Serial.printf("USB Debounced: %s\n", usbConnected ? "YES" : "NO");
  Serial.printf("Power Status: %s\n", powerStatus.c_str());
  
  // Voltage stability check
  if (historyFilled) {
    float minVolt = batteryVoltage, maxVolt = batteryVoltage;
    for (int i = 0; i < 10; i++) {
      if (voltageHistory[i] < minVolt) minVolt = voltageHistory[i];
      if (voltageHistory[i] > maxVolt) maxVolt = voltageHistory[i];
    }
    float stability = maxVolt - minVolt;
    Serial.printf("Voltage Stability: %.3fV range ", stability);
    if (stability < 0.1) {
      Serial.println("(GOOD - stable)");
    } else if (stability < 0.2) {
      Serial.println("(OK - minor fluctuation)");
    } else {
      Serial.println("(POOR - unstable readings)");
    }
  }
  
  Serial.println();
}

float calculateAverageVoltage() {
  if (!historyFilled && historyIndex == 0) {
    return voltageHistory[0];  // Only one reading so far
  }
  
  float sum = 0;
  int count = historyFilled ? 10 : historyIndex;
  
  for (int i = 0; i < count; i++) {
    sum += voltageHistory[i];
  }
  
  return sum / count;
}

float calculateBatteryPercentage(float voltage) {
  if (voltage >= BATTERY_MAX_VOLTAGE) return 100.0;
  if (voltage <= BATTERY_MIN_VOLTAGE) return 0.0;
  
  // Linear interpolation between min and max voltage
  float percent = ((voltage - BATTERY_MIN_VOLTAGE) / 
                   (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) * 100.0;
  
  return constrain(percent, 0.0, 100.0);
}

bool debouncedUSBRead(bool currentReading) {
  // Store current reading in history
  usbHistory[usbHistoryIndex] = currentReading;
  usbHistoryIndex = (usbHistoryIndex + 1) % 10;
  if (usbHistoryIndex == 0) usbHistoryFilled = true;
  
  // Count how many readings are TRUE
  int trueCount = 0;
  int totalReadings = usbHistoryFilled ? 10 : usbHistoryIndex;
  
  for (int i = 0; i < totalReadings; i++) {
    if (usbHistory[i]) trueCount++;
  }
  
  // Require at least 70% of readings to be consistent
  if (totalReadings < 3) {
    return currentReading;  // Not enough history, use current
  }
  
  return (trueCount >= (totalReadings * 0.7));
}

String determinePowerStatus(float batteryPercent, bool usbConnected) {
  if (usbConnected) {
    if (batteryPercent >= 95.0) {
      return "USB Connected - Fully Charged";
    } else {
      return "USB Connected - Charging";
    }
  } else {
    if (batteryPercent <= CRITICAL_BATTERY_THRESHOLD) {
      return "CRITICAL BATTERY - Sleep Required";
    } else if (batteryPercent <= LOW_BATTERY_THRESHOLD) {
      return "LOW BATTERY - Warning";
    } else if (batteryPercent >= 75.0) {
      return "Battery Good";
    } else {
      return "Battery OK";
    }
  }
}
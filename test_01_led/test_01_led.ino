/*
 * Test 1: LED Pattern Test
 * 
 * This test verifies that the LED can display all the patterns
 * used in the main voice recorder firmware.
 * 
 * Expected behavior:
 * 1. LED turns on solid for 2 seconds (initialization)
 * 2. Slow pulse (1Hz) for 5 seconds (listening mode)
 * 3. Fast blink (3Hz) for 5 seconds (recording mode) 
 * 4. Rapid flash (5Hz) for 5 seconds (low battery)
 * 5. Double flash pattern for 5 seconds (uploading)
 * 6. Morse code "S" (...) for error indication
 * 7. Repeats the cycle
 * 
 * SUCCESS CRITERIA:
 * - You can visually see each distinct LED pattern
 * - Patterns match the expected timing
 * - LED brightness is appropriate
 * - Serial output shows current pattern being tested
 */

#include <math.h>

#define LED_PIN LED_BUILTIN
#define LED_BRIGHTNESS 128

// LED Patterns (in milliseconds)
#define LED_LISTENING_PERIOD 1000    // 1Hz slow pulse
#define LED_RECORDING_PERIOD 333     // 3Hz fast blink
#define LED_LOW_BATTERY_PERIOD 200   // 5Hz rapid flash
#define LED_UPLOADING_ON_TIME 100
#define LED_UPLOADING_OFF_TIME 100

// Morse code timing
#define MORSE_DOT_DURATION 200
#define MORSE_DASH_DURATION 600
#define MORSE_SYMBOL_GAP 200
#define MORSE_LETTER_GAP 600

unsigned long testStartTime = 0;
int currentTest = 0;
unsigned long lastLEDToggle = 0;
bool ledState = false;
int morseStep = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== LED Pattern Test ===");
  Serial.println("Watch the LED for different patterns:");
  Serial.println("1. Solid (2s) - Initialization");
  Serial.println("2. Slow pulse (5s) - Listening mode");
  Serial.println("3. Fast blink (5s) - Recording mode");
  Serial.println("4. Rapid flash (5s) - Low battery");
  Serial.println("5. Double flash (5s) - Uploading");
  Serial.println("6. Morse 'S' (5s) - Error code");
  Serial.println("Then repeats...\n");
  
  Serial.printf("LED_PIN = %d\n", LED_PIN);
  
  pinMode(LED_PIN, OUTPUT);
  
  // Test basic LED control
  Serial.println("Testing basic LED control...");
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  Serial.println("Basic test complete\n");
  
  testStartTime = millis();
  currentTest = 0;
}

void loop() {
  static bool testStarted = false;
  unsigned long elapsed = millis() - testStartTime;
  
  switch(currentTest) {
    case 0: // Solid LED (initialization)
      if (!testStarted) {
        Serial.println("Testing: SOLID LED (initialization)");
        testStarted = true;
      }
      digitalWrite(LED_PIN, HIGH);
      if (elapsed > 2000) {
        nextTest();
        testStarted = false;
      }
      break;
      
    case 1: // Slow pulse (listening)
      if (!testStarted) {
        Serial.println("Testing: SLOW PULSE (listening mode)");
        testStarted = true;
      }
      slowPulse();
      if (elapsed > 5000) {
        nextTest();
        testStarted = false;
      }
      break;
      
    case 2: // Fast blink (recording)
      if (!testStarted) {
        Serial.println("Testing: FAST BLINK (recording mode)");
        testStarted = true;
      }
      fastBlink();
      if (elapsed > 5000) {
        nextTest();
        testStarted = false;
      }
      break;
      
    case 3: // Rapid flash (low battery)
      if (!testStarted) {
        Serial.println("Testing: RAPID FLASH (low battery)");
        testStarted = true;
      }
      rapidFlash();
      if (elapsed > 5000) {
        nextTest();
        testStarted = false;
      }
      break;
      
    case 4: // Double flash (uploading)
      if (!testStarted) {
        Serial.println("Testing: DOUBLE FLASH (uploading)");
        testStarted = true;
      }
      doubleFlash();
      if (elapsed > 5000) {
        nextTest();
        testStarted = false;
      }
      break;
      
    case 5: // Morse code S (error)
      if (!testStarted) {
        Serial.println("Testing: MORSE CODE 'S' (error - SD card failed)");
        testStarted = true;
        morseStep = 0;
      }
      morseCodeS();
      if (elapsed > 5000) {
        nextTest();
        testStarted = false;
      }
      break;
  }
}

void nextTest() {
  currentTest++;
  if (currentTest > 5) {
    currentTest = 0;
    Serial.println("\n=== Test cycle complete, repeating ===\n");
  }
  testStartTime = millis();
  lastLEDToggle = 0;
  digitalWrite(LED_PIN, LOW);
  delay(500); // Brief pause between tests
}

void slowPulse() {
  // Simple on/off pattern for slow pulse (1Hz)
  if (millis() - lastLEDToggle > LED_LISTENING_PERIOD / 2) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastLEDToggle = millis();
  }
}

void fastBlink() {
  if (millis() - lastLEDToggle > LED_RECORDING_PERIOD / 2) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastLEDToggle = millis();
  }
}

void rapidFlash() {
  if (millis() - lastLEDToggle > LED_LOW_BATTERY_PERIOD / 2) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastLEDToggle = millis();
  }
}

void doubleFlash() {
  unsigned long elapsed = (millis() - testStartTime) % 1000; // 1 second cycle
  
  if (elapsed < LED_UPLOADING_ON_TIME) {
    digitalWrite(LED_PIN, HIGH); // First flash
  } else if (elapsed < LED_UPLOADING_ON_TIME + LED_UPLOADING_OFF_TIME) {
    digitalWrite(LED_PIN, LOW);
  } else if (elapsed < LED_UPLOADING_ON_TIME * 2 + LED_UPLOADING_OFF_TIME) {
    digitalWrite(LED_PIN, HIGH); // Second flash
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void morseCodeS() {
  // Morse code for 'S' is three dots: . . .
  unsigned long elapsed = millis() - testStartTime;
  unsigned long cycleTime = elapsed % 2000; // 2 second cycle
  
  if (cycleTime < MORSE_DOT_DURATION) {
    digitalWrite(LED_PIN, HIGH); // First dot
  } else if (cycleTime < MORSE_DOT_DURATION + MORSE_SYMBOL_GAP) {
    digitalWrite(LED_PIN, LOW);
  } else if (cycleTime < MORSE_DOT_DURATION * 2 + MORSE_SYMBOL_GAP) {
    digitalWrite(LED_PIN, HIGH); // Second dot
  } else if (cycleTime < MORSE_DOT_DURATION * 2 + MORSE_SYMBOL_GAP * 2) {
    digitalWrite(LED_PIN, LOW);
  } else if (cycleTime < MORSE_DOT_DURATION * 3 + MORSE_SYMBOL_GAP * 2) {
    digitalWrite(LED_PIN, HIGH); // Third dot
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}
/*
 * Braille-Click Firmware
 * One-handed semi-chording input device based on braille patterns
 * 
 * Hardware: Arduino Pro Micro (ATmega32U4)
 * 4 finger keys + thumb controls + haptic motor
 */

#include <Keyboard.h>

// Pin definitions
const int INDEX_PIN = 2;      // Index finger (dot 1/5)
const int MIDDLE_PIN = 3;     // Middle finger (dot 2/6)  
const int RING_PIN = 4;       // Ring finger (dot 3/7)
const int PINKY_PIN = 5;      // Pinky finger (dot 4/8)
const int THUMB_UP_PIN = 6;   // Thumb up
const int THUMB_DOWN_PIN = 7; // Thumb down
const int THUMB_LEFT_PIN = 8; // Thumb left
const int THUMB_RIGHT_PIN = 9; // Thumb right
const int THUMB_PRESS_PIN = 10; // Thumb press
const int HAPTIC_PIN = 11;    // Haptic motor control

// Timing constants
const unsigned long DEBOUNCE_TIME = 10;    // ms
const unsigned long PHASE_TIMEOUT = 200;   // ms
const unsigned long CONFIG_HOLD_TIME = 3000; // ms

// State machine
enum InputState {
  WAIT_LEFT,
  LEFT_LOCKED, 
  WAIT_RIGHT,
  RIGHT_LOCKED,
  EMIT,
  CONFIG
};

// Global variables
InputState currentState = WAIT_LEFT;
unsigned long lastKeyTime = 0;
unsigned long configStartTime = 0;
bool configMode = false;

// Key states (with debouncing)
bool indexPressed = false;
bool middlePressed = false;
bool ringPressed = false;
bool pinkyPressed = false;
bool thumbUpPressed = false;
bool thumbDownPressed = false;
bool thumbLeftPressed = false;
bool thumbRightPressed = false;
bool thumbPressPressed = false;

// Captured patterns
uint8_t leftPattern = 0;
uint8_t rightPattern = 0;
uint8_t fullPattern = 0;

// Braille to ASCII mapping (6-dot mode)
char brailleToAscii(uint8_t pattern) {
  switch(pattern) {
    case 0x01: return 'a';  // Dot 1
    case 0x03: return 'b';  // Dots 1,2
    case 0x09: return 'c';  // Dots 1,4
    case 0x19: return 'd';  // Dots 1,4,5
    case 0x11: return 'e';  // Dots 1,5
    case 0x0B: return 'f';  // Dots 1,2,4
    case 0x1B: return 'g';  // Dots 1,2,4,5
    case 0x13: return 'h';  // Dots 1,2,5
    case 0x0A: return 'i';  // Dots 2,4
    case 0x1A: return 'j';  // Dots 2,4,5
    case 0x05: return 'k';  // Dots 1,3
    case 0x07: return 'l';  // Dots 1,2,3
    case 0x0D: return 'm';  // Dots 1,3,4
    case 0x1D: return 'n';  // Dots 1,3,4,5
    case 0x15: return 'o';  // Dots 1,3,5
    case 0x0F: return 'p';  // Dots 1,2,3,4
    case 0x1F: return 'q';  // Dots 1,2,3,4,5
    case 0x17: return 'r';  // Dots 1,2,3,5
    case 0x0E: return 's';  // Dots 2,3,4
    case 0x1E: return 't';  // Dots 2,3,4,5
    case 0x25: return 'u';  // Dots 1,3,6
    case 0x27: return 'v';  // Dots 1,2,3,6
    case 0x3A: return 'w';  // Dots 2,4,5,6
    case 0x2D: return 'x';  // Dots 1,3,4,6
    case 0x3D: return 'y';  // Dots 1,3,4,5,6
    case 0x35: return 'z';  // Dots 1,3,5,6
    case 0x20: return ' ';  // Space (no dots)
    case 0x08: return '\b'; // Backspace (dot 3)
    case 0x0C: return '\n'; // Enter (dots 3,4)
    default: return 0;      // Unknown pattern
  }
}

void setup() {
  // Initialize pins
  pinMode(INDEX_PIN, INPUT_PULLUP);
  pinMode(MIDDLE_PIN, INPUT_PULLUP);
  pinMode(RING_PIN, INPUT_PULLUP);
  pinMode(PINKY_PIN, INPUT_PULLUP);
  pinMode(THUMB_UP_PIN, INPUT_PULLUP);
  pinMode(THUMB_DOWN_PIN, INPUT_PULLUP);
  pinMode(THUMB_LEFT_PIN, INPUT_PULLUP);
  pinMode(THUMB_RIGHT_PIN, INPUT_PULLUP);
  pinMode(THUMB_PRESS_PIN, INPUT_PULLUP);
  pinMode(HAPTIC_PIN, OUTPUT);
  
  // Initialize USB keyboard
  Keyboard.begin();
  
  // Start serial for debugging
  Serial.begin(9600);
  Serial.println("Braille-Click initialized");
  
  // Check for config mode
  if (digitalRead(THUMB_PRESS_PIN) == LOW) {
    configStartTime = millis();
  }
}

void loop() {
  // Check for config mode entry
  if (digitalRead(THUMB_PRESS_PIN) == LOW) {
    if (millis() - configStartTime > CONFIG_HOLD_TIME) {
      enterConfigMode();
    }
  } else {
    configStartTime = millis();
  }
  
  if (configMode) {
    handleConfigMode();
    return;
  }
  
  // Scan all keys
  scanKeys();
  
  // Process state machine
  switch(currentState) {
    case WAIT_LEFT:
      handleWaitLeft();
      break;
    case LEFT_LOCKED:
      handleLeftLocked();
      break;
    case WAIT_RIGHT:
      handleWaitRight();
      break;
    case RIGHT_LOCKED:
      handleRightLocked();
      break;
    case EMIT:
      handleEmit();
      break;
  }
  
  delay(1); // Small delay for stability
}

void scanKeys() {
  // Simple debouncing - in production, use proper debounce library
  static unsigned long lastScan = 0;
  if (millis() - lastScan < DEBOUNCE_TIME) return;
  lastScan = millis();
  
  indexPressed = (digitalRead(INDEX_PIN) == LOW);
  middlePressed = (digitalRead(MIDDLE_PIN) == LOW);
  ringPressed = (digitalRead(RING_PIN) == LOW);
  pinkyPressed = (digitalRead(PINKY_PIN) == LOW);
  thumbUpPressed = (digitalRead(THUMB_UP_PIN) == LOW);
  thumbDownPressed = (digitalRead(THUMB_DOWN_PIN) == LOW);
  thumbLeftPressed = (digitalRead(THUMB_LEFT_PIN) == LOW);
  thumbRightPressed = (digitalRead(THUMB_RIGHT_PIN) == LOW);
  thumbPressPressed = (digitalRead(THUMB_PRESS_PIN) == LOW);
}

void handleWaitLeft() {
  // Check if any finger keys are pressed
  if (indexPressed || middlePressed || ringPressed || pinkyPressed) {
    lastKeyTime = millis();
    return;
  }
  
  // If keys were released, capture left pattern
  if (millis() - lastKeyTime > 50 && lastKeyTime > 0) {
    leftPattern = 0;
    if (indexPressed) leftPattern |= 0x01;
    if (middlePressed) leftPattern |= 0x02;
    if (ringPressed) leftPattern |= 0x04;
    if (pinkyPressed) leftPattern |= 0x08;
    
    if (leftPattern > 0) {
      currentState = LEFT_LOCKED;
      hapticFeedback(1); // Short buzz
      Serial.print("Left pattern: 0x");
      Serial.println(leftPattern, HEX);
    }
  }
}

void handleLeftLocked() {
  // Wait for right half input
  if (indexPressed || middlePressed || ringPressed || pinkyPressed) {
    lastKeyTime = millis();
    currentState = WAIT_RIGHT;
  }
  
  // Timeout - emit left-only character
  if (millis() - lastKeyTime > PHASE_TIMEOUT) {
    currentState = EMIT;
  }
}

void handleWaitRight() {
  // Check if any finger keys are pressed
  if (indexPressed || middlePressed || ringPressed || pinkyPressed) {
    lastKeyTime = millis();
    return;
  }
  
  // If keys were released, capture right pattern
  if (millis() - lastKeyTime > 50 && lastKeyTime > 0) {
    rightPattern = 0;
    if (indexPressed) rightPattern |= 0x10;
    if (middlePressed) rightPattern |= 0x20;
    if (ringPressed) rightPattern |= 0x40;
    if (pinkyPressed) rightPattern |= 0x80;
    
    fullPattern = leftPattern | rightPattern;
    currentState = EMIT;
    hapticFeedback(1); // Short buzz
    Serial.print("Right pattern: 0x");
    Serial.println(rightPattern, HEX);
  }
}

void handleRightLocked() {
  // This state is not used in current implementation
  currentState = EMIT;
}

void handleEmit() {
  // Convert braille pattern to character
  char character = brailleToAscii(fullPattern);
  
  if (character != 0) {
    // Send character via USB
    Keyboard.write(character);
    hapticFeedback(2); // Longer buzz for character emit
    Serial.print("Emitted: ");
    Serial.println(character);
  } else {
    // Unknown pattern - error buzz
    hapticFeedback(3);
    Serial.print("Unknown pattern: 0x");
    Serial.println(fullPattern, HEX);
  }
  
  // Reset for next character
  leftPattern = 0;
  rightPattern = 0;
  fullPattern = 0;
  currentState = WAIT_LEFT;
  lastKeyTime = 0;
}

void hapticFeedback(int type) {
  switch(type) {
    case 1: // Short buzz (phase lock)
      analogWrite(HAPTIC_PIN, 128);
      delay(50);
      analogWrite(HAPTIC_PIN, 0);
      break;
    case 2: // Medium buzz (character emit)
      analogWrite(HAPTIC_PIN, 200);
      delay(100);
      analogWrite(HAPTIC_PIN, 0);
      break;
    case 3: // Error buzz (unknown pattern)
      analogWrite(HAPTIC_PIN, 255);
      delay(50);
      analogWrite(HAPTIC_PIN, 0);
      delay(50);
      analogWrite(HAPTIC_PIN, 255);
      delay(50);
      analogWrite(HAPTIC_PIN, 0);
      break;
  }
}

void enterConfigMode() {
  configMode = true;
  Serial.println("Configuration mode entered");
  hapticFeedback(3); // Error buzz to indicate config mode
}

void handleConfigMode() {
  // Simple config mode - use thumb directions to adjust settings
  // This is a placeholder - implement based on your needs
  
  if (thumbPressPressed) {
    configMode = false;
    Serial.println("Configuration mode exited");
    hapticFeedback(2);
  }
}

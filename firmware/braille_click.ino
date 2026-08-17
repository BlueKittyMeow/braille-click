/*
 * Braille-Click Firmware
 * One-handed semi-chording input device based on braille patterns
 *
 * Hardware: Arduino Pro Micro (ATmega32U4)
 *   - 4 finger keys (Box Jade switches) on internal pull-ups
 *   - Analog thumb joystick (Keyes-style: X/Y pots + push button)
 *   - Coin vibration motor via NPN transistor on a PWM pin
 *
 * Input model (6-dot mode):
 *   Phase 1 (left column):  index=dot1, middle=dot2, ring=dot3
 *   Phase 2 (right column): index=dot4, middle=dot5, ring=dot6
 *   Pinky is reserved (dots 4/8 in future 8-dot mode).
 *
 *   A phase is captured by ACCUMULATING every key that goes down while
 *   any key is held, and committing the mask once all keys are released.
 *   (Reading the keys after release doesn't work — they're all up by then.)
 *
 * Thumb functions:
 *   Press  = Space
 *   Up     = Enter
 *   Down   = Backspace
 *   Left   = Cancel current character (abandon captured left half)
 *   Right  = reserved (future: number/mode prefix)
 */

#include <Keyboard.h>

// ---------------------------------------------------------------------------
// Pin map — real Pro Micro breakout: digital 2-10, 14-16, analog A0-A3.
// PWM is only available on 3, 5, 6, 9, 10; the haptic motor needs one.
// ---------------------------------------------------------------------------
const int INDEX_PIN   = 2;
const int MIDDLE_PIN  = 3;
const int RING_PIN    = 4;
const int PINKY_PIN   = 5;
const int JOY_BTN_PIN = 7;    // joystick push switch (SW), to GND
const int HAPTIC_PIN  = 9;    // PWM: transistor base via ~330R-1k
const int JOY_X_PIN   = A0;   // joystick VRx
const int JOY_Y_PIN   = A1;   // joystick VRy

// ---------------------------------------------------------------------------
// Tuning
// ---------------------------------------------------------------------------
const unsigned long DEBOUNCE_TIME    = 10;   // ms of stable reading to accept
const unsigned long RELEASE_SETTLE   = 40;   // ms all-keys-up before committing a phase
const unsigned long PHASE_TIMEOUT    = 600;  // ms after left half before emitting left-only
const unsigned long CONFIG_HOLD_TIME = 3000; // ms joystick press to enter config

// Analog joystick thresholds (10-bit ADC, center ~512).
// A direction engages past the outer threshold and releases inside the
// inner one — the gap is hysteresis so a wobbly center can't chatter.
const int JOY_ENGAGE_LO  = 250;
const int JOY_ENGAGE_HI  = 774;
const int JOY_RELEASE_LO = 400;
const int JOY_RELEASE_HI = 624;

// ---------------------------------------------------------------------------
// State machine
// ---------------------------------------------------------------------------
enum InputState {
  IDLE,           // nothing captured, waiting for first finger press
  CAPTURE_LEFT,   // fingers down, accumulating left-column mask
  WAIT_RIGHT,     // left half locked; waiting for right half or timeout
  CAPTURE_RIGHT   // fingers down again, accumulating right-column mask
};

InputState currentState = IDLE;
uint8_t leftPattern  = 0;
uint8_t captureMask  = 0;   // accumulates while keys are held
unsigned long allUpSince   = 0;
unsigned long phaseTimeRef = 0;

// ---------------------------------------------------------------------------
// Debounced finger keys
// ---------------------------------------------------------------------------
const int NUM_FINGERS = 4;
const int fingerPins[NUM_FINGERS] = {INDEX_PIN, MIDDLE_PIN, RING_PIN, PINKY_PIN};
bool fingerState[NUM_FINGERS]      = {false, false, false, false}; // debounced
bool fingerLastRaw[NUM_FINGERS]    = {false, false, false, false};
unsigned long fingerChangeAt[NUM_FINGERS] = {0, 0, 0, 0};

// Joystick button (debounced) + press-edge tracking
bool joyBtnState = false, joyBtnLastRaw = false, joyBtnPrev = false;
unsigned long joyBtnChangeAt = 0;
unsigned long joyBtnHeldSince = 0;

// Joystick directions with hysteresis + edge tracking
// index: 0=up 1=down 2=left 3=right
bool joyDir[4]     = {false, false, false, false};
bool joyDirPrev[4] = {false, false, false, false};

bool configMode = false;
bool configArmed = true; // blocks re-trigger until button released

// ---------------------------------------------------------------------------
// Non-blocking haptic player
// ---------------------------------------------------------------------------
struct HapticStep { uint8_t level; uint16_t ms; };

const HapticStep PATTERN_LOCK[]  = {{128, 50}};                            // phase locked
const HapticStep PATTERN_EMIT[]  = {{200, 100}};                           // character sent
const HapticStep PATTERN_ERROR[] = {{255, 50}, {0, 60}, {255, 50}};        // unknown pattern
const HapticStep PATTERN_CANCEL[] = {{90, 30}, {0, 40}, {90, 30}};         // input cancelled

const HapticStep* hapticPattern = nullptr;
uint8_t hapticSteps = 0, hapticIndex = 0;
unsigned long hapticStepStart = 0;

void playHaptic(const HapticStep* pattern, uint8_t steps) {
  hapticPattern = pattern;
  hapticSteps = steps;
  hapticIndex = 0;
  hapticStepStart = millis();
  analogWrite(HAPTIC_PIN, pattern[0].level);
}

void updateHaptic() {
  if (hapticPattern == nullptr) return;
  if (millis() - hapticStepStart < hapticPattern[hapticIndex].ms) return;
  hapticIndex++;
  if (hapticIndex >= hapticSteps) {
    analogWrite(HAPTIC_PIN, 0);
    hapticPattern = nullptr;
  } else {
    analogWrite(HAPTIC_PIN, hapticPattern[hapticIndex].level);
    hapticStepStart = millis();
  }
}

#define PLAY(p) playHaptic(p, sizeof(p) / sizeof(p[0]))

// ---------------------------------------------------------------------------
// Braille → ASCII (6-dot). Bit n = dot n+1: dots 1-3 = bits 0-2 (left
// column), dots 4-6 = bits 3-5 (right column).
// ---------------------------------------------------------------------------
char brailleToAscii(uint8_t pattern) {
  switch (pattern) {
    case 0x01: return 'a';  // 1
    case 0x03: return 'b';  // 1,2
    case 0x09: return 'c';  // 1,4
    case 0x19: return 'd';  // 1,4,5
    case 0x11: return 'e';  // 1,5
    case 0x0B: return 'f';  // 1,2,4
    case 0x1B: return 'g';  // 1,2,4,5
    case 0x13: return 'h';  // 1,2,5
    case 0x0A: return 'i';  // 2,4
    case 0x1A: return 'j';  // 2,4,5
    case 0x05: return 'k';  // 1,3
    case 0x07: return 'l';  // 1,2,3
    case 0x0D: return 'm';  // 1,3,4
    case 0x1D: return 'n';  // 1,3,4,5
    case 0x15: return 'o';  // 1,3,5
    case 0x0F: return 'p';  // 1,2,3,4
    case 0x1F: return 'q';  // 1,2,3,4,5
    case 0x17: return 'r';  // 1,2,3,5
    case 0x0E: return 's';  // 2,3,4
    case 0x1E: return 't';  // 2,3,4,5
    case 0x25: return 'u';  // 1,3,6
    case 0x27: return 'v';  // 1,2,3,6
    case 0x3A: return 'w';  // 2,4,5,6
    case 0x2D: return 'x';  // 1,3,4,6
    case 0x3D: return 'y';  // 1,3,4,5,6
    case 0x35: return 'z';  // 1,3,5,6
    default:   return 0;    // unknown pattern
  }
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
  for (int i = 0; i < NUM_FINGERS; i++) pinMode(fingerPins[i], INPUT_PULLUP);
  pinMode(JOY_BTN_PIN, INPUT_PULLUP);
  pinMode(HAPTIC_PIN, OUTPUT);
  analogWrite(HAPTIC_PIN, 0);

  Keyboard.begin();
  Serial.begin(9600);
  Serial.println("Braille-Click initialized");
}

// ---------------------------------------------------------------------------
// Input scanning
// ---------------------------------------------------------------------------
void scanKeys() {
  unsigned long now = millis();

  // Fingers: per-key debounce — accept a reading once stable for DEBOUNCE_TIME
  for (int i = 0; i < NUM_FINGERS; i++) {
    bool raw = (digitalRead(fingerPins[i]) == LOW);
    if (raw != fingerLastRaw[i]) {
      fingerLastRaw[i] = raw;
      fingerChangeAt[i] = now;
    } else if (raw != fingerState[i] && now - fingerChangeAt[i] >= DEBOUNCE_TIME) {
      fingerState[i] = raw;
    }
  }

  // Joystick button, same scheme
  bool raw = (digitalRead(JOY_BTN_PIN) == LOW);
  if (raw != joyBtnLastRaw) {
    joyBtnLastRaw = raw;
    joyBtnChangeAt = now;
  } else if (raw != joyBtnState && now - joyBtnChangeAt >= DEBOUNCE_TIME) {
    joyBtnState = raw;
    if (joyBtnState) joyBtnHeldSince = now;
  }

  // Joystick axes → four directions with hysteresis
  int x = analogRead(JOY_X_PIN);
  int y = analogRead(JOY_Y_PIN);

  if (!joyDir[0] && y < JOY_ENGAGE_LO)  joyDir[0] = true;   // up
  if ( joyDir[0] && y > JOY_RELEASE_LO) joyDir[0] = false;
  if (!joyDir[1] && y > JOY_ENGAGE_HI)  joyDir[1] = true;   // down
  if ( joyDir[1] && y < JOY_RELEASE_HI) joyDir[1] = false;
  if (!joyDir[2] && x < JOY_ENGAGE_LO)  joyDir[2] = true;   // left
  if ( joyDir[2] && x > JOY_RELEASE_LO) joyDir[2] = false;
  if (!joyDir[3] && x > JOY_ENGAGE_HI)  joyDir[3] = true;   // right
  if ( joyDir[3] && x < JOY_RELEASE_HI) joyDir[3] = false;
}

bool anyFingerDown() {
  for (int i = 0; i < NUM_FINGERS; i++)
    if (fingerState[i]) return true;
  return false;
}

// Accumulate currently-held fingers into the capture mask for a phase.
// rightPhase shifts dots into the right-column bit positions (dots 4-6).
void accumulate(bool rightPhase) {
  uint8_t shift = rightPhase ? 3 : 0;
  if (fingerState[0]) captureMask |= (0x01 << shift); // index:  dot 1 / 4
  if (fingerState[1]) captureMask |= (0x02 << shift); // middle: dot 2 / 5
  if (fingerState[2]) captureMask |= (0x04 << shift); // ring:   dot 3 / 6
  // Pinky reserved for 8-dot mode; ignored in 6-dot.
}

// Edge helper: true exactly once when a joystick direction newly engages
bool dirPressed(int d) {
  bool edge = joyDir[d] && !joyDirPrev[d];
  return edge;
}

void resetInput() {
  leftPattern = 0;
  captureMask = 0;
  currentState = IDLE;
}

void emitPattern(uint8_t pattern) {
  char c = brailleToAscii(pattern);
  if (c != 0) {
    Keyboard.write(c);
    PLAY(PATTERN_EMIT);
    Serial.print("Emitted: ");
    Serial.println(c);
  } else {
    PLAY(PATTERN_ERROR);
    Serial.print("Unknown pattern: 0x");
    Serial.println(pattern, HEX);
  }
  resetInput();
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
void loop() {
  scanKeys();
  updateHaptic();
  unsigned long now = millis();

  // ---- Config mode: hold joystick press 3s to enter, press again to exit
  if (configMode) {
    handleConfigMode();
    joyBtnPrev = joyBtnState;
    for (int d = 0; d < 4; d++) joyDirPrev[d] = joyDir[d];
    return;
  }
  if (joyBtnState && configArmed && now - joyBtnHeldSince >= CONFIG_HOLD_TIME) {
    configMode = true;
    configArmed = false; // don't re-enter until button released
    resetInput();
    Serial.println("Configuration mode entered");
    PLAY(PATTERN_ERROR);
  }
  if (!joyBtnState) configArmed = true;

  // ---- Thumb actions (edge-triggered, only while not mid-capture)
  if (currentState == IDLE) {
    // Space fires on release of a short press, so a config-mode hold
    // doesn't also type a space first.
    if (!joyBtnState && joyBtnPrev && now - joyBtnHeldSince < CONFIG_HOLD_TIME) {
      Keyboard.write(' ');
      PLAY(PATTERN_EMIT);
    }
    if (dirPressed(0)) { Keyboard.write('\n'); PLAY(PATTERN_EMIT); }   // up = enter
    if (dirPressed(1)) { Keyboard.write('\b'); PLAY(PATTERN_EMIT); }   // down = backspace
  }
  // Cancel works from any capture state
  if (currentState != IDLE && dirPressed(2)) {                          // left = cancel
    resetInput();
    PLAY(PATTERN_CANCEL);
    Serial.println("Cancelled");
  }

  // ---- Character state machine
  switch (currentState) {
    case IDLE:
      if (anyFingerDown()) {
        captureMask = 0;
        accumulate(false);
        currentState = CAPTURE_LEFT;
      }
      break;

    case CAPTURE_LEFT:
      if (anyFingerDown()) {
        accumulate(false);      // keep collecting keys as they join the chord
        allUpSince = 0;
      } else {
        if (allUpSince == 0) allUpSince = now;
        if (now - allUpSince >= RELEASE_SETTLE) {
          leftPattern = captureMask;
          captureMask = 0;
          currentState = WAIT_RIGHT;
          phaseTimeRef = now;
          PLAY(PATTERN_LOCK);
          Serial.print("Left half: 0x");
          Serial.println(leftPattern, HEX);
        }
      }
      break;

    case WAIT_RIGHT:
      if (anyFingerDown()) {
        accumulate(true);
        currentState = CAPTURE_RIGHT;
        allUpSince = 0;
      } else if (now - phaseTimeRef >= PHASE_TIMEOUT) {
        emitPattern(leftPattern);  // left-only character (a, b, k, l, ...)
      }
      break;

    case CAPTURE_RIGHT:
      if (anyFingerDown()) {
        accumulate(true);
        allUpSince = 0;
      } else {
        if (allUpSince == 0) allUpSince = now;
        if (now - allUpSince >= RELEASE_SETTLE) {
          Serial.print("Right half: 0x");
          Serial.println(captureMask, HEX);
          emitPattern(leftPattern | captureMask);
        }
      }
      break;
  }

  // Edge state for next pass
  joyBtnPrev = joyBtnState;
  for (int d = 0; d < 4; d++) joyDirPrev[d] = joyDir[d];
}

// ---------------------------------------------------------------------------
// Config mode — placeholder: exits on a fresh button press.
// Future: joystick up/down to pick a setting, left/right to adjust
// (phase timeout, haptic intensity, 6/8-dot mode).
// ---------------------------------------------------------------------------
void handleConfigMode() {
  if (joyBtnState && !joyBtnPrev) {
    configMode = false;
    Serial.println("Configuration mode exited");
    PLAY(PATTERN_EMIT);
  }
}

# Braille-Click Technical Documentation

## 8-Bit Braille Mask Explanation

### What is the 8-bit mask?
The 8-bit mask is how we encode braille patterns as numbers that the computer can understand. Think of it as a binary representation of which dots are "pressed" in a braille cell.

### Braille Dot Positions
```
6-dot braille:    8-dot braille:
1 4              1 5
2 5              2 6  
3 6              3 7
                 4 8
```

### How the mask works:
- Each dot position gets a bit (0 or 1)
- 0 = dot not pressed, 1 = dot pressed
- We combine all bits into one 8-bit number

### Example: Letter "A" in 6-dot braille
- Braille pattern: Dot 1 only (top-left)
- Binary representation: `00000001` (only bit 0 is set)
- Decimal value: 1
- Unicode: U+2801 (U+2800 + 1)

### Example: Letter "B" in 6-dot braille  
- Braille pattern: Dots 1 and 2 (top-left and middle-left)
- Binary representation: `00000011` (bits 0 and 1 are set)
- Decimal value: 3
- Unicode: U+2803 (U+2800 + 3)

### Code Implementation:

The bit-to-dot assignment depends on the mode, because 6-dot and 8-dot
braille number their columns differently (6-dot: 1-3 left / 4-6 right;
8-dot: 1-4 left / 5-8 right). The lookup table and the capture code must
agree on the same convention -- bit n = dot n+1:

```cpp
// 6-dot mode: dots 1-3 = bits 0-2 (left phase), dots 4-6 = bits 3-5 (right phase)
uint8_t leftHalf = 0;
if (indexPressed)  leftHalf |= 0x01;  // Bit 0 (dot 1)
if (middlePressed) leftHalf |= 0x02;  // Bit 1 (dot 2)
if (ringPressed)   leftHalf |= 0x04;  // Bit 2 (dot 3)

uint8_t rightHalf = 0;
if (indexPressed)  rightHalf |= 0x08; // Bit 3 (dot 4)
if (middlePressed) rightHalf |= 0x10; // Bit 4 (dot 5)
if (ringPressed)   rightHalf |= 0x20; // Bit 5 (dot 6)

uint8_t fullMask = leftHalf | rightHalf;
```

In 8-dot mode the pinky joins in and the right column shifts up: left
phase sets bits 0-3 (dots 1-4), right phase sets bits 4-7 (dots 5-8).

**Capture timing gotcha:** the pattern must be *accumulated while keys are
held* (OR each newly-pressed key into a running mask) and committed once
all keys are released -- reading the key states after release yields an
empty mask, since everything is already up.

## Hardware Specifications

### Microcontroller Options

#### Option 1: Arduino Pro Micro (ATmega32U4) - RECOMMENDED
- **Pros**: Built-in USB HID, Arduino IDE compatible, 5V tolerant
- **Cons**: Limited GPIO, no wireless
- **Price**: ~$10-15
- **GPIO Available**: 9 digital pins

#### Option 2: Raspberry Pi Pico (RP2040)
- **Pros**: More GPIO, faster, cheaper
- **Cons**: Requires TinyUSB library for HID
- **Price**: ~$4
- **GPIO Available**: 26 digital pins

#### Option 3: nRF52840 (Wireless)
- **Pros**: BLE keyboard support, more features
- **Cons**: More complex, higher power consumption
- **Price**: ~$15-25
- **GPIO Available**: 20+ digital pins

### Pin Assignments (Arduino Pro Micro)

A real Pro Micro breaks out digital pins 2-10 and 14-16 plus analog
A0-A3. PWM is only available on pins 3, 5, 6, 9, and 10 -- the haptic
motor must live on one of those.

```
Finger Keys (Digital Input, internal pull-ups -- no external resistors):
- Index Finger:   Pin 2
- Middle Finger:  Pin 3
- Ring Finger:    Pin 4
- Pinky:          Pin 5

Thumb Joystick (Keyes-style analog module):
- VRx (X axis):   A0
- VRy (Y axis):   A1
- SW  (press):    Pin 7 (internal pull-up)

Haptic Motor:
- Motor Control:  Pin 9 (PWM) via NPN transistor

Power:
- VCC: 5V (from USB)
- GND: Common ground
```

The analog joystick uses 3 pins instead of the 5 a digital 5-way switch
would need, and the self-centering stick gives tactile "home position"
feedback for free. Directions are derived in firmware by thresholding
the ADC readings with hysteresis.

### Haptic Motor Circuit

```
VCC (5V) ---+----[Coin Motor]----+---- NPN Collector
            |                    |
            +---|<|--(1N4148)----+      <- flyback diode ACROSS the motor,
                 cathode to VCC            cathode (banded end) to VCC

Pin 9 ----[330R-1k Resistor]---- NPN Base

NPN Emitter ---- GND
```

The flyback diode goes **across the motor terminals** (cathode toward
VCC), not anywhere near the base -- it absorbs the inductive spike when
the transistor switches off.

**Parts Needed:**
- 1x NPN Transistor (2N2222 / PN2222A or similar)
- 1x 330R-1k base resistor (330R preferred: guarantees saturation)
- 1x Coin Vibration Motor (3V-5V)
- 1x Flyback Diode (1N4148) -- cheap insurance, use it

### Construction Guide

#### Step 1: Key Layout
```
Top View (held in right hand):
    [Pinky] [Ring] [Middle] [Index]
    
    [Thumb Toggle/Joystick]
```

#### Step 2: Wiring
1. **Mount switches** on cardboard/perfboard at comfortable angles
2. **Wire all switch commons** to GND
3. **Wire each switch signal** to its assigned GPIO pin (the firmware
   enables internal pull-ups -- no external pull-up resistors needed)
4. **Mount the joystick** where the thumb naturally rests; wire VRx/VRy
   to A0/A1, SW to pin 7, plus VCC and GND
5. **Install haptic motor** where it can be felt through the case

#### Step 3: Software Setup
1. **Install Arduino IDE** with Pro Micro support
2. **Install libraries**: Keyboard.h (built-in), HID.h (built-in)
3. **Upload firmware** (see firmware section below)
4. **Test each key** with serial monitor
5. **Calibrate timing** via configuration mode

### Firmware Architecture

#### State Machine
```cpp
enum InputState {
  IDLE,           // Nothing captured, waiting for first finger press
  CAPTURE_LEFT,   // Fingers down, accumulating left-column mask
  WAIT_RIGHT,     // Left half locked; waiting for right half or timeout
  CAPTURE_RIGHT   // Fingers down again, accumulating right-column mask
};
```

A phase's pattern is accumulated while keys are held and committed once
all keys have been released for a short settle time. If no right half
arrives within the phase timeout, the left half is emitted alone
(left-only letters: a, b, k, l, ...). Config mode is a separate flag
entered by holding the joystick press for 3 seconds.

#### Key Functions
- `scanKeys()`: Debounce finger keys and joystick button; threshold the
  joystick axes into up/down/left/right with hysteresis
- `accumulate()`: OR currently-held fingers into the capture mask
  (left or right column bit positions)
- `emitPattern()`: Convert braille mask to character, send via USB HID,
  fire haptic feedback
- `playHaptic()` / `updateHaptic()`: Non-blocking vibration patterns
  (blocking delay()s would freeze key scanning mid-buzz)
- `brailleToAscii()`: Convert braille mask to ASCII character

#### Thumb Functions
- **Press (short)**: Space (fires on release, so a config-mode hold
  doesn't also type a space)
- **Up**: Enter
- **Down**: Backspace
- **Left**: Cancel the current character mid-entry (also clears pending
  capital/number indicators)
- **Right**: Skip the left half -- locks an empty left column so the next
  chord captures as the right column. This is how right-column-only
  cells are entered (every letter a-z has at least one left-column dot,
  but indicators like the capital sign are right-only). If no right
  half follows within the phase timeout, the skip quietly cancels.

#### Indicators
- **Capital (dot 6 alone)**: sets a caps-next flag; the next letter is
  emitted uppercase
- **Number (dots 3,4,5,6)**: enters number mode; a-j emit 1-0 until a
  space or enter terminates it

### Braille Character Mapping

#### 6-Cell Mode (Standard)
```
A = 1     (dot 1 only)
B = 1,2   (dots 1,2)
C = 1,4   (dots 1,4)
D = 1,4,5 (dots 1,4,5)
E = 1,5   (dots 1,5)
F = 1,2,4 (dots 1,2,4)
G = 1,2,4,5 (dots 1,2,4,5)
H = 1,2,5 (dots 1,2,5)
I = 2,4   (dots 2,4)
J = 2,4,5 (dots 2,4,5)
...and so on
```

#### 8-Cell Mode (Extended)
- Uses all 8 dots for more characters
- Includes accented characters, symbols
- Requires pinky finger for dots 4 and 8

### Configuration Mode

**Enter**: Hold joystick press for 3 seconds (any time)
**Navigate**: Use thumb directions
**Settings**:
- Phase timeout (default 600ms)
- Haptic intensity (0-255)
- Mode selection (6-cell/8-cell)
- Key repeat prevention (on/off)

### Troubleshooting

#### Common Issues
1. **Keys not registering**: Check wiring and that switch commons go to GND (pull-ups are internal)
2. **USB not recognized**: Check Pro Micro drivers, try different USB port
3. **Haptic not working**: Check transistor wiring, motor polarity
4. **Character errors**: Verify braille mapping, check debounce timing
5. **Phase confusion**: Increase timeout, add visual feedback

#### Testing Procedure
1. **Hardware test**: Use serial monitor to verify each key
2. **Timing test**: Measure phase transition times
3. **Character test**: Input known patterns, verify output
4. **Ergonomics test**: Use for 30+ minutes, check comfort
5. **Compatibility test**: Try with different operating systems

### Parts List

#### Essential Components
- 1x Arduino Pro Micro (ATmega32U4)
- 4x Box Jade switches (or similar tactile switches)
- 1x Analog thumb joystick module (Keyes-style: VRx/VRy/SW)
- 1x Coin vibration motor (3V-5V)
- 1x NPN transistor (2N2222 / PN2222A)
- 1x 330R-1k base resistor
- 1x 1N4148 diode (flyback protection, across the motor)
- Wire, perfboard, cardboard for case

No external pull-up resistors are needed -- the ATmega32U4's internal
pull-ups are enabled in firmware for every switch input.


#### Optional Upgrades
- 1x LiPo battery (3.7V, 500mAh+)
- 1x Battery charger module
- 1x On/off switch
- 1x Status LED
- 1x 3D printed case (future upgrade)

### Cost Estimate
- **Basic Version**: $25-35
- **With Battery**: $35-45  
- **Professional**: $50-75 (with 3D printed case)

### Next Steps
1. **Order parts** from your preferred supplier
2. **Build basic prototype** on cardboard
3. **Test with simple firmware** (just key detection)
4. **Implement braille mapping** and USB HID
5. **Add haptic feedback** and error prevention
6. **Refine ergonomics** and timing
7. **Create final documentation** and user guide

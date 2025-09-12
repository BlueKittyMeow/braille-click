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
```cpp
// Build the 8-bit mask from finger inputs
uint8_t leftHalf = 0;
if (indexPressed) leftHalf |= 0x01;  // Bit 0 (dot 1)
if (middlePressed) leftHalf |= 0x02; // Bit 1 (dot 2)  
if (ringPressed) leftHalf |= 0x04;   // Bit 2 (dot 3)
if (pinkyPressed) leftHalf |= 0x08;  // Bit 3 (dot 4) - 8-dot only

uint8_t rightHalf = 0;
if (indexPressed) rightHalf |= 0x10; // Bit 4 (dot 5)
if (middlePressed) rightHalf |= 0x20; // Bit 5 (dot 6)
if (ringPressed) rightHalf |= 0x40;  // Bit 6 (dot 7)
if (pinkyPressed) rightHalf |= 0x80; // Bit 7 (dot 8) - 8-dot only

uint8_t fullMask = leftHalf | rightHalf;
```

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

```
Finger Keys (Digital Input with Pull-up):
- Index Finger:    Pin 2 (PD1)
- Middle Finger:   Pin 3 (PD0) 
- Ring Finger:     Pin 4 (PD4)
- Pinky:          Pin 5 (PC6)

Thumb Controls (Digital Input with Pull-up):
- Thumb Up:       Pin 6 (PD7)
- Thumb Down:     Pin 7 (PE6)
- Thumb Left:     Pin 8 (PB4)
- Thumb Right:    Pin 9 (PB5)
- Thumb Press:    Pin 10 (PB6)

Haptic Motor (PWM Output):
- Motor Control:  Pin 11 (PB7) via NPN transistor

Power:
- VCC: 5V (from USB)
- GND: Common ground
```

### Haptic Motor Circuit

```
VCC (5V) ----[Motor]----[NPN Collector]
                    |
                    [NPN Emitter]----GND
                    |
              [1kΩ Resistor]----Pin 11 (PB7)
                    |
              [NPN Base]
```

**Parts Needed:**
- 1x NPN Transistor (2N2222 or similar)
- 1x 1kΩ Resistor
- 1x Coin Vibration Motor (3V-5V)
- 1x Flyback Diode (1N4148) - optional but recommended

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
3. **Wire each switch signal** to its assigned GPIO pin
4. **Add pull-up resistors** (10kΩ) between each signal pin and VCC
5. **Mount thumb control** for easy thumb access
6. **Install haptic motor** where it can be felt through the case

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
  WAIT_LEFT,      // Waiting for left half input
  LEFT_LOCKED,    // Left half captured, waiting for right
  WAIT_RIGHT,     // Waiting for right half input  
  RIGHT_LOCKED,   // Right half captured, ready to emit
  EMIT,           // Sending character via USB
  CONFIG          // Configuration mode
};
```

#### Key Functions
- `scanKeys()`: Read all inputs with debouncing
- `processLeftHalf()`: Capture and lock left half pattern
- `processRightHalf()`: Capture and lock right half pattern
- `emitCharacter()`: Convert braille mask to character and send via USB
- `hapticFeedback()`: Control vibration motor
- `brailleToAscii()`: Convert braille pattern to ASCII character

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

**Enter**: Hold thumb press for 3 seconds at startup
**Navigate**: Use thumb directions
**Settings**:
- Phase timeout (100-500ms)
- Haptic intensity (0-255)
- Mode selection (6-cell/8-cell)
- Key repeat prevention (on/off)

### Troubleshooting

#### Common Issues
1. **Keys not registering**: Check wiring, pull-up resistors
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
- 1x 5-way joystick or 4x momentary switches for thumb
- 1x Coin vibration motor (3V-5V)
- 1x NPN transistor (2N2222)
- 1x 1kΩ resistor
- 4x 10kΩ pull-up resistors
- 1x 1N4148 diode (flyback protection)
- Wire, perfboard, cardboard for case

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

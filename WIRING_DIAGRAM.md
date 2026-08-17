# Braille-Click Wiring Diagram

## Arduino Pro Micro — real pins only

The Pro Micro breaks out **digital 2–10 and 14–16**, plus **analog A0–A3**.
There is no pin 11/12/13 pad on the board. PWM is available only on
pins **3, 5, 6, 9, 10** — the haptic motor lives on pin 9.

```
                 USB
              ┌───┴───┐
       TX0/1 ─┤       ├─ RAW
       RX1/0 ─┤       ├─ GND
         GND ─┤  Pro  ├─ RST
         GND ─┤ Micro ├─ VCC (5V)
   D2 Index  ─┤       ├─ A3
   D3 Middle ─┤       ├─ A2
   D4 Ring   ─┤       ├─ A1  ← Joystick VRy
   D5 Pinky  ─┤       ├─ A0  ← Joystick VRx
   D6        ─┤       ├─ D15
   D7 Joy SW ─┤       ├─ D14
   D8        ─┤       ├─ D16
   D9 Haptic ─┤       ├─ D10
              └───────┘
```

## Switch Wiring

### Finger Keys (4× Box Jade switches)

The firmware enables the ATmega32U4's **internal pull-ups**
(`INPUT_PULLUP`), so each switch needs exactly two wires and
**no external resistors**:

```
Each switch:
- One leg  → GND
- Other leg → its Arduino pin (D2, D3, D4, D5)

Pressed = pin reads LOW.
```

### Thumb Joystick (Keyes-style analog module)

```
Joystick module    Pro Micro
    VCC (+5V)  →   VCC
    GND        →   GND
    VRx        →   A0
    VRy        →   A1
    SW         →   D7   (internal pull-up; pressed = LOW)
```

The stick self-centers, so "neutral" is always findable by feel.
Firmware thresholds the ADC readings into up/down/left/right with
hysteresis (engage past ~25% deflection, release near center), so
a wobbly center can't chatter.

## Haptic Motor Circuit

```
 VCC (5V) ──┬──────[Coin Motor]──────┬──── NPN Collector
            │                        │
            └────────|<|─────────────┘
              1N4148 flyback diode
              (cathode/banded end toward VCC)

 D9 ────[330R–1k]──── NPN Base

 NPN Emitter ──── GND
```

Two rules that matter:

1. **The flyback diode goes across the motor terminals** — cathode
   (banded end) toward VCC. It absorbs the inductive voltage spike when
   the transistor switches off. It does NOT connect to the base.
2. **The base resistor (330R–1k) sits between D9 and the base.** 330R
   gives ~13mA of base drive, comfortably saturating a PN2222A for a
   small coin motor.

## Complete Connection Table

| Signal | Pro Micro pin | Notes |
|---|---|---|
| Index finger | D2 | switch to GND, internal pull-up |
| Middle finger | D3 | switch to GND, internal pull-up |
| Ring finger | D4 | switch to GND, internal pull-up |
| Pinky | D5 | switch to GND, internal pull-up (reserved for 8-dot mode) |
| Joystick SW | D7 | to GND when pressed, internal pull-up |
| Haptic motor | D9 | PWM → 330R–1k → NPN base |
| Joystick VRx | A0 | analog |
| Joystick VRy | A1 | analog |
| Joystick VCC | VCC | |
| Motor + circuit VCC | VCC | |
| All grounds | GND | common ground |

## Construction Tips

1. **Switch Mounting**: Mount switches at comfortable angles for finger access
2. **Thumb Position**: Place the joystick where the thumb naturally rests
3. **Haptic Placement**: Position motor where it can be felt through the case
4. **Wire Management**: Use short, flexible wires to avoid interference
5. **Ground Plane**: Connect all grounds together for stable operation

## Testing Procedure

1. **Power Test**: Connect USB, verify 5V on VCC pin
2. **Key Test**: Use serial monitor to verify each key press
3. **Joystick Test**: Watch the serial monitor while deflecting each direction
4. **Haptic Test**: Trigger a phase lock, verify motor vibration
5. **USB Test**: Verify device appears as keyboard in OS
6. **Character Test**: Input known braille patterns, verify output

## Troubleshooting

- **Keys not working**: Check that switch commons go to GND and each signal wire reaches its pin (pull-ups are internal — no resistors to check)
- **USB not recognized**: Check Pro Micro drivers, try different USB port
- **Haptic not working**: Check transistor orientation (E/B/C), diode direction, and that the motor is on the collector side
- **Joystick directions swapped/inverted**: Swap VRx/VRy, or flip the threshold logic in `scanKeys()` — module orientation varies
- **Character errors**: Verify braille bit mapping (dots 1–3 = bits 0–2, dots 4–6 = bits 3–5), check debounce timing
- **Phase confusion**: Increase `PHASE_TIMEOUT`, practice the left-pause-right rhythm

# Braille-Click Wiring Diagram

## Arduino Pro Micro Pinout

```
                    ┌─────────────┐
                    │  Arduino    │
                    │  Pro Micro  │
                    │             │
    VCC (5V) ───────┤ 1           │
    GND ────────────┤ 2           │
    TX ─────────────┤ 3           │
    RX ─────────────┤ 4           │
    D2 (Index) ─────┤ 5           │
    D3 (Middle) ────┤ 6           │
    D4 (Ring) ──────┤ 7           │
    D5 (Pinky) ─────┤ 8           │
    D6 (Thumb Up) ──┤ 9           │
    D7 (Thumb Down)─┤ 10          │
    D8 (Thumb Left)─┤ 11          │
    D9 (Thumb Right)┤ 12          │
    D10 (Thumb Press)┤ 13         │
    D11 (Haptic) ───┤ 14          │
    D12 ────────────┤ 15          │
    D13 ────────────┤ 16          │
    A0 ─────────────┤ 17          │
    A1 ─────────────┤ 18          │
    A2 ─────────────┤ 19          │
    A3 ─────────────┤ 20          │
    A4 ─────────────┤ 21          │
    A5 ─────────────┤ 22          │
    A6 ─────────────┤ 23          │
    A7 ─────────────┤ 24          │
                    └─────────────┘
```

## Switch Wiring

### Finger Keys (4x Box Jade switches)
```
Each switch:
    ┌─────────┐
    │   1   2 │  ← Switch pins
    │   3   4 │
    └─────────┘

Wiring:
- Pin 1: Connect to GND
- Pin 2: Connect to Arduino pin (D2, D3, D4, D5)
- Add 10kΩ pull-up resistor between pin 2 and VCC
```

### Thumb Controls (5-way joystick or 4x switches)
```
If using 5-way joystick:
- VCC: Connect to 5V
- GND: Connect to GND  
- UP: Connect to D6
- DOWN: Connect to D7
- LEFT: Connect to D8
- RIGHT: Connect to D9
- PRESS: Connect to D10

If using 4x separate switches:
- Wire each switch like finger keys
- Connect to D6, D7, D8, D9, D10
```

## Haptic Motor Circuit

```
VCC (5V) ────[Coin Motor]───[NPN Collector]
                    │
                    [NPN Emitter]─── GND
                    │
              [1kΩ Resistor]─── D11 (HAPTIC_PIN)
                    │
              [NPN Base]
                    │
              [1N4148 Diode] (flyback protection)
                    │
                   GND
```

## Complete Wiring Schematic

```
                    Arduino Pro Micro
                    ┌─────────────────┐
                    │                 │
    VCC ────────────┤ 1               │
                    │                 │
    GND ────────────┤ 2               │
                    │                 │
    Index ──────────┤ 5  ────[10kΩ]───┤ VCC
                    │                 │
    Middle ─────────┤ 6  ────[10kΩ]───┤ VCC
                    │                 │
    Ring ───────────┤ 7  ────[10kΩ]───┤ VCC
                    │                 │
    Pinky ──────────┤ 8  ────[10kΩ]───┤ VCC
                    │                 │
    Thumb Up ───────┤ 9  ────[10kΩ]───┤ VCC
                    │                 │
    Thumb Down ─────┤ 10 ────[10kΩ]───┤ VCC
                    │                 │
    Thumb Left ─────┤ 11 ────[10kΩ]───┤ VCC
                    │                 │
    Thumb Right ────┤ 12 ────[10kΩ]───┤ VCC
                    │                 │
    Thumb Press ────┤ 13 ────[10kΩ]───┤ VCC
                    │                 │
    Haptic ─────────┤ 14              │
                    │                 │
                    └─────────────────┘
                           │
                           │
                    ┌──────┴──────┐
                    │             │
            VCC ────┤ [Coin Motor]├─── NPN Collector
                    │             │
                    └─────────────┘
                           │
                    [1kΩ Resistor]─── D11
                           │
                    [NPN Base]
                           │
                    [1N4148 Diode]─── GND
```

## Construction Tips

1. **Switch Mounting**: Mount switches at comfortable angles for finger access
2. **Thumb Position**: Place thumb control where thumb naturally rests
3. **Haptic Placement**: Position motor where it can be felt through the case
4. **Wire Management**: Use short, flexible wires to avoid interference
5. **Pull-up Resistors**: Use 10kΩ resistors for clean signal detection
6. **Ground Plane**: Connect all grounds together for stable operation

## Testing Procedure

1. **Power Test**: Connect USB, verify 5V on VCC pin
2. **Key Test**: Use serial monitor to verify each key press
3. **Haptic Test**: Send PWM signal to haptic pin, verify motor vibration
4. **USB Test**: Verify device appears as keyboard in OS
5. **Character Test**: Input known braille patterns, verify output

## Troubleshooting

- **Keys not working**: Check wiring, pull-up resistors, pin assignments
- **USB not recognized**: Check Pro Micro drivers, try different USB port
- **Haptic not working**: Check transistor wiring, motor polarity, PWM signal
- **Character errors**: Verify braille mapping, check debounce timing
- **Phase confusion**: Increase timeout, add visual feedback

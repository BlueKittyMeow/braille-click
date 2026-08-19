# Braille-Click: One-Handed Semi-Chording Input Device

A one-handed input device based on braille patterns, designed to be held in a loose fist with four fingers on keys and thumb on a toggle/joystick.

## Concept

The Braille-Click device allows users to input text using standard 6-cell or extended 8-cell braille patterns through a two-phase input system:

1. **Left Half**: Press finger combinations for the left side of a braille character
2. **Right Half**: Press the same finger combinations for the right side of a braille character
3. **Output**: Device converts the braille pattern to text and provides haptic feedback

## Key Features

- **Ergonomic Design**: Held in loose fist, four fingers on keys, thumb on a self-centering analog joystick (neutral is always findable by feel)
- **Two-Phase Input**: Left half → Right half → Character output
- **Haptic Feedback**: Gentle vibration to confirm input phases and character registration
- **Dual Modes**: 6-cell (standard) and 8-cell (extended) braille support
- **USB HID**: Works as a standard keyboard with any computer
- **Error Prevention**: Prevents "half-off" encoding through clear phase separation

## Finger Mapping

### 6-Cell Mode
- **Index Finger**: Dot 1 (top-left) / Dot 4 (top-right)
- **Middle Finger**: Dot 2 (middle-left) / Dot 5 (middle-right)  
- **Ring Finger**: Dot 3 (bottom-left) / Dot 6 (bottom-right)
- **Pinky**: Reserved (unused in 6-cell mode)

### Thumb Functions
- **Press (short)**: Space
- **Up**: Enter
- **Down**: Backspace
- **Left**: Cancel current character
- **Right**: Skip left half — enter a right-column-only cell (e.g. the capital indicator, dot 6)
- **Press (hold 3s)**: Configuration mode

### Indicators
- **Dot 6 alone** (thumb-right, then right-phase middle finger): capitalize the next letter
- **Dots 3,4,5,6** (left ring, then right index+middle+ring): number mode — a–j emit 1–0 until the next space

### 8-Cell Mode
- **Index Finger**: Dot 1 (top-left) / Dot 5 (top-right)
- **Middle Finger**: Dot 2 (middle-left) / Dot 6 (middle-right)
- **Ring Finger**: Dot 3 (bottom-left) / Dot 7 (bottom-right)
- **Pinky**: Dot 4 (bottom-left) / Dot 8 (bottom-right)

## Getting Started

1. **Hardware Setup**: Follow the construction guide in `TECHNICAL.md`
2. **Firmware**: Flash the Arduino sketch to your microcontroller
3. **Calibration**: Adjust timing and haptic settings via configuration mode
4. **Practice**: Start with simple letters (A-Z) before moving to punctuation

## Documentation

- `PLANNING.md` - Feature roadmap and development phases
- `TECHNICAL.md` - Hardware specifications, pinouts, and construction guide

## License

Open source - feel free to modify and improve!

## Contributing

This is a personal project, but suggestions and improvements are welcome!

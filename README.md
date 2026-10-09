# duckduino

My own re-implementation of duckduino. An SD-driven USB keyboard macro device. An Arduino Pro Micro reads a plain-text script from a microSD card and types it on the host over USB HID. Good for automating repetitive keystroke sequences, demos, and setup routines.

Symbols are typed in a layout-safe way, so the same script produces the right characters on different keyboard layouts (US, Spanish, and others). For academic and authorized use only.

## Hardware

- **Board:** SparkFun Pro Micro (ATmega32U4) or Arduino Leonardo. The native USB on this chip is what lets it act as a real HID keyboard. A plain Pro Mini (ATmega328) will not work.
- **Storage:** a standard microSD module with an onboard 3.3V regulator and level shifter, so it runs from 5V directly.

### Wiring

Hardware SPI is fixed on the 32U4. Only the chip select (CS) pin is a free choice.

| SD module | Pro Micro pin |
|-----------|---------------|
| VCC       | VCC (5V)      |
| GND       | GND           |
| SCK       | 15            |
| MISO      | 14            |
| MOSI      | 16            |
| CS        | 4             |

## Build and upload

1. Open `duckduino.ino` in the Arduino IDE.
2. Install the **SD** and **Keyboard** libraries.
3. Select the board: **SparkFun Pro Micro (5V, 16MHz)**, or **Arduino Leonardo** if that core is not installed.
4. Upload.

## How to use

1. Save your script as `script.txt` in the **root** of the microSD card.
2. Insert the card into the module.
3. Plug the board into the host over USB. The script runs once, top to bottom.

The repo includes a `script.txt` that is a full symbol test (punctuation, brackets, operators, pipes, quotes, accents). Copy it to the card as-is to try it. A minimal example looks like this:

```
REM opens notepad and types a line of symbols
DEFAULT_DELAY 100
DELAY 500
GUI r
DELAY 400
STRING notepad
ENTER
DELAY 1000
STRING Hello World!
```

> Save `script.txt` as ANSI / ISO-8859-1 (Latin-1), **not** UTF-8, or accented characters will type as garbage.

## Script commands

One command per line.

| Command | Effect |
|---------|--------|
| `REM ...` | Comment, ignored. |
| `STRING text` | Type the text. |
| `DELAY n` | Wait `n` milliseconds. |
| `DEFAULT_DELAY n` | Wait `n` ms after every line (also `DEFAULTDELAY n`). |
| `REPEAT n` | Run the previous line `n` more times. |
| `ALTCODES ON` | Symbols as Windows Alt+numpad codes (default). Windows only, needs NumLock on. |
| `ALTCODES LINUX` | Symbols as Linux IBus unicode entry. GUI apps only (GNOME, KDE). |
| `ALTCODES OFF` | Raw key positions. Correct only if the host layout matches. The only option on macOS. |
| key combo | One or more key names on a line, pressed together then released. |

**Key combos:** a line of key names is pressed at once, then released. A single character is pressed as that key.

```
GUI r
CTRL ALT DELETE
CTRL c
```

Supported key names: `ENTER` `GUI`/`WINDOWS` `SHIFT` `ALT` `CTRL`/`CONTROL` `TAB` `ESC`/`ESCAPE` `SPACE` `DELETE` `BACKSPACE` `HOME` `END` `INSERT` `PAGEUP` `PAGEDOWN` `CAPSLOCK` `UP` `DOWN` `LEFT` `RIGHT` `F1`–`F12`.

## Status LED

The onboard TX led (pin 30) signals state:

| LED | Meaning |
|-----|---------|
| Steady on | Running the script. |
| Off | Done. |
| Slow Pulse | Error: SD init failed (no card, bad card, bad wiring) or `script.txt` not found. |

## Keyboard layout notes

USB HID sends physical key positions, not characters. The host OS maps a position to a character using its own active layout, which the device cannot read. Letters, digits, and space share positions across US and Spanish QWERTY, so they always type correctly. Symbols and accents move between layouts, so they are sent through the mode set by `ALTCODES`:

- **Windows** (`ALTCODES ON`): Alt+numpad codes, decoded from the codepoint regardless of layout. Needs NumLock on.
- **Linux** (`ALTCODES LINUX`): IBus/GTK unicode entry. Works in most GUI apps, not terminals.
- **macOS / known layout** (`ALTCODES OFF`): raw key positions, correct only when the host layout matches.

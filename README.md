
Wolf3D-style raycaster for Arduino Nano (ATmega328P) and SPI TFT/OLED color displays
#Wolf3D-Nano

Wolf3D-style raycaster for Arduino Nano (ATmega328P) and SPI TFT/OLED color displays.

This is **not** a full Wolfenstein 3D port.
It is an experimental AVR raycasting project focused on exploring the first episode Wolf3D-style maps on tiny microcontrollers with extremely limited RAM and flash storage.

The project currently targets the **Arduino Nano / ATmega328P** running at 16MHz using small SPI displays such as the SSD1331 OLED and ST77XX TFT displays.

The goal is simple:

> How far can we push an 8-bit AVR before the hardware completely gives up? 😄

---

# Current Features

* Wolf3D-style textured raycaster
* Episode 1 maps (E1M1-E1M10)
* Multiple SPI TFT/OLED display configurations
* Indexed 8-bit textures + RGB565 palette rendering
* Doors with opening/closing support
* Collision detection
* Elevators/exits
* Optional noclip mode
* Direct SPI register rendering for speed
* Chunked rendering to fit inside 2KB RAM

---

# Supported Displays

Display selection is done in `config.h`.

Currently tested/supported configurations:


//#define ST7735_160X80_BLUE_PCB
//#define ST7735_160X80_BLACK_PCB
//#define ST7735_160X128
//#define ST7789_240X135
//#define ST7789_240X240
//#define ST7789_240X240_240X150
//#define ST7735_128X128
//#define ST7789_170X320
//#define ILI9342_320X240

// OLED displays
#define SSD1331_96X64
//#define SSD1351_128X128


---

# Performance

Approximate FPS on ATmega328P @ 16MHz:

| Display                         | FPS        |
| ------------------------------- | ---------- |
| SSD1331 96x64                   | ~9-12 FPS  |
| ST7735 128x128                  | ~5-7 FPS   |
| ST7735 128x128 with COLUMN_SKIP | ~10-11 FPS |
| ST7735 160x80                   | ~10-11 FPS |

The biggest bottleneck is currently SPI display bandwidth, not only the raycaster itself.

---

# Controls

Current prerelease versions use 4 buttons:

* Forward
* Backward
* Rotate Left
* Rotate Right

Doors currently open automatically when standing in front of them.

A dedicated action/use button will be added later.

---

# Hardware

Tested hardware:

* Arduino Nano (ATmega328P)

Experimental/planned support:

* LGT8F328P @ 32MHz
  (Recommended for future higher FPS versions)

The renderer uses direct AVR port manipulation for speed, so some boards may require adjustments.

---

# Build System

The project currently uses:

* PlatformIO

Arduino IDE compatibility is planned later.

---

# Memory Limitations

ATmega328P only has:

* 32KB flash
* 2KB RAM

After the bootloader and textures/maps are included, flash space becomes extremely limited.

Because of this:

* Only one level can currently be compiled/uploaded at a time
* Rendering is heavily optimized for low RAM usage
* Many features are intentionally simplified or removed

This project exists because trying to force Wolf3D-style rendering onto tiny 8-bit hardware is fun 😄

---

# Current Limitations

This prerelease version currently has:

* No enemy sprites
* No decorations
* No keys
* No save system
* No sound/music
* No multi-level progression

The player explores one compiled level at a time.

---

# Planned Features / Experiments

Future experimental versions may include:

* Pushwalls
* Keys and locked doors
* 8bpp SSD1331 renderer
* Floor and ceiling textures
* SPI flash asset storage
* Multi-AVR rendering systems
* Dual/Triple/Quad Arduino Nano versions
* Sprite coprocessor MCUs
* BSP experiments
* "Maybe Doom on multiple AVR chips?" 😄

The goal is not practicality.

The goal is seeing how absurdly far small AVR hardware can be pushed.

---

# Why Not ESP32/RP2040?

Because they are too easy 😄

ESP32, RP2040, ESP8266 and similar MCUs are already far more powerful than the PCs Wolf3D originally ran on.
(I already made a ESP8266/ESP32/RP2040 Wolf4SDL port)

The fun here is trying to make something barely possible on hardware that absolutely should not be doing this.

---

# Assets / Legal Notes

This project is non-commercial and experimental.

Some generated map/texture data may originate from Wolfenstein 3D shareware assets.

If needed, the asset headers can later be replaced by externally generated data/tools.

Wolfenstein 3D and related assets belong to their respective owners.

---

# License

MIT License

---

# Screenshots

(Add screenshots here later 😄)

---

# Project Status

Very experimental prerelease.

The codebase changes often while testing new rendering ideas and memory-saving techniques.

Pull requests, crazy optimization ideas, and weird AVR experiments are welcome.


# Pico C/C++ Firmware Template --- SH1107 OLED SPI Demo

Embedded systems · Control · Debugging ⚙️  
I/O 🔌, firmware 🔧, and systems that actually run 🚀

---

## Overview

This project demonstrates how to drive a **Waveshare Pico-OLED-1.3 display (SH1107 controller)**  
using **SPI communication** from the Raspberry Pi Pico / Pico 2.

It implements:

- Full framebuffer rendering
- Custom 5x7 bitmap font
- Pixel-level graphics primitives
- Real-time UI rendering
- Hardware‑accelerated SPI updates

The firmware renders a **dark‑theme embedded UI**, including:

- Status header
- System information panel
- Progress bar
- Runtime counter
- Animated UI elements

This demonstrates how to build **embedded user interfaces directly on microcontrollers**.

---

## Hardware Architecture

Display controller:

| Component | Interface |
|---------|-----------|
| SH1107 OLED Controller | SPI |
| Framebuffer | MCU RAM |
| Rendering | Software |
| Update | SPI DMA‑style blocking transfer |

Resolution:

64 × 128 pixels  
Monochrome

---

## SPI Wiring

| OLED Pin | Pico Pin |
|---------|----------|
| SCK     | GP10     |
| MOSI    | GP11     |
| CS      | GP9      |
| DC      | GP8      |
| RST     | GP12     |
| VCC     | 3.3V     |
| GND     | GND      |

SPI Port:

spi1

---

## Features

- Full framebuffer graphics engine
- Custom font rendering
- Hardware SPI at 8 MHz
- Animated UI
- Modular architecture
- Zero external libraries required

---

## Example Behavior

The firmware displays:

- AXFER‑IO themed UI
- Device status
- Animated progress bar
- System uptime counter

All rendering is performed on the Pico itself.

---

## Build and Flash

Configure:

```bash
cmake --preset pico2-release
```

Build:

```bash
cmake --build --preset build-pico2-release
```

Flash:

```bash
picotool load build/pico2-release/app.uf2 -f
picotool reboot
```

---

## Project Structure

```text
.
├── src/
│   └── main.c
├── lib/
├── build/
├── CMakeLists.txt
├── CMakePresets.json
└── README.md
```

---

## Design Principles

- Deterministic rendering
- Explicit hardware control
- No hidden abstractions
- Fully portable graphics core
- Embedded‑first architecture

---

## Supported Targets

- RP2040 (Pico / Pico W)
- RP2350 (Pico 2 / Pico 2 W)

---

## Use Cases

- Embedded UI systems
- Instrumentation displays
- Robotics dashboards
- Industrial controllers
- Debug consoles

---

## Author

**axfer-io**  
Embedded systems · Control · Debugging  
Firmware and systems that actually run.

---

## License

MIT

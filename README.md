# Silent Alert

An LD2410C mmWave radar (outside unit) streams presence/distance over RS-485 to an
ESP32 (inside unit), which shows how close someone is on a NeoPixel strip:
more LEDs and redder = closer, a single blue LED = far away, all off = nobody there.

## Firmware

```
pio run -d firmware -t upload      # build & flash an ESP32 DevKit V1
pio device monitor -d firmware     # debug output at 115200
```

Or open `firmware/silent_alarm.ino` in the Arduino IDE (board: "ESP32 Dev Module",
library: Adafruit NeoPixel).

Wiring (main board): RS-485 RO -> GPIO16, DI -> GPIO17, NeoPixel data -> GPIO4.
The radar must be at its default 256000 baud.

Host-side parser test (no hardware needed):

```
cd firmware/test && g++ -std=c++17 -I. test_parser.cpp -o test && ./test
```

## Hardware

- `hardware/*/*_netlist.py` – source of truth for the netlists
- `hardware/generate_boms.sh` – regenerates the BOMs
- `hardware/generate_gerbers.sh` – exports Gerbers/drill files (needs `kicad-cli`)

Open the `.kicad_pcb` files in KiCad and run DRC before ordering boards: the
layouts were hand-generated, not laid out against the official footprint libraries.

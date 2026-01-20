Title: Add Heltec Wireless Stick V3 (ESP32-S3) board and variant

What:
- Adds `boards/heltec_wireless_stick_v3.json` and `variants/heltec_wireless_stick_v3/pins_arduino.h`.

Why:
- Support Heltec Wireless Stick V3 (ESP32‑S3) in the boards collection so PlatformIO users can select the board by name.

Testing performed:
- Built the `LoRa-RX` project locally using the same board/variant files and verified `pio run -e serial -v` completed and produced `firmware.bin`.

Notes:
- No firmware/source changes included — only board / variant metadata and pin mapping.
- If maintainers prefer different naming or metadata fields, I can update the files.

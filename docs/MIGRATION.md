# Compatibility notes

- The v1 EEPROM layout remains unchanged: `magic=0x4B4D4150`, `version=1`, 160 maximum entries, `KeyEntry` stores two zero-based matrix indices and a 20-byte label.
- **Never** call `:init CONFIRM` or `:erase CONFIRM` during migration unless you deliberately want to destroy the existing key mapping.
- Make a CSV backup with `:export` before flashing.
- Matrix indices in the export are one-based, not the internal zero-based values.
- This refactor removes per-key `DOWN`/`UP` diagnostic logs; the multi-key scanner and ghosting suppression are still used to feed USB and PS/2.
- Firmware 1.0.0 (Release Candidate): modularized build has been successfully tested on the actual hardware with USB-HID and ETC PS/2.

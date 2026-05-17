# Features in current state (only STM32F103)
- Hardcoded Dual Slots A/B
- Updates over UART (python tool)
- Compile time configurable logging
- Security (uECC P-256, sha246, aes128)
- Rollback protection: Bootcount check

## Flash Memory Layout
|            | Start Address | Size  | End Address |
|------------|---------------|-------|--------------|
| Bootloader | 0x08000000    | 24 KB | 0x08006000   |
| Slot A     | 0x08006000    | 48 KB | 0x08012000   |
| Slot B     | 0x08012000    | 48 KB | 0x0801E000   |
| Metadata   | 0x0801E000    | 8 KB  | 0x08020000   |

## Debug Build
| Memory Region | Used Size | Region Size | Usage % |
|----------------|-----------|--------------|---------|
| RAM            | 3104 B    | 20 KB        | 15.16%  |
| FLASH          | 19620 B   | 24 KB        | 79.83%  | 

## Using 3rd party repositories:
1. https://github.com/kokke/tiny-AES-c
2. https://github.com/kmackay/micro-ecc
3. https://github.com/B-Con/crypto-algorithms

# Vision: Hardware Agnostic Custom Bootloader for the STM32 family
- Features: 
  - Dual Slots A/B (configureable app size)
- Size: 24KB 
- Updates over:
  - OTA (esp32)
  - UART (python tool)
  - CAN bus
- Compile time configurable logging over UART, printf-stdarg.c or no logging. 
- Secure:
  - P-256 ECDSA key generation (uECC) over SHA-256 digests (B-con/crypto-algorithms) - signing & verification
  - AES-128 Encrypted (tiny-aes)
  - Hardware CRC32 
- Rollback Protection:
  - Bootcount check
  - Application version # check
- CI/CD pipeline (Jenkins)
  - Test scripts in Lua
  - Git push -> Build -> Check -> Flash over UART, CAN or OTA
- Custom designing PCB on KiCAD with STM32 and ESP32 MCUs

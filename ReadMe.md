# Features in current state
- Hardware agnostic architecture for STM32
- Dual Slots A/B (boots from different slot on update)
- Size: 24KB 
- Updates over UART (python tool)
- Compile time configurable logging over UART, printf-stdarg.c or no logging. 
- Supports: STM32 F1 MCU
- Secure:
  - P-256 ECDSA key generation (uECC) over SHA-256 digests (B-con/crypto-algorithms) - signing & verification
  - AES-128 Encrypted (tiny-aes)
  - Hardware CRC32 
  - Rollback Protection:
    - Bootcount check
    - Application version # check
  - Power loss resistance: 
    - Dual Metadata slots with updated bytes_written variable in case of power loss mid flash write
    - Erases and re-programs the same slot once power is back
  - IWDG runtime check for application

## Flash Memory Layout STM32F103RB
|            | Start Address | Size  | End Address |
|------------|---------------|-------|--------------|
| Bootloader | 0x08000000    | 24 KB | 0x08006000   |
| Slot A     | 0x08006000    | 48 KB | 0x08012000   |
| Slot B     | 0x08012000    | 48 KB | 0x0801E000   |
| Metadata 1 | 0x0801E000    | 4 KB  | 0x0801F000U  |
| Metadata 2 | 0x0801F000U   | 4 KB  | 0x08020000   |

## Debug Build
| Memory Region  | Used Size | Region Size | Usage % |
|----------------|-----------|--------------|---------|
| RAM            | 3144 B    | 20 KB        |  15.35%  |   
| FLASH          |  17556 B  | 24 KB        |  71.44%  |

## Using 3rd party repositories:
1. https://github.com/kokke/tiny-AES-c
2. https://github.com/kmackay/micro-ecc
3. https://github.com/B-Con/crypto-algorithms

# Planned features:
- Updates over OTA (esp32) as well
- Supports: STM32 F1, G4, H7, U5 MCUs
- CI/CD pipeline (Jenkins) or Github Actions
  - Test scripts in Lua
- Shell
- Settings KV store and Logging framework
- Diagnostics
- Modules will be removable/configurable: CRC32, AES128 Encryption, SHA256 Hashing, EDCSA, and EDCSA Curve Selection
- Zephyr Integration for Bootloader + App

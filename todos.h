/**
 * @file todos.h
 * @brief file to search for todos on issues, features and YTI (ideas yet to implement)
 * @date 
 */

Todo based on priority:
Readmefix, mermaid diagram
Remove stm32 specific/hw specific things from main.c and identify unit testable functions
Finish function check_interrupted_write and handle failure case for chain update_found->iROT_Flash_InstallSTMBoot, AFTER Disabling and enabling WRP in iROT
Consolidate all the relevant python scripts into one .sh script with arguments for BL_Update or FW_Update
Power loss: Make Python/BL script resume restart when possible
Test IWDG

Cryptographic ownership transfer by rotating root keys - Public Key Infrastructure, Add hardware-backed keys instead of private_key.pem
Constant-time comparison for signature/CRC verification. avoid early-exit byte comparisons that leak timing information
Secure erase of sensitive RAM after use: zero out AES key context, decrypted chunks, and IV material after each update completes
PVD brownout check: use the STM32's Programmable Voltage Detector to defer flash writes when voltage is already sagging
Secure Debug Auth: JTAG → Challenge (Hardware RNG) → Signed response → Unlock
Implement Memory Read Blocker Hooks: Add a Tcl script hook inside OpenOCD that checks the memory address before executing a read. If GDB asks to read an address below your application slot, OpenOCD can intercept the command and return a dummy error or zeroes.

Fuzzing framework/Github Actions/Testing:
Feature unit tests GTest, Integration testing, Security testing
Acceptance
Bootloader rejects unsigned firmware
Rollback blocked by counter
Runtime detects single-bit corruption
Custom firmware possible with dev key
Brick recovery via button combo
Performance measurements - Boottime w DWT, verification time, update time/fw size, flash footprint, RAM footprint, flash wear resistance/levelling
Store this meas somewhere
Static tests: cpp-check, clang-tidy
Consistent formatting?

Documentation/Report related:
Threat model + security audit
Documentation update/Literature standards 4 report: NIST recommendations, IEC 62443, ETSI IoT security recommendations, RFC 9019
Compare against other bootloaders like WolfBoot

Extensions
USB FWU path: dfu-util host and implement dfu protocol 
Zephyr for iROT? How to intergrate?
Make modules build time configurable: CRC, AES, SHA256/EDCSA (Curve Selection), iROT, in Zephyr + no Zephyr
Improve Zephyr App to use more APIs from stmboot
Zephyr Sysbuild
Shell with Settings/Meta KV store (also for zephyr)
Diagnostics at boot (ram integrity check, peripheral detection, flash wear?) 
Diagnostics recorded into protected memory after failure
Rollback by Delta History

#define BOARD_NAME           "NUCLEO-F103RB"
#define BOARD_FLASH_SIZE_KB  128
#define BOARD_RAM_SIZE_KB    20

#define FLASH_PAGE_SIZE_BL       1024U
#define SLOTA_START_ADDRESS      0x08006000U
#define SLOT_NUM_PAGES           48U
#define SLOTB_START_ADDRESS      0x08012000U
// #define METADATA_ADDRESS 0x0801E000U 
#define METADATA_ADDRESS_SLOT1         0x0801E000U // 4 kbytes each slot meta
#define METADATA_ADDRESS_SLOT2         0x0801F000U
#define FLASH_EOF                0x08020000

#define UART_RECEP_CHUNK_SIZE 1024U // bytes recieved per iteration matching one flash page
#define UART_CHUNK_TIMEOUT_MS 10000U // 115200 baud (11.5 KB/s), 1000 bytes takes 90ms, 1 chunk = 1024, around 93ms
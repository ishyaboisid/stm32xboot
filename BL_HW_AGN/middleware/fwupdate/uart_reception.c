/**
 * @file uart_reception.c
 * @note crc source: https://github.com/Steppeschool/stm32-custom-bootloader/tree/main
 */

#include "uart_reception.h"
#include "platform.h"
#include "crypto/verify.h"
#include "metadata.h"
#include "crypto/aes_ctr.h"
#include "aes/inc/aes.h"
#include "board_config.h"

extern volatile uint16_t uart_size;
extern CRC_HandleTypeDef hcrc;

uint8_t received_crc[4];

#define RECV_TIMEOUT_MS  5000U
#define BYTE_TIMEOUT_MS  2000U

static struct AES_ctx ctx; // context

static void send_ack(void) { PAL_UART_Transmit((uint8_t *)"ACK\r\n", 5, 100); }
static void send_nack(void) { PAL_UART_Transmit((uint8_t *)"NACK\r\n", 6, 100); }
static void send_resume_offset(uint32_t bytes_written) { 
    PAL_UART_Transmit((uint8_t *)bytes_written, sizeof(bytes_written), 100);
}

void Slot_UpdateProgress(Metadata *m, uint32_t bytes_written) {
    m->bytes_written = bytes_written;
    Metadata_Save(m);
}

RECEP_STATUS UART_Receive(uint8_t* received_header, Metadata *meta) {
    uint8_t size_buf[4];

    if(received_header[0] != RECEP_START_0) { send_nack(); return RECEP_ERR_START; }
    if(received_header[1] != RECEP_START_1) { send_nack(); return RECEP_ERR_START; }

    uint8_t incoming_fwv_major = received_header[2];
    uint8_t incoming_fwv_minor = received_header[3];
    // #ifndef DEBUG 
    if (incoming_fwv_major < meta->FW_VER_MAJOR || (incoming_fwv_major == meta->FW_VER_MAJOR && incoming_fwv_minor <= meta->FW_VER_MINOR)) { // will be disregarded when debugging
        send_nack();
        return RECEP_ERR_VERSION;
    }
    // #endif
    
    uint32_t write_addr;
    if (meta->SLOTA_LATEST) {
        if (PAL_Flash_EraseSlot(SLOTB_START_ADDRESS, SLOT_NUM_PAGES) != FLASH_OK) return RECEP_ERR_RECV;
        write_addr = SLOTB_START_ADDRESS;
        PAL_UART_Transmit((uint8_t *)"B\r\n", 3, 100); 
        // meta->SLOTA_LATEST = 0;
    } else {
        if (PAL_Flash_EraseSlot(SLOTA_START_ADDRESS, SLOT_NUM_PAGES) != FLASH_OK) return RECEP_ERR_RECV;
        write_addr = SLOTA_START_ADDRESS; 
        PAL_UART_Transmit((uint8_t *)"A\r\n", 3, 100);
        // meta->SLOTA_LATEST = 1;
    }
    send_ack(); // for 0xAA and 0xBB, fw version approv and after deciding slot to prog

    PAL_UARTEx_Receive_DMA(size_buf, sizeof(size_buf));
    uint32_t total_len = (uint32_t)size_buf[0] | (uint32_t)size_buf[1] << 8 | (uint32_t)size_buf[2] << 16 | (uint32_t)size_buf[3] << 24;
    if (total_len == 0 || total_len > (SLOT_NUM_PAGES * FLASH_PAGE_SIZE_BL)) { send_nack(); return RECEP_ERR_SIZE; }

    PAL_CRC_Reset(); // sets hcrc.Instance->CR = CRC_CR_RESET -> 0xFFFFFFFF
    static uint8_t chunk[UART_RECEP_CHUNK_SIZE]; // static so it doesn't live on stack
    uint32_t remaining_data = total_len; 
    meta->fw_size = total_len; 
    Metadata_Save(meta);
    send_ack(); // for size ok
    
    uint8_t iv[16];
    PAL_UARTEx_Receive_DMA(meta->iv, sizeof(iv));
    Metadata_Save(meta);
    send_ack(); // for IV recieved

    AES_init_ctx_iv(&ctx, AES_KEY, meta->iv); // converts 16 byte key into 11 round keys of 16 bytes each

    int chunk_count; 

    while (remaining_data > 0) {
        uint32_t chunk_len = remaining_data < UART_RECEP_CHUNK_SIZE ? remaining_data : UART_RECEP_CHUNK_SIZE;
        if (PAL_UARTEx_Receive_DMA(chunk, chunk_len) != PAL_OK) {
            send_nack();
            return RECEP_ERR_RECV;
        }

        AES_CTR_xcrypt_buffer(&ctx, chunk, uart_size); // decrypt before crc & flash write

        PAL_CRC_Accumulate(chunk, chunk_len);

        if (PAL_Flash_Write(write_addr, chunk, chunk_len) != FLASH_OK) return RECEP_ERR_RECV;
        write_addr += chunk_len;
        remaining_data -= chunk_len;
        chunk_count++;
        Slot_UpdateProgress(meta, chunk_count * UART_RECEP_CHUNK_SIZE);
        send_ack();
    }
    send_ack(); // for data as a whole

    uint32_t calculated_crc = PAL_CRC_GetResult();

    uint8_t crc_buf[4];
    if (PAL_UART_Receive(crc_buf, 4, BYTE_TIMEOUT_MS) != PAL_OK) {
            send_nack();
            return RECEP_ERR_RECV;
    }  
    uint32_t received_crc_value = (uint32_t)crc_buf[0] | (uint32_t)crc_buf[1] << 8 | (uint32_t)crc_buf[2] << 16 | (uint32_t)crc_buf[3] << 24;
    if(calculated_crc != received_crc_value) return RECEP_ERR_CRC32;
    send_ack();

    uint8_t signature[64];
    uint32_t which_slot_addr = meta->SLOTA_LATEST ? SLOTB_START_ADDRESS : SLOTA_START_ADDRESS;
    uint32_t sig_addr = write_addr - 64U; // signature is the last 64 bytes written to flash
    for (int i = 0; i < 64; i++) {
        signature[i] = *(volatile uint8_t *)(sig_addr + i); 
    }
    if (!Verify_Firmware((uint8_t *)which_slot_addr, total_len - 64U, signature)) {
        send_nack();
        PAL_Flash_EraseSlot(which_slot_addr, SLOT_NUM_PAGES);
        return RECEP_ERR_SIG; 
    }

    // meta->FW_VER_MAJOR = incoming_fwv_major; // done by app after confirmed healthy
    // meta->FW_VER_MINOR = incoming_fwv_minor;
    send_ack(); 

    return RECEP_OK;
}
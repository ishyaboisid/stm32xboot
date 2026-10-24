/**
 * @file uart_reception.c
 * @note crc source: https://github.com/Steppeschool/stm32-custom-bootloader/tree/main
 */

#include "uart_reception.h"
#include "metadata.h"
#include "img_security.h"

#ifdef UNIT_TEST
    #include "metadata_test_config.h"
    #include "pal_uart.h"
    #include "pal_crc.h"
    #include "pal_flash.h"
#else
    #include "platform.h"
#endif

#define BYTE_TIMEOUT_MS  2000U

extern volatile uint16_t uart_size;

uint8_t VER_MAJOR;
uint8_t VER_MINOR;

uint32_t write_addr;

static SEC_Ctx sec_ctx;

static void send_ack(void) { PAL_UART_Transmit((uint8_t *)"ACK\r\n", 5, 100); }
static void send_nack(void) { PAL_UART_Transmit((uint8_t *)"NACK\r\n", 6, 100); }

RECEP_STATUS UART_Receive(uint8_t* received_header, Metadata *meta, int ptype) {
    meta->fwu_state = FWU_STATE_START;
    Metadata_Save(meta);
    uint8_t size_buf[4];

    if (ptype) {
        if ((received_header[0] == BL_RECEP_START_0) && (received_header[1] == BL_RECEP_START_1)) {} else {
            send_nack();
            return RECEP_ERR_START;
        }
    } else {
        if ((received_header[0] == FW_RECEP_START_0) && (received_header[1] == FW_RECEP_START_1)) {} else {
            send_nack();
            return RECEP_ERR_START;
        }
    }   

    uint8_t incoming_fwv_major = received_header[2];
    uint8_t incoming_fwv_minor = received_header[3];

    if (ptype == 0) {
        VER_MAJOR = meta->FW_VER_MAJOR;
        VER_MINOR = meta->FW_VER_MINOR;
    } else {
        VER_MAJOR = meta->BL_VER_MAJOR;
        VER_MINOR = meta->BL_VER_MINOR;
    }
    if (incoming_fwv_major < VER_MAJOR || (incoming_fwv_major == VER_MAJOR && incoming_fwv_minor <= VER_MINOR)) { // will be disregarded when debugging
        send_nack();
        return RECEP_ERR_VERSION;
    }

    if (meta->SLOTA_LATEST) {
        meta->bl_state = BL_STATE_ERASING;
        Metadata_Save(meta);
        if (PAL_Flash_EraseSlot(SLOTB_START_ADDRESS, SLOT_NUM_PAGES) != FLASH_OK) return RECEP_ERR_RECV;
        write_addr = SLOTB_START_ADDRESS;
        PAL_UART_Transmit((uint8_t *)"B\r\n", 3, 100); 
    } else {
        meta->bl_state = BL_STATE_ERASING;
        Metadata_Save(meta);
        if (PAL_Flash_EraseSlot(SLOTA_START_ADDRESS, SLOT_NUM_PAGES) != FLASH_OK) return RECEP_ERR_RECV;
        write_addr = SLOTA_START_ADDRESS; 
        PAL_UART_Transmit((uint8_t *)"A\r\n", 3, 100);
    }
    meta->bl_state = BL_STATE_IDLE;
    meta->fwu_state = FWU_STATE_ERASEDSLOT;
    Metadata_Save(meta); 
    send_ack(); // for 0xAA and 0xBB, fw version approv and after deciding slot to prog

    if (PAL_UARTEx_Receive_DMA(size_buf, sizeof(size_buf)) != PAL_OK) {
        send_nack();
        return RECEP_ERR_RECV;
    }
    uint32_t total_len = (uint32_t)size_buf[0] | (uint32_t)size_buf[1] << 8 | (uint32_t)size_buf[2] << 16 | (uint32_t)size_buf[3] << 24;
    if (total_len == 0 || total_len > (SLOT_NUM_PAGES * FLASH_PAGE_SIZE_BL)) { send_nack(); return RECEP_ERR_SIZE; }

    static uint8_t chunk[UART_RECEP_CHUNK_SIZE]; // static so it doesn't live on stack
    uint32_t remaining_data = total_len; 
    meta->fw_size = total_len; 
    Metadata_Save(meta);
    send_ack(); // for size ok
    
    uint8_t iv[16];
    if (PAL_UARTEx_Receive_DMA(meta->iv, sizeof(iv)) != PAL_OK) {
        send_nack();
        return RECEP_ERR_RECV;
    }
    Metadata_Save(meta);
    if (security_begin(&sec_ctx, meta->fw_size, meta->iv) != SEC_OK) {
        send_nack();
        return RECEP_ERR_SEC;
    }
    
    meta->bl_state  = BL_STATE_WRITING;
    Metadata_Save(meta);

    send_ack(); // for IV recieved

    while (remaining_data > 0) {
        uint32_t chunk_len = remaining_data < UART_RECEP_CHUNK_SIZE ? remaining_data : UART_RECEP_CHUNK_SIZE;
        if (PAL_UARTEx_Receive_DMA(chunk, chunk_len) != PAL_OK) {
            send_nack();
            return RECEP_ERR_RECV;
        }

        if (security_processChunk(&sec_ctx, chunk, chunk_len, uart_size) != SEC_OK) {
            send_nack();
            return RECEP_ERR_SEC;
        }

        if (PAL_Flash_Write(write_addr, chunk, chunk_len) != FLASH_OK) return RECEP_ERR_RECV;
        write_addr += chunk_len;
        remaining_data -= chunk_len;
        send_ack();
    }

    meta->bl_state  = BL_STATE_IDLE;
    meta->fwu_state = FWU_STATE_WRITECOMPLETE;
    Metadata_Save(meta);

    uint8_t crc_buf[4];
    send_ack(); // for data as a whole
    if (PAL_UART_Receive(crc_buf, 4, BYTE_TIMEOUT_MS) != PAL_OK) {
            send_nack();
            return RECEP_ERR_RECV;
    }  
    uint32_t received_crc_value = (uint32_t)crc_buf[0] | (uint32_t)crc_buf[1] << 8 | (uint32_t)crc_buf[2] << 16 | (uint32_t)crc_buf[3] << 24;
    uint32_t which_slot_addr = meta->SLOTA_LATEST ? SLOTB_START_ADDRESS : SLOTA_START_ADDRESS;
    
    if (security_verify_crc(&sec_ctx, received_crc_value) != SEC_OK) {
        send_nack();
        return RECEP_ERR_SEC;
    }

    meta->fwu_state = FWU_STATE_CRCVERIFIED;
    Metadata_Save(meta);
    send_ack();
 
    if (security_verify_sig(&sec_ctx, which_slot_addr, ptype) != SEC_OK) {
        send_nack();
        if(PAL_Flash_EraseSlot(which_slot_addr, SLOT_NUM_PAGES)) return RECEP_ERR_RECV;
        return RECEP_ERR_SEC;
    }

    if (ptype) meta->bl_update_requested = 0;
    meta->fwu_state = FWU_STATE_ECCVERIFIED;
    Metadata_Save(meta);
    send_ack(); 

    meta->fwu_state = FWU_STATE_IDLE;
    meta->bl_state  = BL_STATE_IDLE;
    Metadata_Save(meta);

    return RECEP_OK;
}
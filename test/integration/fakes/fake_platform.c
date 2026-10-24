#include <string.h>
#include "fake_platform.h"
#include "board_config.h"

boot_uf test_uart;
uint8_t fake_slot_a[FAKE_FLASH_SLOT_SIZE];
uint8_t fake_slot_b[FAKE_FLASH_SLOT_SIZE];
SEC_Status fake_crc_status;
SEC_Status fake_signature_status;
SEC_Status fake_begin_status;
SEC_Status fake_process_status;
uint32_t fake_flash_write_calls;
uint32_t fake_flash_erase_slot_calls;
uint32_t fake_signature_calls;

static uint8_t *flash_ptr(uint32_t address, size_t len)
{
    if (address >= SLOTA_START_ADDRESS &&
        (uint64_t)address + len <= (uint64_t)SLOTA_START_ADDRESS + sizeof(fake_slot_a))
        return &fake_slot_a[address - SLOTA_START_ADDRESS];
    if (address >= SLOTB_START_ADDRESS &&
        (uint64_t)address + len <= (uint64_t)SLOTB_START_ADDRESS + sizeof(fake_slot_b))
        return &fake_slot_b[address - SLOTB_START_ADDRESS];
    return NULL;
}

void fake_platform_reset(void)
{
    memset(&test_uart, 0, sizeof(test_uart));
    memset(fake_slot_a, 0xFF, sizeof(fake_slot_a));
    memset(fake_slot_b, 0xFF, sizeof(fake_slot_b));
    fake_crc_status = SEC_OK;
    fake_signature_status = SEC_OK;
    fake_begin_status = SEC_OK;
    fake_process_status = SEC_OK;
    fake_flash_write_calls = 0U;
    fake_flash_erase_slot_calls = 0U;
    fake_signature_calls = 0U;
}

bool fake_uart_queue(const uint8_t *data, size_t len)
{
    if (data == NULL || len > sizeof(test_uart.rx_data) - test_uart.rx_length)
        return false;
    memcpy(test_uart.rx_data + test_uart.rx_length, data, len);
    test_uart.rx_length += len;
    return true;
}

PAL_UART_StatusTypeDef PAL_UART_Transmit(const uint8_t *data, uint16_t len, uint32_t timeout)
{
    (void)timeout;
    if (data == NULL || len > sizeof(test_uart.tx_data) - test_uart.tx_length)
        return PAL_ERROR;
    memcpy(test_uart.tx_data + test_uart.tx_length, data, len);
    test_uart.tx_length += len;
    return PAL_OK;
}

static PAL_UART_StatusTypeDef uart_read(uint8_t *dst, size_t len)
{
    if (dst == NULL || len > test_uart.rx_length - test_uart.rx_offset)
        return PAL_ERROR;
    memcpy(dst, test_uart.rx_data + test_uart.rx_offset, len);
    test_uart.rx_offset += len;
    return PAL_OK;
}

PAL_UART_StatusTypeDef PAL_UART_Receive(uint8_t *data, uint16_t len, uint32_t timeout)
{
    (void)timeout;
    return uart_read(data, len);
}

PAL_UART_StatusTypeDef PAL_UARTEx_Receive_DMA(uint8_t *data, uint16_t len)
{
    return uart_read(data, len);
}

PAL_UART_StatusTypeDef PAL_UART_DMAStop(void) { return PAL_OK; }

PAL_Flash_StatusTypeDef PAL_Flash_ErasePage(uint32_t address)
{
    if (address % FLASH_PAGE_SIZE_BL != 0U)
        return FLASH_ERR_INVALID_ADDR;
    uint8_t *p = flash_ptr(address, FLASH_PAGE_SIZE_BL);
    if (p == NULL) return FLASH_ERR_INVALID_ADDR;
    memset(p, 0xFF, FLASH_PAGE_SIZE_BL);
    return FLASH_OK;
}

PAL_Flash_StatusTypeDef PAL_Flash_EraseSlot(uint32_t start, uint32_t pages)
{
    fake_flash_erase_slot_calls++;
    if (pages == 0U || pages > SLOT_NUM_PAGES ||
        (start != SLOTA_START_ADDRESS && start != SLOTB_START_ADDRESS))
        return FLASH_ERR_INVALID_ADDR;
    memset(flash_ptr(start, (size_t)pages * FLASH_PAGE_SIZE_BL), 0xFF,
           (size_t)pages * FLASH_PAGE_SIZE_BL);
    return FLASH_OK;
}

PAL_Flash_StatusTypeDef PAL_Flash_Write(uint32_t address, const uint8_t *src, size_t len)
{
    fake_flash_write_calls++;
    if (src == NULL || (address & 1U) != 0U) return FLASH_ERR_INVALID_ADDR;
    uint8_t *dst = flash_ptr(address, len);
    if (dst == NULL) return FLASH_ERR_INVALID_ADDR;
    /* Model flash programming: bits may change from 1 to 0, never 0 to 1. */
    for (size_t i = 0; i < len; ++i)
        if ((uint8_t)(dst[i] & src[i]) != src[i]) return FLASH_ERR_WRITE;
    for (size_t i = 0; i < len; ++i) dst[i] &= src[i];
    return FLASH_OK;
}

/* Keep the serial-flow integration test focused on the transport and flash seam. */
PAL_Flash_StatusTypeDef Metadata_Save(Metadata *meta)
{
    return meta == NULL ? FLASH_ERR_INVALID_ADDR : FLASH_OK;
}

SEC_Status security_begin(SEC_Ctx *ctx, uint32_t size, const uint8_t iv[16])
{
    (void)iv;
    if (ctx == NULL) return SEC_ERR_CRC;
    ctx->total_size = size;
    ctx->bytes_processed = 0U;
    return fake_begin_status;
}

SEC_Status security_processChunk(SEC_Ctx *ctx, uint8_t *chunk, uint32_t len, uint16_t uart_size)
{
    (void)chunk;
    (void)uart_size;
    if (ctx != NULL) ctx->bytes_processed += len;
    return fake_process_status;
}

SEC_Status security_verify_crc(SEC_Ctx *ctx, uint32_t crc)
{
    (void)ctx; (void)crc;
    return fake_crc_status;
}

SEC_Status security_verify_sig(SEC_Ctx *ctx, uint32_t slot, int ptype)
{
    (void)ctx; (void)slot; (void)ptype;
    fake_signature_calls++;
    return fake_signature_status;
}

bool Reverify_Signature(Metadata *meta, uint32_t slot_addr)
{
    (void)meta; (void)slot_addr;
    return false;
}

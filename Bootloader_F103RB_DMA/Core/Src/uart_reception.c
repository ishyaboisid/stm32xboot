/**
 * @file uart_reception.c
 * @note crc source: https://github.com/Steppeschool/stm32-custom-bootloader/tree/main
 */

/* a/b logic:
Program into B -> Erase A -> Copy B into A
Essentially B is a staging area. Later we can boot from different slots for a true a/b system. 
both slots now contain same firmware.
*/

#include "uart_reception.h"
#include "flash.h"
#include "stm32f1xx_hal.h"

extern UART_HandleTypeDef huart2;
extern CRC_HandleTypeDef hcrc;

extern volatile uint8_t uart_rx_done;
extern volatile uint16_t uart_size;

uint8_t received_crc[4];

#define RECV_TIMEOUT_MS  5000U
#define BYTE_TIMEOUT_MS  2000U
#define CHUNK_TIMEOUT_MS 10000U // 115200 baud (11.5 KB/s), 1000 bytes takes 90ms, 1 chunk = 1024, around 93ms

static uint32_t crc_accumulate(uint8_t *data, size_t len) { // todo: Using HAL crc acc now?
    size_t word_count = len / 4;
    size_t remainder = len % 4;

    for (size_t i = 0; i < word_count; i++) { // feed 32bit words msb first into CRC->DR
        uint32_t word = (uint32_t)data[i*4] << 24 | (uint32_t)data[i*4 + 1] << 16 | (uint32_t)data[i*4 + 2] << 8 | (uint32_t)data[i*4 + 3]; // MSB to LSB
        hcrc.Instance->DR = word;
    } 
    if (remainder > 0) { // for e.g. data = [0xAF, 0xBC, 0xD9, 0xC5, 0x70, 0x3E]. Remainder = 2 (i = 0, 1), therefore 
        uint32_t last = 0xFFFFFFFFU;
        for (size_t i = 0; i < remainder; i++) { // todo: calculate by hand
            last = (last & ~(0xFFU << (24 - i*8))) | ((uint32_t)data[word_count*4 + i] << (24 - i*8));
        }
        hcrc.Instance->DR = last;
    }
    return hcrc.Instance->DR;
}

static void send_ack(void) { HAL_UART_Transmit(&huart2, (uint8_t *)"ACK\r\n", 5, 100); }
static void send_nack(void) { HAL_UART_Transmit(&huart2, (uint8_t *)"NACK\r\n", 6, 100); }

RECEP_STATUS UART_Receive(uint8_t* received_header) {
    HAL_UART_DMAStop(&huart2);
    huart2.RxState = HAL_UART_STATE_READY;

    if(received_header[0] != RECEP_START_0) { send_nack(); return RECEP_ERR_START; }
    if(received_header[1] != RECEP_START_1) { send_nack(); return RECEP_ERR_START; }
    uint32_t total_len = (uint32_t)received_header[2] | (uint32_t)received_header[3] << 8 | (uint32_t)received_header[4] << 16 | (uint32_t)received_header[5] << 24;
    if (total_len == 0 || total_len > (SLOT_NUM_PAGES * FLASH_PAGE_SIZE_BL)) { send_nack(); return RECEP_ERR_SIZE; }

    if (Flash_EraseSlot(SLOTB_START_ADDRESS, SLOT_NUM_PAGES) != FLASH_OK) return RECEP_ERR_RECV;

    __HAL_CRC_DR_RESET(&hcrc); // sets hcrc.Instance->CR = CRC_CR_RESET -> 0xFFFFFFFF
    static uint8_t chunk[RECEP_CHUNK_SIZE]; // static so it doesn't live on stack
    uint32_t remaining_data = total_len; uint32_t write_addr = SLOTB_START_ADDRESS;
    send_ack();
    while (remaining_data > 0) {
        uint32_t chunk_len = remaining_data < RECEP_CHUNK_SIZE ? remaining_data : RECEP_CHUNK_SIZE;
        uart_rx_done = 0; uart_size = 0;
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, chunk, chunk_len);
        uint32_t start = HAL_GetTick();

        while (!uart_rx_done || uart_size != chunk_len) {
            if ((HAL_GetTick() - start) > CHUNK_TIMEOUT_MS) {
                HAL_UART_DMAStop(&huart2);
                send_nack();
                return RECEP_ERR_RECV;
            }
        }
        
        HAL_UART_DMAStop(&huart2);
        huart2.RxState = HAL_UART_STATE_READY;

        crc_accumulate(chunk, chunk_len);

        if (Flash_Write(write_addr, chunk, chunk_len) != FLASH_OK) return RECEP_ERR_RECV;
        write_addr += chunk_len;
        remaining_data -= chunk_len;
        send_ack();
    }
    send_ack(); // for data as a whole

    uint32_t calculated_crc = hcrc.Instance->DR;

    uint8_t crc_buf[4];
    if (HAL_UART_Receive(&huart2, crc_buf, 4, BYTE_TIMEOUT_MS) != HAL_OK) {
            send_nack();
            return RECEP_ERR_RECV;
    }  
    uint32_t received_crc = (uint32_t)crc_buf[0] | (uint32_t)crc_buf[1] << 8 | (uint32_t)crc_buf[2] << 16 | (uint32_t)crc_buf[3] << 24;
    if(calculated_crc != received_crc) return RECEP_ERR_CRC32;
    send_ack();
    return RECEP_OK;
}
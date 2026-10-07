#include "blupdate.h"
#include "uart_reception.h"
#include "/Users/siddharthmanikant/Desktop/Bachelor_Project/iROT/inc/iROT_metadata.h"
#include "verify.h"
#include "platform.h"

uint8_t header_buf_bl[4]; // header buf contains 0xCC and 0xDD (for bootloader updates) and version 2b+2b=4b

extern uint32_t write_addr;

int check_for_bl_update(Metadata *meta) {
    if(!meta->bl_update_requested) return 0; // todo, decide the trigger for bl update

    uint32_t stage_addr = meta->SLOTA_LATEST? SLOTB_START_ADDRESS : SLOTA_START_ADDRESS;
    PAL_UART_Transmit((uint8_t *)"BL_READY\r\n", 10, 100);
    PAL_UARTEx_Receive_DMA(header_buf_bl, sizeof(header_buf_bl));
    if (UART_Receive(header_buf_bl, meta, 1) != RECEP_OK) {
        meta->bl_update_requested = 0;
        Metadata_Save(meta);
        return 1;
    }
    meta->bl_update_requested = 0;
    Metadata_Save(meta);

    IROT_Metadata irot_meta;
    IROT_Metadata_Load(&irot_meta);
    
    IROT_Metadata_SetPending(&irot_meta, stage_addr, meta->fw_size); // write_addr instead of stage_addr as write_addr takes into account the chunks programmed on top of stage_addr from uart_reception.c

    PAL_Delay(100);
    // PAL_NVIC_SystemReset(); // for testing commented out
}

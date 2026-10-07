// todo: for now, there is no wrp
// todo: refactor hal init and clock together as systeminit and move out of main

#include "iROT_metadata.h"
#include "iROT_flash.h" 
#include "iROT_clock.h"
#include "iROT_verify.h"
#include "stm32f1xx_hal.h"

#define SYST_CSR (*(volatile uint32_t *)0xE000E010U)

CRC_HandleTypeDef hcrc; // for testing

IROT_Metadata meta;
uint8_t AUTOMATIC_UPDATE = 0;

void check_interrupted_write(IROT_Metadata *m);
static void check_for_update(IROT_Metadata *m, uint8_t AUTOMATIC_UPDATE);
static void update_found(IROT_Metadata *m);
static void goto_stmboot(IROT_Metadata *meta);
static void enter_recovery(IROT_Metadata *m);

int main(void)
{
    init_clock();
    HAL_Init();
    IROT_Metadata_Load(&meta);
    check_interrupted_write(&meta);
    check_for_update(&meta, AUTOMATIC_UPDATE);
    if (meta.bl_update_verified) update_found(&meta);
    goto_stmboot(&meta);
    
    while (1) {}
}

void check_interrupted_write(IROT_Metadata *m) { // todo after enabling WRP 
    switch(m->wrp_state) {
        case IROT_WRP_IDLE:
            return;
        case IROT_WRP_DISABLING:
        case IROT_WRP_DISABLED:
        default:
            m->wrp_state = IROT_WRP_IDLE;
            m->bl_update_pending = 0;
            IROT_Metadata_Save(m);
            return;
    }
}

static void check_for_update(IROT_Metadata *meta, uint8_t AUTOMATIC_UPDATE) {
    if (AUTOMATIC_UPDATE) {

    } else if (!meta->bl_update_pending) return;
   if (meta->boot_state != 0xFFFFFFFFUL) { // blindly trust first boot
        uint32_t systick_ctrl = SYST_CSR;
        SYST_CSR = 0;
        
        if (!iROT_VerifySTMBoot(meta->bl_bin_addr, meta->bl_bin_size)) { // signature not valid 
            volatile int y = 1;
            SYST_CSR = systick_ctrl;
            return;
        }

        SYST_CSR = systick_ctrl;
   } 

    meta->bl_update_verified = 1;

    // disable_stmboot_write_protect(m);
}

static void update_found(IROT_Metadata *m) {
    uint32_t systick_ctrl = SYST_CSR;
    SYST_CSR = 0;
    if (iROT_Flash_InstallSTMBoot(m, (uint32_t)m->bl_bin_size)) { // todo failing here
        // todo error in update, infinite dfu mode (Recovery/Update Mode), this is where AUTOMATIC_UPDATE is used. 
        /*
        1. Reset Peripherals to Safe State
        2. Signal Error (e.g., Blink Error LED)
        3. Open Secure Interface (UART/USB)
        4. Loop & Wait for Valid Signed Image, anti rollback, maintain keys
        */
        SYST_CSR = systick_ctrl;
        m->bl_update_pending  = 0;
        m->bl_update_verified = 0;
        IROT_Metadata_Save(m);
        enter_recovery(m); // never return
    }
    SYST_CSR = systick_ctrl;
    // fall through to main.goto_stmboot
}

static void enter_recovery(IROT_Metadata *m) { // todo
    while (1) {
        // blink error pattern 
        HAL_Delay(500);
        // todo possibly add uart recieve to accept recovery binary - excluded for size reasons 
    }
}

static void goto_stmboot(IROT_Metadata *meta) {
    if ((*(volatile uint32_t *)STMBOOT_ADDRESS) == 0xFFFFFFFF) {
        return;
    } else {
        // if (meta->boot_state != 0xFFFFFFFFUL) { // if it is not first boot verify, maybe also check for if (meta->bl_bin_size == 0) return; ? 
        //     if (!iROT_VerifySTMBoot(STMBOOT_ADDRESS, meta->bl_bin_size)) {
        //         HAL_Delay(500);
        //         return; 
        //     }
        // }

        void (*app_reset_handler)(void) = (void *) ( *(volatile uint32_t *) (STMBOOT_ADDRESS + 4U));
        HAL_DeInit();        // de-init peripherals + mask SysTick IRQ
        SysTick->CTRL = 0;  // fully stop the counter itself
        ResetClocks();
        SCB->VTOR = STMBOOT_ADDRESS;
        __set_MSP(*(volatile uint32_t *)STMBOOT_ADDRESS);
        app_reset_handler();
    }
}
// screen /dev/tty.usbmodem103 115200 
#include "main.h"
Metadata meta;

extern uint32_t iwdg_reset;

CRC_HandleTypeDef hcrc;

/** @todo TEST IWDG AND COMMIT */
// IWDG_HandleTypeDef hiwdg;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

uint8_t header_buf[4]; // header buf contains 0xAA and 0xBB and version 2b+2b=4b

const uint8_t BL_Version[2] = { BL_VERSION_MAJOR, BL_VERSION_MINOR };
uint8_t AUTOMATIC_UPDATE = 0;
#if BL_LOG_LEVEL == BL_LOG_PRINTF
int uart_putchar(int c)
{
  PAL_UART_Transmit((uint8_t *)&c, 1, PAL_MAX_DELAY);
  return c;
}
#endif

int Rollback_Check(Metadata *m) {
  if (m->bootcount <= BOOT_COUNT_MAX || (m->runtime_fault_count <= RUNTIME_BOOT_COUNT_MAX)) return 0;
  m->SLOTA_LATEST = !meta.SLOTA_LATEST; // flip to previous slot 
  m->bootcount = 0;
  m->image_state = IMG_STATE_REVERTED;
  Metadata_Save(m); 
  return 1;
}

void update_image_state(Metadata *m) {
  if (meta.image_state == IMG_STATE_PENDING) {
    meta.image_state = IMG_STATE_TRIAL;
    // reset bootcount in Metadata_UpdateAfterRecieve
    Metadata_Save(&meta);
  } else if (meta.image_state == IMG_STATE_TRIAL) {
    Metadata_IncrementBootCount(&meta); // increment before jumping
    if (Rollback_Check(&meta)) {
      BL_LOG("Bootcount exceeded. Rolling back to previous version.");
    }
  } else if (meta.image_state == IMG_STATE_HEALTHY) {
    if (iwdg_reset) { // app failed to pet
      meta.runtime_fault_count++; // 10 resets allowed
      Metadata_Save(&meta);
      BL_LOG("IWDG reset detected\r\n");
    }
    if (Rollback_Check(&meta)) {
      BL_LOG("Runtime faults exceeded. Rolling back to previous version.");
    }
  } else if (meta.image_state == IMG_STATE_REVERTED) { /* @todo verify but cannot until IWDG is verified*/
    // old slot is now running — treat it as healthy, reset fault counters
    meta.image_state = IMG_STATE_HEALTHY;
    meta.bootcount = 0;
    meta.runtime_fault_count = 0;
    Metadata_Save(&meta);
    BL_LOG("Running reverted slot — marked healthy\r\n");
  }
}

void check_interrupted_write(Metadata *m) {
  uint32_t slot_addr = meta.SLOTA_LATEST ? SLOTB_START_ADDRESS : SLOTA_START_ADDRESS;

  if (meta.bl_state == BL_STATE_IDLE && meta.fwu_state == FWU_STATE_IDLE) { // normal boot
    BL_LOG("No interrupted writes!\r\n");
    AUTOMATIC_UPDATE = 0; 
    return;
  }

  if (meta.bl_state == BL_STATE_ERASING || meta.bl_state == BL_STATE_WRITING) { // power lost during erase or write
    meta.bl_state = BL_STATE_ERASING;
    Metadata_Save(&meta);
    PAL_Flash_EraseSlot(slot_addr, SLOT_NUM_PAGES);
    meta.bl_state = BL_STATE_IDLE;
    meta.fwu_state = FWU_STATE_IDLE;
    Metadata_Save(&meta);
    AUTOMATIC_UPDATE = 1;
    return;
  }

  // bl state idle but fwu state mid protocol
  switch(meta.fwu_state) {
    case FWU_STATE_START: // do nothing
      BL_LOG("FWU Interrupted @ state START - retrying...\r\n");
      meta.fwu_state = FWU_STATE_IDLE;
      AUTOMATIC_UPDATE = 1; 
      break;
    case FWU_STATE_ERASEDSLOT: // slot erase but no fw written, fall through and do same as FWU_STATE_WRITECOMPLETE
    case FWU_STATE_WRITECOMPLETE: // chunks written but no crc so integretiy is not confirmed
      BL_LOG("FWU Interrupted @ state ERASEDSLOT or WRITECOMPLETE - retrying...\r\n");
      PAL_Flash_EraseSlot(slot_addr, SLOT_NUM_PAGES);
      meta.fwu_state = FWU_STATE_IDLE;
      Metadata_Save(&meta);
      AUTOMATIC_UPDATE = 1;
      break; 
    case FWU_STATE_CRCVERIFIED: // integrity good, check authenticity
      BL_LOG("FWU Interrupted @ state CRCVERIFIED - checking authenticity...\r\n");
      if (Reverify_Signature(m, slot_addr)) {
        meta.fwu_state = FWU_STATE_ECCVERIFIED;
        Metadata_Save(&meta);
        Metadata_UpdateAfterRecieve(&meta, meta.SLOTA_LATEST);
      } else {
        PAL_Flash_EraseSlot(slot_addr, SLOT_NUM_PAGES);
        meta.fwu_state = FWU_STATE_IDLE;
        Metadata_Save(&meta);
        AUTOMATIC_UPDATE = 1;
      }
      break;
    case FWU_STATE_ECCVERIFIED: // image trustworthy, just commit
      BL_LOG("FWU Interrupted @ state ECCVERIFIED - committing to slot...\r\n");
      Metadata_UpdateAfterRecieve(&meta, meta.SLOTA_LATEST); // todo possibly rename the function
      break;
    default: 
      BL_LOG("FWU Interrupted - retrying...\r\n");
      PAL_Flash_EraseSlot(slot_addr, SLOT_NUM_PAGES);
      meta.fwu_state = FWU_STATE_IDLE;
      Metadata_Save(&meta);
      AUTOMATIC_UPDATE = 1;
      break;
  }
}

int main(void)
{
  System_Init();
  
  Metadata_Load(&meta);

  Task_BL_SetLED();

  BL_LOG("Starting bootloader (0.1)\r\n");

  update_image_state(&meta);

  check_interrupted_write(&meta);

  check_for_update(&meta, AUTOMATIC_UPDATE);
  
  goto_application(&meta);

  while (1) {}
}

void Task_BL_SetLED(void) {
  PAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
}

void Task_BL_BlinkLED(void) {
  PAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
  PAL_Delay(1000); 
  PAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
  PAL_Delay(1000); 
}

static void goto_application(Metadata *meta) {
  uint32_t boot_addr = meta->SLOTA_LATEST ? SLOTA_START_ADDRESS : SLOTB_START_ADDRESS; // true = slot a contains latest firmware // false = slot b contains latest firmware
  if ((*(volatile uint32_t *)boot_addr) == 0xFFFFFFFF) {
    BL_LOG("No app found, staying in bootloader...\r\n");
    return;
  } else {
    BL_LOG("Jumping to application...\r\n");
    PAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    void (*app_reset_handler)(void) = (void *) ( *(volatile uint32_t *) (boot_addr + 4U));
    /** @todo implement PAL functions needed before jumping to app */
    /*
    HAL_DeInit();        // de-init peripherals + mask SysTick IRQ
    SysTick->CTRL = 0;  // fully stop the counter itself
    __disable_irq(); // EXTI interrupt enabled (blue button, EXTI15_10_IRQn). If fired during/after jump, before app sets up SCB->VTOR, will call the bootloader's ISR handler — which no longer has a valid stack context
    */
    SCB->VTOR = boot_addr;
    __set_MSP(*(volatile uint32_t *)boot_addr); // used by CPU for exception handlers, HardFaults, SysTick, UART interrupts, any ISR. psp = application thread code
    /** @todo TEST IWDG AND COMMIT */
    /*
    #ifdef DEBUG
    PAL_FREEZE_IWDG();  // don't let breakpoints trigger resets
    #endif
    PAL_MX_IWDG_Init();
    */
    app_reset_handler(); // call function pointer 
  } 
}

/* User button nucleo f103rb:
Pressed State: Low (HAL_GPIO_ReadPin returns GPIO_PIN_RESET or 0)
Released State: High (HAL_GPIO_ReadPin returns GPIO_PIN_SET or 1)
*/
static void check_for_update(Metadata *meta, uint8_t AUTOMATIC_UPDATE) {

  PAL_GPIO_PinState Update_Pin_State;
  uint32_t end_tick = PAL_GetTick() + 5000; // 5 seconds from now

  BL_LOG("Press the User Button B1 PC13 to trigger UART update...\r\n"); // todo make this conditional if AUTOMATIC_UPDATE = 1;
  do {
    Update_Pin_State = PAL_GPIO_ReadPin( B1_GPIO_Port, B1_Pin ); 
    uint32_t current_tick = PAL_GetTick(); 

    if ((Update_Pin_State == GPIO_PIN_RESET ) || (current_tick > end_tick)) { // either button not pressed OR current tick > end_tick after 5 second window
      break;
    }
  } while (1);
  if (Update_Pin_State == GPIO_PIN_RESET || AUTOMATIC_UPDATE /*|| DBG_FORCE_UPDATE*/) {
    if (AUTOMATIC_UPDATE) {
      BL_LOG("Interrupted update detected!\r\n");
      if (meta->fwu_state == FWU_STATE_CRCVERIFIED) BL_LOG("power lost after crcverified!\r\n");
    }
    BL_LOG("Starting Firmware Download!\r\n");
    PAL_UART_Transmit((uint8_t *)"READY\r\n", 7, 100);
    PAL_UARTEx_Receive_DMA(header_buf, sizeof(header_buf));
    if (UART_Receive(header_buf, meta) == RECEP_OK) {
      Metadata_UpdateAfterRecieve(meta, meta->SLOTA_LATEST);
      PAL_UART_Transmit((uint8_t *)"OK\r\n", 4, 100);
      BL_LOG("Firmware update done!\r\nRebooting...\r\n");
      PAL_Delay(100);
      PAL_NVIC_SystemReset();
    } else {
      BL_LOG("Firmware update error! Halt!\r\n"); while (1) { Task_BL_BlinkLED(); };
    }
    // will fall to goto_application() as button not pressed within 5 secs
  }
}
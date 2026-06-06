 #include "main.h"
Metadata meta;

extern uint32_t iwdg_reset;

CRC_HandleTypeDef hcrc;

/** @todo TEST IWDG AND COMMIT */
// IWDG_HandleTypeDef hiwdg;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

uint8_t header_buf[4]; // header buf contains 0xAA and 0xBB no size (6b->2b) added version in header 2b+2b=4b
// bool SLOTA_LATEST = true; // true = slot a contains latest firmware // false = slot b contains latest firmware

const uint8_t BL_Version[2] = { BL_VERSION_MAJOR, BL_VERSION_MINOR };
uint8_t AUTOMATIC_UPDATE = 0;
#if BL_LOG_LEVEL == BL_LOG_PRINTF
int uart_putchar(int c)
{
  PAL_UART_Transmit((uint8_t *)&c, 1, HAL_MAX_DELAY);
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
  }

  if (meta.image_state == IMG_STATE_TRIAL) {
    Metadata_IncrementBootCount(&meta); // increment before jumping
    if (Rollback_Check(&meta)) {
      BL_LOG("Bootcount exceeded. Rolling back to previous version.");
    }
  }

  if (meta.image_state == IMG_STATE_HEALTHY) {
    if (iwdg_reset) { // app failed to pet
      meta.runtime_fault_count++; // 10 resets allowed
      Metadata_Save(&meta);
      BL_LOG("IWDG reset detected\r\n");
    }
    if (Rollback_Check(&meta)) {
      BL_LOG("Runtime faults exceeded. Rolling back to previous version.");
    }
  }
}

int main(void)
{
  System_Init();
  
  Metadata_Load(&meta);

  uint32_t power_loss_reset = (meta.bytes_written > 0) && (meta.bytes_written < meta.fw_size) && (meta.fw_size > 0);

  Task_BL_SetLED();

  BL_LOG("Starting bootloader (0.1)\r\n");

  update_image_state(&meta);

  if (power_loss_reset) {
      if (meta.SLOTA_LATEST) {
        PAL_Flash_EraseSlot(SLOTB_START_ADDRESS, SLOT_NUM_PAGES);
      } else {
        PAL_Flash_EraseSlot(SLOTA_START_ADDRESS, SLOT_NUM_PAGES); 
      }
      AUTOMATIC_UPDATE = 1;
          // clear progress so this doesn't trigger again next boot
    meta.bytes_written = 0;
    meta.fw_size       = 0;
    Metadata_Save(&meta);
    BL_LOG("Slot erased — waiting for retransmit\r\n");
  }

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

/** @todo TEST IWDG AND COMMIT */
static void goto_application(Metadata *meta) {
  if (meta->SLOTA_LATEST) {
    if ((*(volatile uint32_t *)SLOTA_START_ADDRESS) == 0xFFFFFFFF) {
      BL_LOG("No app found, staying in bootloader...\r\n");
      return;
    } else {
      BL_LOG("Jumping to application...\r\n");
      PAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
      void (*app_reset_handler)(void) = (void *) ( *(volatile uint32_t *) (SLOTA_START_ADDRESS + 4U));
      SCB->VTOR = SLOTA_START_ADDRESS;
      __set_MSP(*(volatile uint32_t *)SLOTA_START_ADDRESS); // used by CPU for exception handlers, HardFaults, SysTick, UART interrupts, any ISR. psp = application thread code
      // #ifdef DEBUG
      // PAL_FREEZE_IWDG();  // don't let breakpoints trigger resets
      // #endif
      // PAL_MX_IWDG_Init();
      app_reset_handler(); // call function pointer 
    }
  } else {
    if ((*(volatile uint32_t *)SLOTB_START_ADDRESS) == 0xFFFFFFFF) {
      BL_LOG("No app found, staying in bootloader...\r\n");
      return;
    } else {
      BL_LOG("Jumping to application...\r\n");
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
      void (*app_reset_handler)(void) = (void *) ( *(volatile uint32_t *) (SLOTB_START_ADDRESS + 4U));
      SCB->VTOR = SLOTB_START_ADDRESS;
      __set_MSP(*(volatile uint32_t *)SLOTB_START_ADDRESS); 
      // #ifdef DEBUG
      // PAL_FREEZE_IWDG();
      // #endif
      // PAL_MX_IWDG_Init(); 
      app_reset_handler();
    }  
  }
}

/* User button nucleo f103rb:
Pressed State: Low (HAL_GPIO_ReadPin returns GPIO_PIN_RESET or 0)
Released State: High (HAL_GPIO_ReadPin returns GPIO_PIN_SET or 1)
*/
static void check_for_update(Metadata *meta, uint8_t AUTOMATIC_UPDATE) {

  PAL_GPIO_PinState Update_Pin_State;
  uint32_t end_tick = PAL_GetTick() + 5000; // 5 seconds from now

  BL_LOG("Press the User Button B1 PC13 to trigger UART update...\r\n");
  do {
    Update_Pin_State = PAL_GPIO_ReadPin( B1_GPIO_Port, B1_Pin ); 
    uint32_t current_tick = PAL_GetTick(); 

    if ((Update_Pin_State == GPIO_PIN_RESET ) || (current_tick > end_tick)) { // either button not pressed OR current tick > end_tick after 5 second window
      break;
    }
  } while (1);
  if (Update_Pin_State == GPIO_PIN_RESET || AUTOMATIC_UPDATE /*|| DBG_FORCE_UPDATE*/) {
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
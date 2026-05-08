/* USER CODE BEGIN Header */
// todo: implement uart drivers by self to reduce flash space? 
/* todo: a/b logic:
uint8_t erase[4] = {0xFF, 0xFF, 0xFF, 0xFF};

switch (Flash_Verify(0x08006000, erase, 4))
{
    case FLASH_ERR_VERIFY:
        // Slot A contains data
        SLOT_START_ADDRESS = SLOTB_START_ADDRESS;
        break;

    case FLASH_OK:
        // Slot A still erased
        SLOT_START_ADDRESS = SLOTA_START_ADDRESS;
        break;
}
*/
#ifdef DEBUG
#warning "DEBUG BUILD"
#endif
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "debug.h"
#include "flash.h"
#include "uart_reception.h"
#ifdef DEBUG
#include "printf-stdarg.c"
#endif
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BL_VERSION_MAJOR 0
#define BL_VERSION_MINOR 1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */
const uint8_t BL_Version[2] = { BL_VERSION_MAJOR, BL_VERSION_MINOR };
// static const uint8_t test_pattern[] = {0xDE, 0xAD, 0xBE, 0xEF};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_CRC_Init(void);
/* USER CODE BEGIN PFP */
void Task_BL_SetLED(void);
void Task_BL_BlinkLED(void);
static void goto_application(void);
static void check_for_update(void);
#ifdef DEBUG
int uart_putchar(int c);
#endif
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile uint8_t uart_rx_done = 0;
volatile uint16_t uart_size = 0;
uint8_t header_buf[6];

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART2)
    {
        uart_size = Size;
        uart_rx_done = 1;
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */


  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */
  Task_BL_SetLED();
  DEBUG_PRINTF("Starting bootloader (%d.%d)...\r\n", BL_Version[0], BL_Version[1]); // ls /dev/tty.*
                                                                              // screen /dev/tty.usbmodem1103 115200
                                                                              // exit: Ctrl + A, then K
  check_for_update();
  goto_application();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

#ifdef DEBUG
int uart_putchar(int c)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&c, 1, HAL_MAX_DELAY);
  return c;
}
#endif

void Task_BL_SetLED(void) {
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
}

void Task_BL_BlinkLED(void) {
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
  HAL_Delay(1000); 
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
  HAL_Delay(1000); 
}

static void goto_application(void) {
  if ((*(volatile uint32_t *)SLOTA_START_ADDRESS) == 0xFFFFFFFF) {
    DEBUG_PRINTF("No app found, staying in bootloader...\r\n");
    return;
  } else {
DEBUG_PRINTF("Jumping to application...\r\n");
 HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
  void (*app_reset_handler)(void) = (void *) ( *(volatile uint32_t *) (SLOTA_START_ADDRESS + 4U)); // slot A's reset handler
  //   HAL_DeInit();        // de-init peripherals + mask SysTick IRQ
  // SysTick->CTRL = 0;  // fully stop the counter itself
  // __disable_irq(); // EXTI interrupt enabled (blue button, EXTI15_10_IRQn). If fired during/after jump, before app sets up SCB->VTOR, will call the bootloader's ISR handler — which no longer has a valid stack context

  __set_MSP(*(volatile uint32_t *)SLOTA_START_ADDRESS); //  used by CPU for exception handlers, HardFaults, SysTick, UART interrupts, any ISR. psp = application thread code
  app_reset_handler(); // call function pointer 
  }
  
}
/* User button nucleo f103rb:
Pressed State: Low (HAL_GPIO_ReadPin returns GPIO_PIN_RESET or 0)
Released State: High (HAL_GPIO_ReadPin returns GPIO_PIN_SET or 1)
*/
static void check_for_update(void) {

  GPIO_PinState Update_Pin_State;
  uint32_t end_tick = HAL_GetTick() + 5000; // 5 seconds from now

  DEBUG_PRINTF("Press the User Button B1 PC13 to trigger UART update...\r\n");
  do {
    Update_Pin_State = HAL_GPIO_ReadPin( B1_GPIO_Port, B1_Pin ); 
    uint32_t current_tick = HAL_GetTick(); 

    if ((Update_Pin_State == GPIO_PIN_RESET ) || (current_tick > end_tick)) { // either button not pressed OR current tick > end_tick after 5 second window
      break;
    }
  } while (1);
  if (Update_Pin_State == GPIO_PIN_RESET) {
    DEBUG_PRINTF("Starting Firmware Download!\r\n");
    // // if (Flash_ErasePage(SLOTA_START_ADDRESS) != FLASH_OK || Flash_Write(SLOTA_START_ADDRESS, test_pattern /*src*/, sizeof(test_pattern)/*len*/) != FLASH_OK || Flash_Verify(SLOTA_START_ADDRESS, test_pattern /*src*/, sizeof(test_pattern)/*len*/) != FLASH_OK) {
    // //     DEBUG_PRINTF("Firmware update error! Halt!\r\n"); while (1) { Task_BL_BlinkLED(); };
    // }
    HAL_UART_Transmit(&huart2, (uint8_t *)"READY\r\n", 7, 100);
    uart_rx_done = 0; uart_size = 0;
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, header_buf, sizeof(header_buf));
    __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
    while (!uart_rx_done || uart_size != sizeof(header_buf)) {}
    if (UART_Receive(header_buf) == RECEP_OK) {
      DEBUG_PRINTF("Firmware update done! Rebooting...\r\n");
      HAL_UART_Transmit(&huart2, (uint8_t *)"OK\r\n", 4, 100);
      HAL_Delay(100);
      HAL_NVIC_SystemReset();
    } else {
      DEBUG_PRINTF("Firmware update error! Halt!\r\n"); while (1) { Task_BL_BlinkLED(); };
    }
    // will fall to goto_application() as button not pressed within 5 secs
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

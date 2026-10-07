#include "iROT_clock.h"

/**
 * @brief Configure and initialise clock for SysCLK of 72 MHz
 * @note https://github.com/void-pointer-x/bluepill-clock-configuration/blob/main/src/main.c
 */
void init_clock(void)
{
	FLASH->ACR	|= FLASH_ACR_LATENCY_2; 	// Two wait states, per datasheet
	RCC->CFGR	|= RCC_CFGR_PPRE1_2;		// prescale AHB1 = HCLK/2
	RCC->CFGR 	|= RCC_CFGR_PLLXTPRE_HSE;	// PREDIV1 = 0
	RCC->CR 	|= RCC_CR_HSEON; 			// enable HSE clock
	while (!(RCC->CR & RCC_CR_HSERDY))
		; // wait for the HSEREADY flag

	RCC->CFGR 	|= RCC_CFGR_PLLSRC;			// set PLL source to HSE
	RCC->CFGR 	|= RCC_CFGR_PLLMULL9; 		// multiply by 9
	RCC->CR |= RCC_CR_PLLON;				// enable the PLL
	while (!(RCC->CR & RCC_CR_PLLRDY))
		; // wait for the PLLRDY flag

	RCC->CFGR 	|= RCC_CFGR_SW_PLL; 		// set clock source to pll

	while (!(RCC->CFGR & RCC_CFGR_SWS_PLL))
		; // wait for PLL to be CLK

	// SystemCoreClockUpdate(); 				// calculate the SYSCLOCK value
}

// /**
//   * @brief System Clock Configuration
//   * @retval None
//   */
// void SystemClock_Config(void)
// {
//   RCC_OscInitTypeDef RCC_OscInitStruct = {0};
//   RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

//   /** Initializes the RCC Oscillators according to the specified parameters
//   * in the RCC_OscInitTypeDef structure.
//   */
//   RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
//   RCC_OscInitStruct.HSIState = RCC_HSI_ON;
//   RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
//   // RCC_OscInitStruct.LSIState = RCC_LSI_ON;
//   RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//   RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
//   RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
//   if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//   {
//     Error_Handler();
//   }

//   /** Initializes the CPU, AHB and APB buses clocks
//   */
//   RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
//                               |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
//   RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
//   RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
//   RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
//   RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

//   if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
//   {
//     Error_Handler();
//   }
// }

void ResetClocks(void) // claude generated
{
    /* Switch SYSCLK back to HSI */
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_SYSCLKSOURCE_HSI);
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_SYSCLKSOURCE_STATUS_HSI)
    {
    }

    /* Disable PLL */
    CLEAR_BIT(RCC->CR, RCC_CR_PLLON);

    while (READ_BIT(RCC->CR, RCC_CR_PLLRDY) != 0)
    {
    }

    /* Disable HSE */
    CLEAR_BIT(RCC->CR, RCC_CR_HSEON);

    while (READ_BIT(RCC->CR, RCC_CR_HSERDY) != 0)
    {
    }

    /* Reset clock prescalers and PLL configuration */
    CLEAR_BIT(RCC->CFGR,
              RCC_CFGR_HPRE |
              RCC_CFGR_PPRE1 |
              RCC_CFGR_PPRE2 |
              RCC_CFGR_ADCPRE |
              RCC_CFGR_PLLSRC |
              RCC_CFGR_PLLXTPRE |
              RCC_CFGR_PLLMULL);

    /* Return SystemCoreClock to the HSI frequency */
    SystemCoreClock = HSI_VALUE;
}
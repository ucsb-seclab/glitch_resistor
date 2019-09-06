/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <string.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
unsigned int gr_tick = 0;
enum valueRtn {
  GR_SUCCESS = 3889321827,
  GR_FAILURE = 3552161478,
  GR_UNKNOWN = 879491493
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int checkValue(int value) {
  if (value == 0) {
    return GR_SUCCESS;
  } else {
    return GR_FAILURE;
  }
}

int checkTick() {
  if (gr_tick == 0) {
    return 0;
  } else {
    return -1;
  }
}

/**
 * Let's keep timing information
 * Reference:
https://stackoverflow.com/questions/13379220/generating-nanosecond-delay-in-c-on-stm32
*/
uint32_t m_nStart; // DEBUG Stopwatch start cycle counter value
uint32_t m_nStop;  // DEBUG Stopwatch stop cycle counter value

uint32_t flash_start = 0; // DEBUG Stopwatch start cycle counter value
uint32_t flash_end = 0;   // DEBUG Stopwatch stop cycle counter valu

#define DEMCR_TRCENA 0x01000000

/* Core Debug registers */
#define DEMCR (*((volatile uint32_t *)0xE000EDFC))
#define DWT_CTRL (*(volatile uint32_t *)0xe0001000)
#define CYCCNTENA (1 << 0)
#define DWT_CYCCNT ((volatile uint32_t *)0xE0001004)
#define CPU_CYCLES *DWT_CYCCNT

#define STOPWATCH_START                                                        \
  { m_nStart = *((volatile unsigned int *)0xE0001004); }
#define STOPWATCH_STOP                                                         \
  { m_nStop = *((volatile unsigned int *)0xE0001004); }

static inline void stopwatch_reset(void) {
  /* Enable DWT */
  DEMCR |= DEMCR_TRCENA;
  *DWT_CYCCNT = 0;
  /* Enable CPU cycle counter */
  DWT_CTRL |= CYCCNTENA;
}

static inline uint32_t stopwatch_getticks() { return CPU_CYCLES; }

static inline void stopwatch_delay(uint32_t ticks) {
  uint32_t end_ticks = ticks + stopwatch_getticks();
  while (1) {
    if (stopwatch_getticks() >= end_ticks)
      break;
  }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  int last_tick = -1;
  char buffer[100];
  // gr_tick = -1;
  u_int32_t loop_time = 0;

  sprintf(buffer, "Boot time: %d\n\r", stopwatch_getticks());
  HAL_UART_Transmit(&huart2, (uint8_t *)buffer, strlen(buffer), 0xFFFF);
  sprintf(buffer, "Flash time: %d\n\r", flash_end - flash_start);
  HAL_UART_Transmit(&huart2, (uint8_t *)buffer, strlen(buffer), 0xFFFF);

  while (last_tick != 0) {
    stopwatch_reset();

    /* USER CODE END WHILE */
    gr_tick = HAL_GetTick();
    int diff = (gr_tick - last_tick);

    if (checkValue(gr_tick) == GR_SUCCESS || checkTick() == 0) {
      while (1) {
        char buffer[] = "You win!";
        HAL_UART_Transmit(&huart2, (uint8_t *)buffer, strlen(buffer), 0xFFFF);
        HAL_Delay(100);
      }
    } else {
      sprintf(buffer, "no %d %d\n\r", diff, gr_tick);

      HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
      HAL_Delay(500);
    }
    HAL_UART_Transmit(&huart2, (uint8_t *)buffer, strlen(buffer), 0xFFFF);

    last_tick = gr_tick;
    /* USER CODE BEGIN 3 */

    loop_time = stopwatch_getticks();
    sprintf(buffer, "Loop time: %d\n\r", loop_time);
    HAL_UART_Transmit(&huart2, (uint8_t *)buffer, strlen(buffer), 0xFFFF);
  }
  while (1) {
    char buffer[] = "You win!";
    HAL_UART_Transmit(&huart2, (uint8_t *)buffer, strlen(buffer), 0xFFFF);
    HAL_Delay(100);
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 38400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

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
void assert_failed(char *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line)
   */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

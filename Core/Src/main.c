/* USER CODE BEGIN Header */
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
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "retarget.h"
#include "ringbuff.h"
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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t dma_buffer[256];
ringbuf_t ring_buff;
static uint16_t old_pos = 0;
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  uint16_t new_pos = Size;
  if(new_pos > old_pos)
  {
    ringbuf_write(&ring_buff, &dma_buffer[old_pos], new_pos - old_pos);
  }
  else
  {
    ringbuf_write(&ring_buff, &dma_buffer[old_pos], 256 - old_pos);
    ringbuf_write(&ring_buff, &dma_buffer[0], new_pos);
  }
}

#define MAX_DATA_LEN 64

typedef enum
{
  WAIT_HEADER1,
  WAIT_HEADER2,
  WAIT_LEN,
  WAIT_DATA,
  WAIT_CRC

}parser_state_t;

typedef struct
{
  parser_state_t state;

  uint8_t len;

  uint8_t data[MAX_DATA_LEN];

  uint8_t data_pos;

  uint8_t crc;

}parser_t;

void parser_init(parser_t *p)
{
  p->state = WAIT_HEADER1;

  p->len = 0;

  p->data_pos = 0;

  p->crc = 0;
}

uint8_t calc_crc(uint8_t len,
                 uint8_t *data)
{
  uint8_t crc = 0;

  crc += 0xAA;
  crc += 0x55;
  crc += len;

  for(uint8_t i = 0; i < len; i++)
  {
    crc += data[i];
  }

  return crc;
}

void packet_ok(parser_t *p)
{
  printf("recv packet: ");

  for(int i = 0; i < p->len; i++)
  {
    printf("%02X ", p->data[i]);
  }

  printf("\n");
}

void packet_error(void)
{
  printf("crc error\n");
}

void parser_input(parser_t *p,
                  uint8_t ch)
{
  switch(p->state)
  {
    case WAIT_HEADER1:

      if(ch == 0xAA)
      {
        p->state = WAIT_HEADER2;
      }

      break;

    case WAIT_HEADER2:

      if(ch == 0x55)
      {
        p->state = WAIT_LEN;
      }
      else
      {
        p->state = WAIT_HEADER1;
      }

      break;

    case WAIT_LEN:

      if(ch > MAX_DATA_LEN)
      {
        parser_init(p);
      }
      else
      {
        p->len = ch;

        p->data_pos = 0;

        p->state = WAIT_DATA;
      }

      break;

    case WAIT_DATA:

      p->data[p->data_pos++] = ch;

      if(p->data_pos >= p->len)
      {
        p->state = WAIT_CRC;
      }

      break;

    case WAIT_CRC:
    {
      uint8_t crc;

      crc = calc_crc(
              p->len,
              p->data);

      if(crc == ch)
      {
        packet_ok(p);
      }
      else
      {
        packet_error();
      }

      parser_init(p);

      break;
    }
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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  RetargetInit(&huart1);

  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_buffer, sizeof(dma_buffer));
  // __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
  char buff[100];
  char bytschar;
  parser_t parser;

  parser_init(&parser);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (ringbuf_read(&ring_buff, &bytschar, 1)) {
      // printf("%c", bytschar);
      parser_input(&parser, bytschar);
    }
    // scanf("%s", buff);
    // printf("Hello Worl .%s r\r\n", buff);
    // HAL_Delay(1000);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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

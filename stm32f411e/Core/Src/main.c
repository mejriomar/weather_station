/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "adc.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include <stdlib.h>  // Pour atoi()
#include <string.h>
#include "bmp280.h"
#include "i2c_lcd.h"

I2C_LCD_HandleTypeDef lcd1;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

void init_lcds(void) {
	lcd1.hi2c = &hi2c1;     // hi2c1 is your I2C handler
	lcd1.address = 0x27 << 1;    // I2C address for the first LCD
	lcd_init(&lcd1);        // Initialize the first LCD
}

uint32_t adc_read_channel(uint32_t chanel)
{
  /*get the adc hundel*/
  ADC_HandleTypeDef *hadc1 = get_adc_hundle();
  uint32_t adc_value;

  /*select channel manualy without scan mode*/
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel = chanel;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /*start the adc*/
  if (HAL_ADC_Start(hadc1) != HAL_OK)
  {
    Error_Handler();
  }
	if (HAL_ADC_PollForConversion(hadc1, HAL_MAX_DELAY) == HAL_OK)
	{
	  adc_value = HAL_ADC_GetValue(hadc1);  // Read value
	  // For debugging, you can output this via UART or LED logic
	}
  else
  {
    Error_Handler();
  }
  HAL_ADC_Stop(hadc1);
  return adc_value;
}
const char* is_gaz_f(uint8_t gaz)
{
    if (gaz == 0)
    {
        return "No gaz detected";
    }
    else
    {
        return "Gaz detected";
    }
}

const char* is_flame_f(uint8_t flame)
{
    if (flame == 0)
    {
        return "No flame detected";
    }
    else
    {
        return "Flame detected";
    }
}
uint8_t lcd_select = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	lcd_select++;
}

static void EXTILine0_Config(void)
{
  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOA clock */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Pin = GPIO_PIN_0;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

#define RX_BUF_SIZE 16

uint8_t  uart_byte;                       // pour réception 1 octet
uint8_t  uart_rx_buffer[RX_BUF_SIZE];     // buffer de reconstruction
uint8_t  rx_index = 0;
uint32_t co2 = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        // Stocke l'octet et avance l'index
        uart_rx_buffer[rx_index++] = uart_byte;

        // Si fin de trame (\n) ou buffer plein :
        if (uart_byte == '\n' || rx_index >= RX_BUF_SIZE-1)
        {
            uart_rx_buffer[rx_index] = '\0';   // termine la chaîne
            co2 = atoi((char*)uart_rx_buffer); // convertit

            // vidage pour prochaine réception
            rx_index = 0;
            memset(uart_rx_buffer, 0, RX_BUF_SIZE);

            // indication visuelle
            HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
        }

        // Relance toujours la réception d’1 octet
        HAL_UART_Receive_IT(&huart2, &uart_byte, 1);
    }
}
BMP280_HandleTypedef bmp280;

float pressure, temperature, humidity;

uint8_t is_gaz = 0;
uint8_t is_flame = 0;
uint8_t button = 0;

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
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  init_lcds();
  lcd_clear(&lcd1);

  uint32_t gaz_adc_value = 0;
  uint32_t flame_adc_value = 0;
	bmp280_init_default_params(&bmp280.params);
	bmp280.addr = BMP280_I2C_ADDRESS_0;
	bmp280.i2c = &hi2c1;

  	if (!bmp280_init(&bmp280, &bmp280.params))
  	{
  		Error_Handler();
		HAL_Delay(2000);
	}
	bool bme280p = bmp280.id == BME280_CHIP_ID;

	printf("BMP280: found %s\n", bme280p ? "BME280" : "BMP280");
	EXTILine0_Config();
	// Lancement de la 1ʳᵉ réception 1 octet
	HAL_UART_Receive_IT(&huart2, &uart_byte, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

  
  gaz_adc_value = adc_read_channel(ADC_CHANNEL_4);
  flame_adc_value = adc_read_channel(ADC_CHANNEL_1);

	while (!bmp280_read_float(&bmp280, &temperature, &pressure, &humidity))
	{
		printf("Temperature/pressure reading failed\n");
		HAL_Delay(2000);
	}
	if(gaz_adc_value > 1000)
	{

		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
		is_gaz = 1;
	}
	else
	{
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);
		is_gaz = 0;
	}

	if(flame_adc_value < 1000)
	{
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
		is_flame = 1;
	}
	else
	{
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);
		is_flame = 0;
	}

	switch (lcd_select)
	{
	    case 1:
			LCD_PrintfAt(&lcd1,0, 0,"pres %.2f hpa",pressure/100);
			LCD_PrintfAt(&lcd1,0, 1,"temp %.2f C",temperature);
	        break;
	    case 0:
			LCD_PrintfAt(&lcd1,0, 0,"hum %.2f\%",humidity);
			LCD_PrintfAt(&lcd1,0, 1,"CO2 %d ppm",co2);
	        break;
	    case 2:
			LCD_PrintfAt(&lcd1,0, 0,is_gaz_f(is_gaz));
			LCD_PrintfAt(&lcd1,0, 1,is_flame_f(is_flame));
	        break;

	    default:
	    	lcd_select = 0;
	}


   printf("{\"pressure\": %.2f, \"temperature\": %.2f, \"humidity\": %.2f, \"gaz\": %u, \"flame\": %u}\n",
	       pressure/100.0, temperature, humidity, is_gaz, is_flame);

	HAL_Delay(1000);
	lcd_clear(&lcd1);

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 50;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 8;
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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

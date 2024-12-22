/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include <string.h>
#include <stdio.h>
#include "i3g4250d_reg.h"
#include "stm32f4xx_hal.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define    BOOT_TIME            10
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static int16_t data_raw_angular_rate[3];
static float_t angular_rate_mdps[3];
static uint8_t whoamI;
static uint8_t tx_buffer[1000];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static int32_t write_to_i3g4250d(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len);
static int32_t read_from_i3g4250d(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len);
static void power_on_delay(uint32_t ms);
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
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  stmdev_ctx_t dev_ctx;

  /* Initialises gyro driver interface */
  dev_ctx.write_reg = write_to_i3g4250d;
  dev_ctx.read_reg = read_from_i3g4250d;
  dev_ctx.mdelay = power_on_delay;
  dev_ctx.handle = &hspi1;

  /* Wait sensor boot time this is called Power On Delay conceptually */
  platform_delay(BOOT_TIME);

  // Check device ID since i chose I3G4250D_ID
  i3g4250d_device_id_get(&dev_ctx, &whoamI);
  if (whoamI != I3G4250D_ID)
	  while (1); /*manage here device not found */

  /* Configure filtering chain -  Gyroscope - High Pass */
  i3g4250d_filter_path_set(&dev_ctx, I3G4250D_LPF1_HP_ON_OUT);
  i3g4250d_hp_bandwidth_set(&dev_ctx, I3G4250D_HP_LEVEL_3);

  /* Set Output Data Rate */
  i3g4250d_data_rate_set(&dev_ctx, I3G4250D_ODR_100Hz);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  uint8_t reg;

	  /* Read output only if new value is available */
	  i3g4250d_flag_data_ready_get(&dev_ctx, &reg);

	  if (reg) {
	  	  /* Read angular rate data */
	  	  memset(data_raw_angular_rate, 0x00, 3 * sizeof(int16_t));
	  	  /* Get the raw Angular velocity from the sensor - Coloriolis Forces*/
	  	  i3g4250d_angular_rate_raw_get(&dev_ctx, data_raw_angular_rate);

	  	  angular_rate_mdps[0] = i3g4250d_from_fs245dps_to_mdps(
	  							   data_raw_angular_rate[0]);
	  	  angular_rate_mdps[1] = i3g4250d_from_fs245dps_to_mdps(
	  							   data_raw_angular_rate[1]);
	  	  angular_rate_mdps[2] = i3g4250d_from_fs245dps_to_mdps(
	  							   data_raw_angular_rate[2]);
	  	  /*Send the Data via UART */
	  	  snprintf((char *)tx_buffer,
	  			  sizeof(tx_buffer),
	  			  "Angular Rate [mdps]:%4.2f\t%4.2f\t%4.2f\r\n",
	  			  angular_rate_mdps[0], angular_rate_mdps[1], angular_rate_mdps[2]);
	  	  HAL_UART_Transmit(&huart2, tx_buffer, len, 1000);
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
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
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

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GYRO_CS_OUT_GPIO_Port, GYRO_CS_OUT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : GYRO_CS_OUT_Pin */
  GPIO_InitStruct.Pin = GYRO_CS_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GYRO_CS_OUT_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 * @brief  Writes data to a specified register of the Gyro.
 *
 * Sends register address followed by the data to be written.
 * The SPI write operation is executed by setting RW bit to `0` as per the datasheet.
 * @param[in] handle SPI handle to use for communication.
 * @param[in] reg Register address to which data needs to be written.
 * @param[in] bufp Pointer to the data buffer to be written.
 * @param[in] len Length of the data buffer in bytes.
 *
 * @return int32_t 0 on success, error code otherwise.
 */
static int32_t write_to_i3g4250d(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
    // Set RW bit to 0 for write operation
    reg &= 0x7F;  // (Binary = 01111111) Clear the MSB for write command

    HAL_GPIO_WritePin(GYRO_CS_OUT_GPIO_Port, GYRO_CS_OUT_Pin, GPIO_PIN_RESET); // Enable sensor
    HAL_SPI_Transmit(handle, &reg, 1, 1000);  // Send the 8 bits register address
    HAL_SPI_Transmit(handle, (uint8_t*)bufp, len, 1000); // Send the 8 bit data
    HAL_GPIO_WritePin(GYRO_CS_OUT_GPIO_Port, GYRO_CS_OUT_Pin, GPIO_PIN_SET); // Deselect the sensor

    return 0;
}

/**
 * @brief  Reads data from a specified register of the Gyro device.
 *
 * This function communicates with the device over SPI, sending the register address and then receiving the data from the
 * device. The SPI read operation is executed by setting the RW bit to `1` as per the datasheet. The CS pin is used
 * to select the device, and SPI transactions are done with the SPI handle.
 *
 * @param[in] handle SPI handle to use for communication.
 * @param[in] reg Register address from which data needs to be read.
 * @param[out] bufp Pointer to the buffer where the read data will be stored.
 * @param[in] len Length of the data buffer in bytes to be read.
 *
 * @return int32_t 0 on success, error code otherwise.
 */
static int32_t read_from_i3g4250d(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    // Set RW bit to 1 for read operation
    reg |= 0x80;  // Setting MSB to indicate a read command

    HAL_GPIO_WritePin(GYRO_CS_OUT_GPIO_Port, GYRO_CS_OUT_Pin, GPIO_PIN_RESET); // Enables sensor
    HAL_SPI_Transmit(handle, &reg, 1, 1000);  // Send 8 bit register address
    HAL_SPI_Receive(handle, bufp, len, 1000);  // Receive 8 bit data
    HAL_GPIO_WritePin(GYRO_CS_OUT_GPIO_Port, GYRO_CS_OUT_Pin, GPIO_PIN_SET); // Deselect the sensor

    return 0;
}


/*
 * @brief  Helps implement a blocking delay to initialise our gyroscope peripheral.
 * This function implements Power On Delay concept basically.
 *
 * @param  ms        delay in ms
 *
 */
static void power_on_delay(uint32_t ms)
{
	HAL_Delay(ms);
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

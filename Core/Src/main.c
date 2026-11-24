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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "audio_signals.h"
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
I2C_HandleTypeDef hi2c1;

I2S_HandleTypeDef hi2s3;
DMA_HandleTypeDef hdma_spi3_tx;

TIM_HandleTypeDef htim7;

/* USER CODE BEGIN PV */
/* Endereço de 7 bits do CS43L22: 6 bits superiores fixos '100101' + pino AD0 (0 ou 1).
 * Na HAL do STM32, deslocamos 1 bit à esquerda (addr << 1).
 * Se AD0 = 0 -> 7-bit 0x4A -> HAL 0x94. Se AD0 = 1 -> 7-bit 0x4B -> HAL 0x96. */
#define CS43L22_I2C_ADDR_1  (0x4AU << 1)
#define CS43L22_I2C_ADDR_2  (0x4BU << 1)

#define AUDIO_RAM_MAX_LEN  AUDIO_100HZ_LEN

static uint16_t audio_ram_buf[AUDIO_RAM_MAX_LEN];
//static const uint16_t *active_flash_signal = audio_tone_100hz_flash;
//static uint16_t active_flash_len = AUDIO_100HZ_LEN;
//static const uint16_t *active_flash_signal = audio_tone_200hz_flash;
//static uint16_t active_flash_len = AUDIO_200HZ_LEN;
//static const uint16_t *active_flash_signal = audio_tone_400hz_flash;
//static uint16_t active_flash_len = AUDIO_400HZ_LEN;
//static const uint16_t *active_flash_signal = audio_tone_800hz_flash;
//static uint16_t active_flash_len = AUDIO_800HZ_LEN;
//static const uint16_t *active_flash_signal = audio_tone_1600hz_flash;
//static uint16_t active_flash_len = AUDIO_1600HZ_LEN;
static const uint16_t *active_flash_signal = audio_tone_6000hz_flash;
static uint16_t active_flash_len = AUDIO_6000HZ_LEN;
static uint8_t filter_enabled = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2S3_Init(void);
static void MX_TIM7_Init(void);
/* USER CODE BEGIN PFP */
static uint16_t CS43L22_DetectAndInit(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static HAL_StatusTypeDef CS43L22_WriteReg(uint16_t devAddr, uint8_t reg,
		uint8_t val) {
	return HAL_I2C_Mem_Write(&hi2c1, devAddr, reg, I2C_MEMADD_SIZE_8BIT, &val,
			1, 100);
}

static uint16_t CS43L22_DetectAndInit(void) {
	uint16_t addr = 0;

	/* 1. Mantém RESET baixo e libera para nível alto (amostra AD0 na subida) */
	HAL_GPIO_WritePin(Audio_RST_GPIO_Port, Audio_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(5);
	HAL_GPIO_WritePin(Audio_RST_GPIO_Port, Audio_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(5);

	/* 2. Testa resposta I2C nos dois endereços possíveis (AD0=0 ou AD0=1) */
	if (HAL_I2C_IsDeviceReady(&hi2c1, CS43L22_I2C_ADDR_1, 3, 100) == HAL_OK) {
		addr = CS43L22_I2C_ADDR_1;
	} else if (HAL_I2C_IsDeviceReady(&hi2c1, CS43L22_I2C_ADDR_2, 3, 100)
			== HAL_OK) {
		addr = CS43L22_I2C_ADDR_2;
	} else {
		return 0; /* Codec não respondeu no I2C1 */
	}

	/* 3. Sequência de inicialização do CS43L22 (Power Ctl 1 em 0x01 + ajustes obrigatórios) */
	CS43L22_WriteReg(addr, 0x02, 0x01); /* Power Ctl 1: mantém em power-down durante config - É DEFAULT*/

	/* Ajustes exigidos pela sequência de inicialização do datasheet */
	CS43L22_WriteReg(addr, 0x00, 0x99);
	CS43L22_WriteReg(addr, 0x47, 0x80);
	CS43L22_WriteReg(addr, 0x32, 0x80); /* bit 7 = 1 */
	CS43L22_WriteReg(addr, 0x32, 0x00); /* bit 7 = 0 */
	CS43L22_WriteReg(addr, 0x00, 0x00);

	/* Interface serial (I2S Philips 16-bit), clocking, saída de fone e volumes */
	CS43L22_WriteReg(addr, 0x06, 0x07); /* Interface Ctl 1: slave, I2S 16-bit */
	CS43L22_WriteReg(addr, 0x05, 0x80); /* Clocking Ctl: AUTO enabled */
	CS43L22_WriteReg(addr, 0x04, 0xAF); /* Power Ctl 2: habilita canais de saída */
	CS43L22_WriteReg(addr, 0x1A, 0x0A); /* PCM A volume */
	CS43L22_WriteReg(addr, 0x1B, 0x0A); /* PCM B volume */
//	CS43L22_WriteReg(addr, 0x20, 0x00); /* Master A volume */
//	CS43L22_WriteReg(addr, 0x21, 0x00); /* Master B volume */
	CS43L22_WriteReg(addr, 0x20, 0xD0); /* Master A volume */
	CS43L22_WriteReg(addr, 0x21, 0xD0); /* Master B volume */

	/* 4. Liga o codec (Power Ctl 1 = 0x9E após MCLK estar ativo) */
	CS43L22_WriteReg(addr, 0x02, 0x9E);
	HAL_Delay(5);

	return addr;
}

static void Audio_CopyToRam(const uint16_t *in_stereo, uint16_t *out_stereo,
		uint16_t len_halfwords) {
	for (uint16_t i = 0; i < len_halfwords; i++) {
		out_stereo[i] = in_stereo[i];
	}
}

static void Audio_LowPassFilter_400Hz(const uint16_t *in_stereo,
		uint16_t *out_stereo, uint16_t len_halfwords) {
	/* fs = 96000 Hz, fc = 400 Hz -> alpha = (2*pi*fc) / (fs + 2*pi*fc) */
	const float alpha = 0.0255106f;
	float y_prev_l = 0.0f;
	float y_prev_r = 0.0f;

	for (uint16_t i = 0; i + 1 < len_halfwords; i += 2) {
		float x_l = (float) ((int16_t) in_stereo[i]);
		float x_r = (float) ((int16_t) in_stereo[i + 1]);

		y_prev_l = y_prev_l + alpha * (x_l - y_prev_l);
		y_prev_r = y_prev_r + alpha * (x_r - y_prev_r);

		out_stereo[i] = (uint16_t) ((int16_t) y_prev_l);
		out_stereo[i + 1] = (uint16_t) ((int16_t) y_prev_r);
	}
}

static void Audio_ApplyOutputBuffer(uint8_t use_filter) {
	if (use_filter != 0U) {
		Audio_LowPassFilter_400Hz(active_flash_signal, audio_ram_buf,
				active_flash_len);
	} else {
		Audio_CopyToRam(active_flash_signal, audio_ram_buf, active_flash_len);
	}

	HAL_I2S_DMAStop(&hi2s3);
	HAL_I2S_Transmit_DMA(&hi2s3, audio_ram_buf, active_flash_len);
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
	MX_I2C1_Init();
	MX_I2S3_Init();
	MX_TIM7_Init();
	/* USER CODE BEGIN 2 */
	HAL_GPIO_WritePin(LD6_GPIO_Port, LD6_Pin, GPIO_PIN_SET);
	HAL_TIM_Base_Start_IT(&htim7);

	uint16_t codec_addr = CS43L22_DetectAndInit();
	if (codec_addr != 0) {
		/* LED3 (laranja) aceso = CS43L22 respondeu no I2C1 */
		HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);

		/* Tamanho em número de amostras/half-words (uint16_t), não em bytes */

//		HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*) audio_tone_100hz_flash, AUDIO_100HZ_LEN);
//		HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*) audio_tone_200hz_flash, AUDIO_200HZ_LEN);
//		HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*) audio_tone_400hz_flash, AUDIO_400HZ_LEN);
//		HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*) audio_tone_800hz_flash, AUDIO_800HZ_LEN);
//		HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*) audio_tone_1600hz_flash, AUDIO_1600HZ_LEN);
		HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*) audio_tone_6000hz_flash, AUDIO_6000HZ_LEN);
		filter_enabled = 0U;
		HAL_GPIO_WritePin(LD5_GPIO_Port, LD5_Pin, GPIO_PIN_RESET);
		Audio_ApplyOutputBuffer(filter_enabled);
	} else {
		/* LED4 (verde) aceso = falha de comunicação I2C com o CS43L22 */
		HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_SET);
	}
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

		static GPIO_PinState last_btn_state = GPIO_PIN_RESET;
		static uint32_t last_debounce_ms = 0U;

		GPIO_PinState current_btn_state = HAL_GPIO_ReadPin(B1_GPIO_Port,
		B1_Pin);

		if ((current_btn_state == GPIO_PIN_SET)
				&& (last_btn_state == GPIO_PIN_RESET)) {
			if ((HAL_GetTick() - last_debounce_ms) > 150U) {
				last_debounce_ms = HAL_GetTick();
				filter_enabled = (filter_enabled == 0U) ? 1U : 0U;

				HAL_GPIO_WritePin(LD5_GPIO_Port, LD5_Pin,
						(filter_enabled != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);

				Audio_ApplyOutputBuffer(filter_enabled);
			}
		}
		last_btn_state = current_btn_state;

	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

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
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 100000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief I2S3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2S3_Init(void) {

	/* USER CODE BEGIN I2S3_Init 0 */

	/* USER CODE END I2S3_Init 0 */

	/* USER CODE BEGIN I2S3_Init 1 */

	/* USER CODE END I2S3_Init 1 */
	hi2s3.Instance = SPI3;
	hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
	hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
	hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
	hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
	hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_96K;
	hi2s3.Init.CPOL = I2S_CPOL_LOW;
	hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
	hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
	if (HAL_I2S_Init(&hi2s3) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2S3_Init 2 */

	/* USER CODE END I2S3_Init 2 */

}

/**
 * @brief TIM7 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM7_Init(void) {

	/* USER CODE BEGIN TIM7_Init 0 */

	/* USER CODE END TIM7_Init 0 */

	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM7_Init 1 */

	/* USER CODE END TIM7_Init 1 */
	htim7.Instance = TIM7;
	htim7.Init.Prescaler = 8399;
	htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim7.Init.Period = 4999;
	htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim7) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM7_Init 2 */

	/* USER CODE END TIM7_Init 2 */

}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {

	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Stream5_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */

	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(CS_I2C_SPI_GPIO_Port, CS_I2C_SPI_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin,
			GPIO_PIN_SET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOD,
	LD4_Pin | LD3_Pin | LD5_Pin | LD6_Pin | Audio_RST_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : CS_I2C_SPI_Pin */
	GPIO_InitStruct.Pin = CS_I2C_SPI_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(CS_I2C_SPI_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : OTG_FS_PowerSwitchOn_Pin */
	GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = B1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : BOOT1_Pin */
	GPIO_InitStruct.Pin = BOOT1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : LD4_Pin LD3_Pin LD5_Pin LD6_Pin
	 Audio_RST_Pin */
	GPIO_InitStruct.Pin = LD4_Pin | LD3_Pin | LD5_Pin | LD6_Pin | Audio_RST_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
	GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : MEMS_INT2_Pin */
	GPIO_InitStruct.Pin = MEMS_INT2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(MEMS_INT2_GPIO_Port, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */

	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM7) {
		HAL_GPIO_TogglePin(LD6_GPIO_Port, LD6_Pin);
	}
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
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

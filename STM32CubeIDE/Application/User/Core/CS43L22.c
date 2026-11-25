/*
 * CS43L22.c
 *
 *  Created on: 25 de nov. de 2026
 *      Author: isabe
 */

#include "CS43L22.h"

/* Handle do barramento I2C1 definido em main.c */
extern I2C_HandleTypeDef hi2c1;


HAL_StatusTypeDef CS43L22_WriteReg(uint16_t devAddr, uint8_t reg, uint8_t val) {
	return HAL_I2C_Mem_Write(&hi2c1, devAddr, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
}

/**
 * @brief Pulsa o pino de reset do codec e inicializa os registradores via I2C1.
 * @retval Endereço HAL detectado (0x94 ou 0x96), ou 0 em caso de falha.
 */
uint16_t CS43L22_DetectAndInit(void) {
	uint16_t addr = 0U;

	CS43L22_Initialize();

	addr = CS43L22_DetectAddress();
	if (addr == 0U) {
		return 0U;
	}

	CS43L22_PowerDown(addr);
	CS43L22_InitialSetUp(addr);
	CS43L22_SetUpOutput(addr);
	CS43L22_PowerUp(addr);
	HAL_Delay(5U);

	return addr;
}

/**
 * Mantém RESET baixo e libera para nível alto (amostra AD0 na subida)
 */
void CS43L22_Initialize(void) {
	HAL_GPIO_WritePin(Audio_RST_GPIO_Port, Audio_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(5);
	HAL_GPIO_WritePin(Audio_RST_GPIO_Port, Audio_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(5);
}

/**
 * Testa resposta I2C nos dois endereços possíveis (AD0=0 ou AD0=1)
 */
uint16_t CS43L22_DetectAddress(void) {
	if (HAL_I2C_IsDeviceReady(&hi2c1, CS43L22_I2C_ADDR_1, 3, 100) == HAL_OK) {
		return CS43L22_I2C_ADDR_1;
	} else if (HAL_I2C_IsDeviceReady(&hi2c1, CS43L22_I2C_ADDR_2, 3, 100) == HAL_OK) {
		return CS43L22_I2C_ADDR_2;
	} else {
		return 0; /* Codec não respondeu no I2C1 */
	}
}

/**
 * Sequência de inicialização do CS43L22
 * Passo 1: desligar o codec
 * Power Ctl 1 em 0x01 + ajustes obrigatórios
 */
void CS43L22_PowerDown(uint16_t devAddr) {
	CS43L22_WriteReg(devAddr, 0x02, 0x01);
}

/**
 * Ajustes exigidos pela sequência de inicialização do datasheet
 */
void CS43L22_InitialSetUp(uint16_t devAddr) {
	CS43L22_WriteReg(devAddr, 0x00, 0x99);
	CS43L22_WriteReg(devAddr, 0x47, 0x80);
	CS43L22_WriteReg(devAddr, 0x32, 0x80); /* bit 7 = 1 */
	CS43L22_WriteReg(devAddr, 0x32, 0x00); /* bit 7 = 0 */
	CS43L22_WriteReg(devAddr, 0x00, 0x00);
}

/**
 * Interface serial (I2S Philips 16-bit), clocking, saída de fone e volumes
 */
void CS43L22_SetUpOutput(uint16_t devAddr) {
	CS43L22_WriteReg(devAddr, 0x06, 0x07); /* Interface Ctl 1: slave, I2S 16-bit */
	CS43L22_WriteReg(devAddr, 0x05, 0x80); /* Clocking Ctl: AUTO enabled */
	CS43L22_WriteReg(devAddr, 0x04, 0xAF); /* Power Ctl 2: habilita canais de saída */
	CS43L22_WriteReg(devAddr, 0x1A, 0x0A); /* PCM A volume */
	CS43L22_WriteReg(devAddr, 0x1B, 0x0A); /* PCM B volume */
//	CS43L22_WriteReg(devAddr, 0x20, 0x00); /* Master A volume */
//	CS43L22_WriteReg(devAddr, 0x21, 0x00); /* Master B volume */
	CS43L22_WriteReg(devAddr, 0x20, 0xD0); /* Master A volume */
	CS43L22_WriteReg(devAddr, 0x21, 0xD0); /* Master B volume */

}

/**
 * Liga o codec (só pode após MCLK estar ativo)
 */
void CS43L22_PowerUp(uint16_t devAddr) {
	CS43L22_WriteReg(devAddr, 0x02, 0x9E);
}

/*
 * CS43L22.h
 *
 *  Created on: 25 de nov. de 2026
 *      Author: isabe
 */

#ifndef APPLICATION_USER_CORE_CS43L22_H_
#define APPLICATION_USER_CORE_CS43L22_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* Endereço de 7 bits do CS43L22: 6 bits superiores fixos '100101' + pino AD0 (0 ou 1).
 * Na HAL do STM32, deslocamos 1 bit à esquerda (addr << 1).
 * Se AD0 = 0 -> 7-bit 0x4A -> HAL 0x94. Se AD0 = 1 -> 7-bit 0x4B -> HAL 0x96. */
#define CS43L22_I2C_ADDR_1  (0x4AU << 1)
#define CS43L22_I2C_ADDR_2  (0x4BU << 1)

/* Registradores principais do CS43L22 */
#define CS43L22_REG_POWER_CTL1      0x02U
#define CS43L22_REG_POWER_CTL2      0x04U
#define CS43L22_REG_CLOCKING_CTL    0x05U
#define CS43L22_REG_INTERFACE_CTL1  0x06U
#define CS43L22_REG_PCM_A_VOL       0x1AU
#define CS43L22_REG_PCM_B_VOL       0x1BU
#define CS43L22_REG_MASTER_A_VOL    0x20U
#define CS43L22_REG_MASTER_B_VOL    0x21U

HAL_StatusTypeDef CS43L22_WriteReg(uint16_t devAddr, uint8_t reg, uint8_t val);
uint16_t CS43L22_DetectAndInit(void);
void CS43L22_Initialize(void);
uint16_t CS43L22_DetectAddress(void);
void CS43L22_PowerDown(uint16_t devAddr);
void CS43L22_InitialSetUp(uint16_t devAddr);
void CS43L22_SetUpOutput(uint16_t devAddr);
void CS43L22_PowerUp(uint16_t devAddr);


#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_USER_CORE_CS43L22_H_ */

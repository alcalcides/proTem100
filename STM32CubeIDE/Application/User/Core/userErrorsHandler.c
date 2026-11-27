/*
 * audio_signals.c
 *
 *  Created on: 23 de nov. de 2026
 *      Author: isabe
 */

#include "userErrorsHandler.h"

int16_t ERROR_CODE = 0;

void userErrorHandler(int16_t code) {
	ERROR_CODE = code;
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
	Error_Handler();
}


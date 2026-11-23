/*
 * audio_signals.h
 *
 *  Created on: 23 de nov. de 2026
 *      Author: isabe
 */

#ifndef APPLICATION_USER_CORE_AUDIO_SIGNALS_H_
#define APPLICATION_USER_CORE_AUDIO_SIGNALS_H_



#include <stdint.h>

/* Quantidade de half-words (uint16_t) em estéreo (L/R) para fs = 96 kHz */
#define AUDIO_100HZ_LEN   1920U
#define AUDIO_200HZ_LEN   960U
#define AUDIO_400HZ_LEN   480U
#define AUDIO_800HZ_LEN   240U
#define AUDIO_1600HZ_LEN  120U
#define AUDIO_6000HZ_LEN  32U

/* Vetores armazenados na Flash (const) */
extern const uint16_t audio_tone_100hz_flash[AUDIO_100HZ_LEN];
extern const uint16_t audio_tone_200hz_flash[AUDIO_200HZ_LEN];
extern const uint16_t audio_tone_400hz_flash[AUDIO_400HZ_LEN];
extern const uint16_t audio_tone_800hz_flash[AUDIO_800HZ_LEN];
extern const uint16_t audio_tone_1600hz_flash[AUDIO_1600HZ_LEN];
extern const uint16_t audio_tone_6000hz_flash[AUDIO_6000HZ_LEN];

#endif /* APPLICATION_USER_CORE_AUDIO_SIGNALS_H_ */

/*
 * write_flash.h
 *
 *  Created on: 10 февр. 2020 г.
 *      Author: Александр
 */
#include "stm32f1xx_hal.h"

#ifndef WRITE_FLASH_H_
#define WRITE_FLASH_H_



int config_rcc(uint8_t *Data8);
void flash_stm32_read_byte(uint8_t *Data8, const uint32_t SizeBytes, const uint32_t AddrStart);
void proba_config(uint8_t *Data);
#endif /* WRITE_FLASH_H_ */

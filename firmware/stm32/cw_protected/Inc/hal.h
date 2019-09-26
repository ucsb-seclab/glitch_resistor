/*
    This file is part of the ChipWhisperer Example Targets
    Copyright (C) 2012-2015 NewAE Technology Inc.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HAL_H_
#define HAL_H_

void platform_init(void);
extern unsigned int delay_seed;

//PLATFORM Define Types
#define CW301_AVR      1
#define CW301_XMEGA    2
#define CW303          3
#define CW304          4
#define CW308_MEGARF   8
#define CW308_PIC24FJ  10
#define CW308_SAM4L    11
#define CW308_SI4010   12
#define CW308_MPC5748G 13
#define CW308_STM32F0  14
#define CW308_STM32F1  15
#define CW308_STM32F2  16
#define CW308_STM32F3  17
#define CW308_STM32F4  18
#define CW308_CC2538   19
#define CW308_K24F     20
#define CW308_NRF52840 21
#define CW308_AURIX    22
#define CW308_SAML11   23
#define CW308_EFM32TG11B 24
#define CW308_K82F     25
#define CW308_LPC55S6X 26

//HAL_TYPE Define Types
#define HAL_avr     1
#define HAL_xmega   2
#define HAL_pic24f  3
#define HAL_sam4l   4
#define HAL_stm32f0 5
#define HAL_stm32f1 6
#define HAL_stm32f2 7
#define HAL_stm32f3 8
#define HAL_stm32f4 9
#define HAL_cc2538  10
#define HAL_k24f    11
#define HAL_nrf52840 12
#define HAL_stm32f0_nano 13
#define HAL_aurix 14
#define HAL_saml11 15
#define HAL_efm32tg11b 16
#define HAL_k82f    17
#define HAL_lpc55s6x 18

#include "stm32f0_hal.h"

#ifndef led_error
#define led_error(a)
#endif

#ifndef led_ok
#define led_ok(a)
#endif

#endif //HAL_H_

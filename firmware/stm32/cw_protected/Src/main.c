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

#include "hal.h"
#include <stdio.h>

void uart_puts(char *s) {
  while (*s) {
    putch(*(s++));
  }
}

unsigned int gr_tick = 0;
enum valueRtn { GR_SUCCESS = 3889321827, GR_FAILURE = 3552161478, GR_UNKNOWN = 879491493 };

int main(void) {

  platform_init();
  init_uart();
  trigger_setup();

  putint(delay_seed);
  volatile uint32_t val = GR_FAILURE;

  volatile uint32_t val2 = 1;

  // Trigger
  *PIN_HIGH = TRIGGER_PIN;
  *PIN_LOW = TRIGGER_PIN;

  if (val == GR_SUCCESS) {
    uart_puts("Yes!");
  } else {
    uart_puts("No!");
  }

  while (val2 != 2) {
    ;
  }
  while (val2 != 2) {
    ;
  }

  return 1;
}

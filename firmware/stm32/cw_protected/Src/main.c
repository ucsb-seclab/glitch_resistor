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
int checkTick() {
  if (gr_tick == 0) {
    return 0;
  } else {
    return -1;
  }
}
int main(void) {

  platform_init();
  init_uart();
  trigger_setup();

  putint(delay_seed);
  volatile int blah = GR_FAILURE;
  volatile uint32_t one = 1;
  volatile uint32_t zero = 0;

  // Trigger
  *PIN_HIGH = TRIGGER_PIN;
  *PIN_LOW = TRIGGER_PIN;
#ifdef WHILE1
  while (one) {
    ;
  }
  asm("bl putint");
  putch('\n');
  uart_puts("Yes!");
#elif WHILENOT0
  while (!zero) {
    ;
  }
  asm("bl putint");
  putch('\n');
  uart_puts("Yes!");
#elif WHILEFIXED
  while (blah != GR_SUCCESS) {
    ;
  }
  asm("bl putint");
  putch('\n');
  uart_puts("Yes!");
#else
  if (blah == GR_SUCCESS) {
    asm("bl putint");
    putch('\n');
    uart_puts("Yes!");
  } else {
    uart_puts("No!");
  }
#endif
  while (one != 2) {
    ;
  }
  while (zero != 2) {
    ;
  }
  while (1) {
    ;
  }

  return 1;
}
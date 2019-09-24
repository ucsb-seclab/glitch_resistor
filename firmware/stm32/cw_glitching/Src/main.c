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

void glitch1(void) {
  // Some fake variable
  volatile uint8_t a = 1;

  // Trigger
  *PIN_HIGH = TRIGGER_PIN;
  *PIN_LOW = TRIGGER_PIN;

  // Infinite loop
  while (a != 0) {
    ;
  }
  asm("mov r0, r3");
  asm("bl putint");
  putch('\n');
  uart_puts("Yes!");
  putch('\n');

  // Several loops in order to try and prevent restarting
  while (a != 2) {
    ;
  }
  while (a != 2) {
    ;
  }
  while (1) {
    ;
  }
}

void glitch_fixed(void) {
  // Some fake variable
  volatile uint32_t a = 3889321827;

  // Trigger
  *PIN_HIGH = TRIGGER_PIN;
  *PIN_LOW = TRIGGER_PIN;

  // Infinite loop
  while (a != 3552161478) {
    ;
  }
  asm("mov r0, r2");
  asm("bl putint");
  putch('\n');
  uart_puts("Yes!");
  putch('\n');

  // Several loops in order to try and prevent restarting
  while (a != 2) {
    ;
  }
  while (a != 2) {
    ;
  }
  while (1) {
    ;
  }
}

void multi_glitch(void) {
  // Some fake variable
  volatile uint8_t a = 1;

  // Trigger
  *PIN_HIGH = TRIGGER_PIN;
  *PIN_LOW = TRIGGER_PIN;

  // Infinite loop
  while (a != 0) {
    ;
  }
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");

  asm("mov r0, r3");
  asm("bl putint");
  putch('\n');
  uart_puts("Yes1!");
  putch('\n');
  for (int i = 0; i < 100000; i++) {
    asm("nop");
  }

  *PIN_HIGH = TRIGGER_PIN;
  *PIN_LOW = TRIGGER_PIN;

  while (a != 0) {
    ;
  }

  asm("mov r0, r3");
  asm("bl putint");
  putch('\n');
  uart_puts("Yes!");
  putch('\n');
  putch(a);
  putch('\n');

  // Several loops in order to try and prevent restarting
  while (a != 2) {
    ;
  }
  while (a != 2) {
    ;
  }
  while (1) {
    ;
  }
}
void long_glitch(void) {
  // Some fake variable
  volatile uint8_t a = 0;
  volatile uint8_t b = 0;

  // Trigger
  *PIN_HIGH = TRIGGER_PIN;
  *PIN_LOW = TRIGGER_PIN;

  // Infinite loops
  while (a != 2) {
    ;
  }
  while (b != 2) {
    ;
  }
  uart_puts("Yes!");
  putch('\n');
  putch(a);
  putch(b);
  putch('\n');

  // Several loops in order to try and prevent restarting
  while (a != 2) {
    ;
  }
  while (a != 2) {
    ;
  }
  while (1) {
    ;
  }
}

int main(void) {

  platform_init();
  init_uart();
  trigger_setup();

  /* Uncomment this to get a HELLO message for debug */

  // This is needed on XMEGA examples, but not normally on ARM. ARM doesn't have
  // this macro normally anyway.
#ifdef __AVR__
  _delay_ms(20);
#endif

  while (1) {
#ifdef SINGLE
    uart_puts("single\n");
    glitch1();
#endif
#ifdef NOZERO
    uart_puts("nozero\n");
    glitch_fixed();
#endif
#ifdef DOUBLE
    uart_puts("multi\n");
    multi_glitch();
#endif
#ifdef LONG
    uart_puts("long\n");
    long_glitch();
#endif
  }

  return 1;
}

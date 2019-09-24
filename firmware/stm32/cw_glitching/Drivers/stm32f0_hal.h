#ifndef STM32F0_HAL_H
#define STM32F0_HAL_H


#define TRIGGER_PIN 0x1000

#define PIN_HIGH ((volatile unsigned int *) (0x48000018))
#define PIN_LOW ((volatile unsigned int *) (0x48000028))

void init_uart(void);
void putch(char c);
char getch(void);
void putint(unsigned int i);

void trigger_setup(void);
void trigger_low(void);
void trigger_high(void);

void led_error(unsigned int status);
void led_ok(unsigned int status);

#endif // STM32F0_HAL_H

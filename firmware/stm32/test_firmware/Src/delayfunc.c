#include "main.h"
#include "stm32_hal_legacy.h"
#include <stdlib.h>

// void gpdelay(void) {
//    // make sure that you initialize
//    // srand before this.
//    unsigned randCount = rand();
//    unsigned i = 0;
//    while(i < randCount) {
//        // some instructions to add delay.
//        rand();
//        i++;
//    }
//    return;
// }
uint32_t delay_seed = 123456789;
const unsigned int delay_m = 2147483648; // 2³¹
const unsigned delay_a = 1103515245;
const unsigned delay_c = 12345;
const unsigned int delay_max_len = 10;
const unsigned int delay_run_prct = delay_m * .5; // %
int delay_first = 1;
int delay_writing = 0;
const unsigned int seed_address = 0x0800e400;

extern flash_start;
extern flash_end;
/***
 * Reference:
https://community.st.com/s/question/0D50X00009XkfIOSAZ/stm32f0-help-with-flash-to-read-and-write-hal-libraries
*/
__attribute__((annotate("NoDelay"))) void seed_write() {

  delay_writing = 1;
  // Implement function to save delay_seed to non-volatile memory
  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();
  FLASH->CR |= FLASH_CR_PER;  /* (1) */
  FLASH->AR = 0x0800e400;     /* (2) */
  FLASH->CR |= FLASH_CR_STRT; /* (3) */

  while ((FLASH->SR & FLASH_SR_BSY) != 0) /* (4) */
  { /* For robust implementation, add here time-out management */
  }

  if ((FLASH->SR & FLASH_SR_EOP) != 0) /* (5) */
  {
    FLASH->SR |= FLASH_SR_EOP; /* (6)*/
  }

  else { /* Manage the error cases */
  }

  FLASH->CR &= ~FLASH_CR_PER; /* (7) */
  HAL_FLASH_Program(TYPEPROGRAM_WORD, 0x0800e400, delay_seed);
  /* Lock the Flash to disable the flash control register access (recommended
  to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();
  delay_writing = 0;
}

__attribute__((annotate("NoDelay"))) void seed_read() {
  // Implement function to read delay_seed from non-volatile memory
  delay_seed = *((volatile uint32_t *)0x0800e400);
}

__attribute__((annotate("NoResistor"))) void gpdelay() {
  if (delay_writing) {
    return;
  }
  if (delay_first) {
    seed_read();
  }
  // Update Seed
  delay_seed = (delay_a * delay_seed + delay_c) % delay_m;

  // Don't always execute loop
  if (delay_seed < delay_run_prct) {
    // Ensure that only loop delay_a maximum number of times (even delay_a few
    // instructions can throw off delay_a glitch)
    unsigned int loop_len = delay_seed % delay_max_len;
    for (int x = 0; x < loop_len; x++)
      asm("");
  }

  // Update our delay_seed
  if (delay_first) {
    flash_start = *((volatile uint32_t *)0xE0001004);
    seed_write();
    flash_end = *((volatile uint32_t *)0xE0001004);
    delay_first = 0;
  }
}

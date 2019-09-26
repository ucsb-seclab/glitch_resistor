#include <stdio.h>
__attribute__((annotate("NoResistor"))) void gr_glitch_detected() {
    while (1) {
        uart_puts("Glitch detected");
    }
}

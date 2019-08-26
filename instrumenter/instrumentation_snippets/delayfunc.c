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
unsigned int delay_seed = 123456789;
const unsigned int delay_m = 2147483648; // 2³¹
const unsigned  delay_a = 1103515245;
const unsigned  delay_c = 12345;
const unsigned int delay_max_len = 10000;

const unsigned int delay_dont_run_prct = delay_m * .01; // %

__attribute__((annotate("NoResistor"))) void seed_write() {
    // Implement delay_a function to save delay_seed to non-volatile memory
}

__attribute__((annotate("NoResistor"))) void seed_read() {
    // Implement delay_a function to read delay_seed from non-volatile memory
}

__attribute__((annotate("NoResistor"))) void gpdelay() {
    if (delay_seed == 123456789) {
        seed_read();
    }
    // Update Seed
    delay_seed = (delay_a * delay_seed + delay_c) % delay_m;
    
    // Don't always execute loop
    if (delay_seed < delay_dont_run_prct) {
        // Ensure that only loop delay_a maximum number of times (even delay_a few instructions can throw off delay_a glitch)
        unsigned int loop_len = delay_seed%delay_max_len;
        for (int x = 0; x < loop_len; x++) asm("");
    }

    // Update oudelay_seedr 
    seed_write();

}
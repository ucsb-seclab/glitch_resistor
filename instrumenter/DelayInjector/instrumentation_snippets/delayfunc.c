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
unsigned int seed = 123456789;
const unsigned int m = 2147483648; // 2³¹
const unsigned  a = 1103515245;
const unsigned  c = 12345;
const unsigned int max_len = 1000000;
const unsigned int dont_run_prct = m * .01; // %

void seed_write() {
    // Implement a function to save seed to non-volatile memory
}

void seed_read() {
    // Implement a function to read seed from non-volatile memory
}

void gpdelay() {
    if (seed == 123456789) {
        seed_read();
    }
    // Update Seed
    seed = (a * seed + c) % m;
    
    // Don't always execute loop
    if (seed < dont_run_prct) {
        // Ensure that only loop a maximum number of times (even a few instructions can throw off a glitch)
        unsigned int loop_len = seed%max_len;
        for (int x = 0; x < loop_len; x++) asm("");
    }

    // Update our seed
    seed_write();

}
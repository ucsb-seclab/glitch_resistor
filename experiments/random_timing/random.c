#include <stdio.h>

unsigned int seed = 123456789;
const unsigned int m = 2147483648; // 2³¹
const unsigned  a = 1103515245;
const unsigned  c = 12345;
const unsigned int max_len = 1000000;
const unsigned int dont_run_prct = m * .05; // %

void seed_write() {
    FILE *fptr;
    fptr = fopen("seed", "w+");
    if (fptr == NULL) {
        printf("Failed to create the file.\n");
        return;
    }
    putw(seed, fptr);
    fclose(fptr);
}

void seed_read() {
    FILE *fptr;
    fptr = fopen("seed", "r");
    if (fptr == NULL) {
        printf("Failed to open the file.\n");
        return;
    }
    seed = getw(fptr);
    fclose(fptr);
}

void gpdelay() {
    if (seed == 123456789) {
        // Repeatidly read seed to thwart glitching of the seed read
        for (int i = 0; i < 100; i++)
            seed_read();
    }
    // Update Seed
    seed = (a * seed + c) % m;
    
    // Don't always execute loop
    if (seed >= dont_run_prct) {
        // Ensure that only loop a maximum number of times (even a few instructions can throw off a glitch)
        unsigned int loop_len = seed%max_len;
        printf ("%d\t%d\t%d\n",seed,seed%max_len, loop_len);
        for (int x = 0; x < loop_len; x++) asm("");
    } else {
        printf("Skipped\n");
    }

    // Update our seed
    seed_write();

}

void rand() {
    register int foo asm ("rcx");
    
    // Read RBP
    // int b;
    // asm ("mov %%rsp, %0\n"
    //      : "=m"(b));
    
    // seed ^= b;
    seed = (a * seed + c) % m;

    unsigned int loop_len = seed%max_len;
    printf ("%d\t%d\t%d\n",seed,seed%max_len, loop_len);
    for (int x = 0; x < loop_len; x++) asm("");

    seed_write();

}

int main() {
    for (int x = 0; x < 100; x++) {
        gpdelay();
    }
}

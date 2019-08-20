#include<stdio.h>
int gpprotect_arr[100];
int main(void) {
    unsigned i;
    scanf("%u", &i);
    gpprotect_arr[1] = i;
    gpprotect_arr[i%100] = i*3;
    if(gpprotect_arr[1]) {
        printf("Not null\n");
    } else {
        printf("Null\n");
    }
    return 0;
}

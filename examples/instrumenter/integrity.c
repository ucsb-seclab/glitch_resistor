#include<stdio.h>
typedef struct foo {
    int a;
    int b;
    char *help;
} FOOS;
FOOS gpprotect_struc;
int main(void) {
    int i;
    scanf("%d", &i);
    gpprotect_struc.a = i;
    gpprotect_struc.b = i;
    if(gpprotect_struc.a) {
        printf("Not null\n");
    } else {
        printf("Null\n");
    }
    return 0;
}

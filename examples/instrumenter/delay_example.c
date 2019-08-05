#include<stdio.h>
int main() {
    int i;
    scanf("%d", &i);
    switch(i) {
       case 0: printf("Zero Value\n"); break;
       case 1: printf("One\n"); break;
       default: printf("Bad Value\n"); break;
    }
    return 0;
}

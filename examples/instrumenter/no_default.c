#include<stdio.h>
int main() {
   int c;
   scanf("%d", &c);
   switch(c) {
      case 1: printf("Hello\n"); break;
      case 2: printf("Baz\n"); break;
   }
   printf("End\n");
}

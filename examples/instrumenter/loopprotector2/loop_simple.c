#include<stdio.h>
int main(void) {
   int i;
   int j;
   int l;
   int tar = 2;
   scanf("%d", &i);
   scanf("%d", &j);
   
   for(l=0; l < i; l++) {
       if(l%3) {
           printf("Hello\n");
           break;
       }
       while(l < j) {
              tar = 6;
           printf("%d", tar);
           l++;
       }
       printf("%d\n", tar);
       tar = 7;
   }
   printf("%d",tar);
   printf("Loop exit\n");
   return 0;
}

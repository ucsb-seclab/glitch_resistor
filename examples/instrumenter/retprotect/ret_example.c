#include<stdio.h>
#include<stdlib.h>

__attribute__((noinline))
static int c() {
  if(rand() % 2 == 0)
    return 0;
  else 
    return 1;
}

__attribute__((noinline))
static int a() {
    printf("hello");
    return 0;
}
__attribute__((noinline))
static int b() {
    printf("goodbye");
    return 1;
}
int main() {
    int i;
    if(!a() && b())
      printf("The beetles say");
    if(c())
      printf("c");
}

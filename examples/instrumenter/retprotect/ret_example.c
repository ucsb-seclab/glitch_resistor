#include<stdio.h>

static int a() {
    printf("hello");
    return 0;
}
static int b() {
    printf("goodbye");
    return 1;
}
int main() {
    int i;
    if(!a() && b())
      printf("The beetles say");
}

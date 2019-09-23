#include<stdio.h>

static int a(int i) {
if (i > 0) {
    printf("hello");
    return 0;
} else {
	return 1;
}
}
static int b() {
    printf("goodbye");
    return 1;
}
int main() {
    volatile int i = 1;
    if(!a(i) && b())
      printf("The beetles say");
}

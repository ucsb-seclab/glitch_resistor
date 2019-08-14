#include<stdio.h>

int main() {
	int i;
	while (1) {
		scanf("%d", &i);
		if (i == 1) {
			printf("one\n");
		} else if (i == 2) {
			printf("two\n");
		} else {
			printf("dunno\n");
		}
	}
	return 0;
}

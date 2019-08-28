#include <stdio.h>
int main()
{
	int a = 0;
	// for (int x = 0; x < 10; x++)
	// {
	// 	a *= 2;
	// }
	// printf("loop 1: %d\n", a);

	while (1)
	{
		scanf("%d", &a);
		if (a == 1)
		{
			break;
		}
		else if (a == 2)
		{
			break;
		}
		else
		{
			printf("try again.\n");
		}
	}
	printf("loop 2: %d\n", a);

	// do
	// {
	// 	printf("hello\n");
	// 	a++;
	// } while (a <= 2);
	// printf("loop 3: %d\n", a);
}

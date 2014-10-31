#include <pthread.h>
#include <stdio.h>

void *print()
{
	printf("thread 1 print!\n");
}

void *print_2()
{
	printf("thread 2 print!\n");
}

main()
{
	pthread_t *p1,*p2;
	pthread_create(p1,NULL,print,NULL);
	pthread_create(p2,NULL,print_2,NULL);
}

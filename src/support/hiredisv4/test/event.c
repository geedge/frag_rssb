#include <pthread.h>
#include <stdio.h>

void thread_info()
{
	printf("pid = %u\ttid = %u\n",(unsigned int)getpid(),(unsigned int)pthread_self());
}

void cb_func()
{
	pthread_t tID;
    pthread_create(&tID,NULL,(void *)thread_info,NULL);
    printf("%u\n",tID);
    //pthread_join(tID,NULL);
}

int main()
{
	cb_func();
	cb_func();
	return 1;
}
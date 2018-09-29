#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <hiredis-vip/hircluster.h>

#define KEY_LEN 512

char ran_str1[250000][KEY_LEN];
char ran_str2[250000][KEY_LEN];
char ran_str3[250000][KEY_LEN];
char ran_str4[250000][KEY_LEN];

char* genRandom_str(int  len)
{
    int flag, i;
    char *string;
    srand((unsigned)time(NULL));
    if((string = (char *)malloc(len))==NULL)
    {
        printf("malloc error\n");
        return NULL;
    }
    for(i = 0;i < len-1; i++)
    {
        flag  = rand()%3;
        switch(flag)
        {
            case 0:
                string[i] = 'A' + rand()%26;
                break;
            case 1:
                string[i] = 'a' + rand()%26;
                break;
            case 2:
                string[i] = '0' + rand()%26;
                break;
            default:
                string[i] = '.';
                break;
        }
    }
    string[len-1] = '\0';
    return string;
}

void set_data1(redisClusterContext *cc)
{
	int i;
	struct timeval start,end;
	gettimeofday(&start,NULL);
	for(i = 0;i<250000;i+=1)
	{
		redisClusterAppendCommand(cc,"SET %d %s",i,ran_str1[i]);
	}
	redisCLusterReset(cc);
	gettimeofday(&end,NULL);
	printf("time: %f\n",(end.tv_sec+end.tv_usec/1000000.0) - (start.tv_sec+start.tv_usec/1000000.0));
}

void set_data2(redisClusterContext *cc)
{
	int i;
	struct timeval start,end;
	gettimeofday(&start,NULL);
	for(i = 0;i<250000;i+=1)
	{
		redisClusterAppendCommand(cc,"SET %d %s",i,ran_str1[i]);
	}
	redisCLusterReset(cc);
	gettimeofday(&end,NULL);
	printf("time: %f\n",(end.tv_sec+end.tv_usec/1000000.0) - (start.tv_sec+start.tv_usec/1000000.0));
}

void set_data3(redisClusterContext *cc)
{
	int i;
	struct timeval start,end;
	gettimeofday(&start,NULL);
	for(i = 0;i<250000;i+=1)
	{
		redisClusterAppendCommand(cc,"SET %d %s",i,ran_str1[i]);
	}
	redisCLusterReset(cc);
	gettimeofday(&end,NULL);
	printf("time: %f\n",(end.tv_sec+end.tv_usec/1000000.0) - (start.tv_sec+start.tv_usec/1000000.0));
}

void set_data4(redisClusterContext *cc)
{
	int i;
	struct timeval start,end;
	gettimeofday(&start,NULL);
	for(i = 0;i<250000;i+=1)
	{
		redisClusterAppendCommand(cc,"SET %d %s",i,ran_str1[i]);
	}
	redisCLusterReset(cc);
	gettimeofday(&end,NULL);
	printf("time: %f\n",(end.tv_sec+end.tv_usec/1000000.0) - (start.tv_sec+start.tv_usec/1000000.0));
}

int main()
{
	redisClusterContext *cc1,*cc2,*cc3,*cc4;
	const char *addr1 = "127.0.0.1:6379";
	const char *addr2 = "127.0.0.1:6379";
	const char *addr3 = "10.0.6.83:6379";
	const char *addr4 = "10.0.6.84:6379";

	cc1 = redisClusterConnectNonBlock(addr1,HIRCLUSTER_FLAG_NULL);
	cc2 = redisClusterConnectNonBlock(addr2,HIRCLUSTER_FLAG_NULL);
	cc3 = redisClusterConnectNonBlock(addr3,HIRCLUSTER_FLAG_NULL);
	cc4 = redisClusterConnectNonBlock(addr4,HIRCLUSTER_FLAG_NULL);
	if(cc1 == NULL||cc2 == NULL||cc3 == NULL||cc4 == NULL)
	{
		printf("connect error!\n");
		redisClusterFree(cc1);
		redisClusterFree(cc2);
		redisClusterFree(cc3);
		redisClusterFree(cc4);
		return -1;
	}
	int i;
	for(i=0;i<250000;i++)
	{
		strcpy(ran_str1[i],genRandom_str(KEY_LEN));
		strcpy(ran_str2[i],genRandom_str(KEY_LEN));
		strcpy(ran_str3[i],genRandom_str(KEY_LEN));
		strcpy(ran_str4[i],genRandom_str(KEY_LEN));
	}
	pthread_t thread_1,thread_2,thread_3,thread_4;
	pthread_create(&thread_1,NULL,(void *)set_data1,(void *)cc1);
	pthread_create(&thread_2,NULL,(void *)set_data2,(void *)cc2);
	pthread_create(&thread_3,NULL,(void *)set_data3,(void *)cc3);
	pthread_create(&thread_4,NULL,(void *)set_data4,(void *)cc4);
	pthread_join(thread_1,NULL);
	pthread_join(thread_2,NULL);
	pthread_join(thread_3,NULL);
	pthread_join(thread_4,NULL);

	redisClusterFree(cc1);
	redisClusterFree(cc2);
	redisClusterFree(cc3);
	redisClusterFree(cc4);

	return 0;
}
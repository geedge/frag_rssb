#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <hiredis-vip/hircluster.h>
#include <hiredis-vip/adapters/libevent.h>

#define KEY_LEN 512
#define VALUE_LEN 512
#define REQ_NUM 900000

char ran_key[REQ_NUM][KEY_LEN];
char ran_val[REQ_NUM][VALUE_LEN];

char* genRandom_str(int  len, int j)
{
    int flag, i;
    char *string;
    //srand((unsigned)time(0));
    srand(j);
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
                string[i] = '0' + rand()%10;
                break;
            default:
                string[i] = '.';
                break;
        }
    }
    string[len-1] = '\0';
    return string;
}

int all_count = 0;
int count = REQ_NUM;
/*
typedef struct calldata{
	redisClusterAsyncContext *acc;
	int count;
}calldata;
*/
void getCallback(redisClusterAsyncContext *acc, void *r, void *privdata){
	redisReply *reply = r;
	//int count = *(int*)privdata;
	struct event_base *base = (struct event_base *)privdata;
	all_count++;
	if(all_count >= count){
		//printf("all_count: %d\tcount: %d\n",all_count,count);
		//event_base_loopbreak(base);
		//redisClusterAsyncDisconnect(acc);
	}
}

void connectCallback(const redisAsyncContext *c, int status){
	if(status != REDIS_OK){
		printf("Error: %s\n",c->errstr);
		return;
	}
	//printf("all_count: %d\n",all_count);
	printf("Connected!\n");
}

void disconnectCallback(const redisAsyncContext *c, int status){
	if(status != REDIS_OK){
		printf("Error: %s\n",c->errstr);
		return;
	}
	//printf("all_count: %d\n",all_count);
	printf("Disconnected!\n");
}


int main(int argc, char **argv){
	int status;
	struct event_base *base = event_base_new();
	redisClusterAsyncContext *acc = redisClusterAsyncConnect("127.0.0.1:7001",HIRCLUSTER_FLAG_NULL);
	if(acc->err){
		printf("Error: %s\n",acc->errstr);
		return 1;
	}
	redisClusterLibeventAttach(acc,base);
	redisClusterAsyncSetConnectCallback(acc,connectCallback);
	redisClusterAsyncSetDisconnectCallback(acc,disconnectCallback);

	int i;
	for(i=0; i<REQ_NUM; i++)
    {
        strcpy(ran_key[i],genRandom_str(KEY_LEN,i));
        strcpy(ran_val[i],genRandom_str(VALUE_LEN,i));
    }
	struct timeval start,end;
	gettimeofday(&start,NULL);
	for(i=0;i<REQ_NUM;i++){
		//status = redisClusterAsyncCommand(acc,getCallback,&base,"set %s %s",ran_key[i],ran_val[i]);
		status = redisClusterAsyncCommand(acc,NULL,NULL,"set %s %s",ran_key[i],ran_val[i]);
		if(status != REDIS_OK){
			printf("error: %d %s\n",acc->err,acc->errstr);
		}
	}
	gettimeofday(&end,NULL);
	double t = (end.tv_sec+end.tv_usec/1000000.0) - (start.tv_sec+start.tv_usec/1000000.0);
	printf("total_time: %f\n",t);
	//redisClusterAsyncDisconnect(acc);
	event_base_dispatch(base);
	//event_base_loop(base,EVLOOP_NONBLOCK);
	gettimeofday(&end,NULL);
	t = (end.tv_sec+end.tv_usec/1000000.0) - (start.tv_sec+start.tv_usec/1000000.0);
	printf("total_time: %f\n",t);
	return 0;
}
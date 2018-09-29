#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <hiredis/hiredis.h>
//#include <hiredis-vip/hircluster.h>

#define KEY_LEN 512
#define VALUE_LEN 100
#define REQ_NUM 3000000

char *ran_key[REQ_NUM];
char *ran_val[REQ_NUM];

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
    //printf("%s\n",string);
    return string;
}



int main()
{
    redisContext *c = redisConnect("127.0.0.1", 6379);

    int i;
    struct timeval start,end;
    /*
    for(i=0; i<REQ_NUM; i++)
    {
        ran_val[i] = genRandom_str(VALUE_LEN,i);
    }
    
    gettimeofday(&start,NULL);
    for(i = 0; i < REQ_NUM; i++)
    {
        redisCommand(c, "zadd testset %d %s", i, ran_val[i]);
    }
    gettimeofday(&end,NULL);
    double t = (end.tv_sec+end.tv_usec/1000000.0) - (start.tv_sec+start.tv_usec/1000000.0);
    printf("zadd_time:%f\n",t);
    */

    gettimeofday(&start,NULL);
    for(i = 0; i < 10; i++)
    {
        redisCommand(c, "zrangebyscore testset -inf +inf");
    }
    gettimeofday(&end,NULL);
    double t = (end.tv_sec+end.tv_usec/1000000.0) - (start.tv_sec+start.tv_usec/1000000.0);
    printf("zrange_time:%f\n",t);
    return 0;
}

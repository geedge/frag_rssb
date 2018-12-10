#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis-vip/hircluster.h>

int main(int argc,char **argv){
	redisClusterContext *cc;
	const char *addr = (argc > 1)?argv[1]:"10.0.6.228:6001";
	//int port = (argc > 2)?atoi(argv[2]):6379;

	cc = redisClusterConnect(addr,HIRCLUSTER_FLAG_NULL);
	if(cc == NULL||cc->err){
		printf("connect error : %s\n", cc == NULL?"NULL":cc->errstr);
		redisClusterFree(cc);
		return -1;
	}

	redisReply *reply;
	/*Ping server
	reply = redisClusterCommand(cc,"PING");
    printf("OK\n");
	printf("PING: %s\n", reply->str);
    freeReplyObject(reply);*/

    //Set a key
    /*
    int i; 
    for(i = 1; i < 10000000; i++)
    {
        redisClusterAppendCommand(cc,"SET %s %s", "foo", "hello world");
        if(i % 5000 == 0)
        {
            redisClusterGetReply(cc,(void *)&reply);
            if(cc->err)
            {
                printf("cc: %s\n",cc->errstr);
            }
            else
            {
                redisCLusterReset(cc);
                freeReplyObject(reply);
            }
        }
    }*/

    //again
    reply = redisClusterCommand(cc,"get %s", "china_unicom_beijing_chaoyang_computerla_10.0.6.228_c_s");
    printf("get: %d\n", reply->integer);
    freeReplyObject(reply);
    /*
    reply = redisClusterCommand(cc,"GET 1");
    printf("GET foo: %s\n", reply->str);
    freeReplyObject(reply); 
    */
    
    /* Set a key using binary safe API 
    reply = redisClusterCommand(cc,"SET %b %b", "bar", (size_t) 3, "hello", (size_t) 5);
    printf("SET (binary API): %s\n", reply->str);
    freeReplyObject(reply);

    // Try a GET and two INCR 
    reply = redisClusterCommand(cc,"GET foo");
    printf("GET foo: %s\n", reply->str);
    freeReplyObject(reply);

    reply = redisClusterCommand(cc,"GET bar");
    printf("GET bar: %s\n",reply->str);
    freeReplyObject(reply);

    reply = redisClusterCommand(cc,"SET counter 1");
    printf("SET :%s\n",reply->str);
    freeReplyObject(reply);

    reply = redisClusterCommand(cc,"INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);
     again ... 
    reply = redisClusterCommand(cc,"INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);*/
    /*
    reply = redisClusterCommand(cc,"MSET %s %s %s %s","foo","test","foo1","test1");
    printf("MSET: %s\n",reply->str);
    freeReplyObject(reply);

    reply = redisClusterCommand(cc,"MGET foo foo1");
    printf("MGET foo foo1: %s\n",reply->str);
    freeReplyObject(reply);
	*/
    redisClusterFree(cc);
    return 0;

}

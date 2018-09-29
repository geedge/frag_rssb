/*
假设北京分中心的redis为主实例，上海和广州的为从实例
北京IP：10.172.*.*
上海IP：10.180.*.*
广州IP：10.188.*.*
*/

#define HIRCLUSTER_FLAG_NULL //主节点客户端标识
#define HIRCLUSTER_FLAG_ADD_SLAVE //从节点客户端标识

//示例
redisClusterContext *mc = redisClusterConnect("10.172.64.61:9001", HIRCLUSTER_FLAG_NULL);
redisClusterContext *sc = redisClusterConnect("10.180.64.61:9001", HIRCLUSTER_FLAG_ADD_SLAVE);

/*
以下四个宏用于标记从集群间的唯一IP段
*/
#define HIRCLUSTER_FLAG_UNIQIP_FRAG_ONE
#define HIRCLUSTER_FLAG_UNIQIP_FRAG_TWO
#define HIRCLUSTER_FLAG_UNIQIP_FRAG_THREE
#define HIRCLUSTER_FLAG_UNIQIP_FRAG_FOUR

/*
功能：	为集群操作句柄提供从实例信息,在与redis集群建立连接后调用
参数：  cc:			集群操作句柄
		flag：		从集群的唯一IP段标识，即上面定义的四个宏选一
		uni_ipfrag:	唯一IP段的具体值
返回值：0 成功
		-1 失败
*/
int redisClusterEnableSalve(redisClusterContext *cc, int flag, const char *uni_ipfrag)

//示例，上海分中心客户端
redisClusterEnableSalve(sc, HIRCLUSTER_FLAG_UNIQIP_FRAG_TWO, "180");
//广州客户端
redisClusterEnableSalve(sc, HIRCLUSTER_FLAG_UNIQIP_FRAG_TWO, "188");

//完整示例
#include <stdio.h>
#include <hiredis-vip/hircluster.h>

int main(int argc, char const *argv[])
{
	const char * host = "10.0.6.80:9001";
	redisClusterContext *cc = redisClusterConnect(host, HIRCLUSTER_FLAG_ADD_SLAVE);
	if(cc->err || cc == NULL)
    {
        printf("connect error\n");
        pthread_exit(NULL);
    }
    const char *uniqIP = "81,83";
    redisClusterEnableSalve(cc, HIRCLUSTER_FLAG_UNIQIP_FRAG_FOUR, uniqIP);
    redisReply *reply;
    int i;
    for(i = 0; i < 100; i++)
    //while(1)
    {
    	reply = redisClusterCommand(cc, "HMGET pet dog");
    	if(reply->type == REDIS_REPLY_ERROR)
    	{
    		printf("something wrong: %s\n", reply->str);
    	}
    	freeReplyObject(reply);
    }
    redisClusterFree(cc);
	return 0;
}
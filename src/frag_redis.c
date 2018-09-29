#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <net/if.h>
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>

#include "hiredis.h"
#include "hircluster.h"

#include "MESA_handle_logger.h"

#include "frag_redis.h"
#include "frag_reassembly_in.h"

extern frag_reassembly_t 			frag_rssb;     //use media hash

/*
功能：	为集群操作句柄提供从实例信息,在与redis集群建立连接后调用
参数：  cc:			集群操作句柄
		flag：		从集群的唯一IP段标识，即上面定义的四个宏选一
		uni_ipfrag:	唯一IP段的具体值
返回值：0 成功
		-1 失败
*/
extern "C" int redisClusterEnableSalve(redisClusterContext *cc, int flag, const char *uni_ipfrag);

/**
*
*return : 
*		-1:connect error; 
*		-2:reply error(need to freeReplyObject) ; 
*		0:succ(need to freeReplyObject) 
*/
int redis_nocluster_excute_command(int thread_seq, void* logger, redisReply** reply, char* cmmd, int argc, const char **argv, const size_t *argvlen)
{
	int rec = 0;
	redisReply* cmmd_reply = *reply;	

	if(NULL==frag_rssb.redis_ctx[thread_seq] || frag_rssb.redis_ctx[thread_seq]->err)
	{		
		if(NULL!=frag_rssb.redis_ctx[thread_seq])
		{
			redisFree(frag_rssb.redis_ctx[thread_seq]);
			frag_rssb.redis_ctx[thread_seq] = NULL;
		}
		frag_rssb.redis_ctx[thread_seq] = redisConnectWithTimeout(frag_rssb.redis_ip, frag_rssb.redis_port, frag_rssb.redis_tv);
	}
	if(NULL==frag_rssb.redis_ctx[thread_seq]) return -1;

	if(frag_rssb.redis_ctx[thread_seq]->err)
	{
		MESA_handle_runtime_log(logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} [redis exec command '%s' fail, connect error:%s].",
								__FILE__,__LINE__, cmmd, frag_rssb.redis_ctx[thread_seq]->errstr);
		redisFree(frag_rssb.redis_ctx[thread_seq]);		
		frag_rssb.redis_ctx[thread_seq] = NULL;
		rec = -1;		
	}
	else
	{
		//cmmd_reply = (redisReply *)redisCommand(frag_rssb.redis_ctx[thread_seq], cmmd);
		cmmd_reply = (redisReply *)redisCommandArgv(frag_rssb.redis_ctx[thread_seq], argc, argv, argvlen);
		/*reply为NULL，表示客户端和服务器端出现了严重错误，必须重新连接*/
		if(NULL==cmmd_reply)
		{			
			MESA_handle_runtime_log(logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} [redis exec command '%s' fail, connect error:%s].",
								__FILE__,__LINE__, cmmd, frag_rssb.redis_ctx[thread_seq]->errstr);
			redisFree(frag_rssb.redis_ctx[thread_seq]);				
			frag_rssb.redis_ctx[thread_seq] = NULL;
			freeReplyObject(cmmd_reply);
			rec = -2;
		}
		else if(cmmd_reply->type==REDIS_REPLY_ERROR)
		{
			MESA_handle_runtime_log(logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} [redis exec command '%s' fail, reply error:%s].",
									__FILE__,__LINE__, cmmd, cmmd_reply->str);
			freeReplyObject(cmmd_reply);
			rec = -2;			
		}		
	}	

	if(0==rec)
	{	
		MESA_handle_runtime_log(logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} [redis exec command '%s' succ].",
								__FILE__,__LINE__, cmmd);
	}
	*reply = cmmd_reply;
	return rec;
}

/**
*
*return : 
*		-1:connect error; 
*		-2:reply error(need to freeReplyObject) ; 
*		0:succ(need to freeReplyObject) 
*/
int redis_cluster_excute_command(int thread_seq, void* logger, redisReply** reply, char* cmmd, int argc, const char **argv, const size_t *argvlen)
{
	int rec = 0;
	redisReply* cmmd_reply = *reply;	

	if(NULL==frag_rssb.redis_cluster_ctx[thread_seq] || frag_rssb.redis_cluster_ctx[thread_seq]->err)
	{		
		if(NULL!=frag_rssb.redis_cluster_ctx[thread_seq])
		{
			redisClusterFree(frag_rssb.redis_cluster_ctx[thread_seq]);
			frag_rssb.redis_cluster_ctx[thread_seq] = NULL;
		}
		frag_rssb.redis_cluster_ctx[thread_seq] = redisClusterConnectWithTimeout(frag_rssb.redis_addr, frag_rssb.redis_tv, HIRCLUSTER_FLAG_NULL);
		//redisClusterEnableSalve(frag_rssb.redis_cluster_ctx[thread_seq], frag_rssb.redis_cluster_netflag, frag_rssb.redis_cluster_net);
	}
	if(NULL==frag_rssb.redis_cluster_ctx[thread_seq]) return -1;

	if(frag_rssb.redis_cluster_ctx[thread_seq]->err)
	{
		MESA_handle_runtime_log(logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} [redis cluster exec command '%s' fail, connect error:%s].",
								__FILE__,__LINE__, cmmd, frag_rssb.redis_cluster_ctx[thread_seq]->errstr);
		redisClusterFree(frag_rssb.redis_cluster_ctx[thread_seq]);		
		frag_rssb.redis_cluster_ctx[thread_seq] = NULL;
		rec = -1;		
	}
	else
	{
		//cmmd_reply = (redisReply *)redisClusterCommand(frag_rssb.redis_cluster_ctx[thread_seq], cmmd);
		cmmd_reply = (redisReply *)redisClusterCommandArgv(frag_rssb.redis_cluster_ctx[thread_seq], argc, argv, argvlen);
		/*reply为NULL，表示客户端和服务器端出现了严重错误，必须重新连接*/
		if(NULL==cmmd_reply)
		{			
			MESA_handle_runtime_log(logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} [redis cluster exec command '%s' fail, connect error:%s].",
								__FILE__,__LINE__, cmmd, frag_rssb.redis_cluster_ctx[thread_seq]->errstr);
			redisClusterFree(frag_rssb.redis_cluster_ctx[thread_seq]);				
			frag_rssb.redis_cluster_ctx[thread_seq] = NULL;
			freeReplyObject(cmmd_reply);
			rec = -2;
		}
		else if(cmmd_reply->type==REDIS_REPLY_ERROR)
		{
			MESA_handle_runtime_log(logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} [redis cluster exec command '%s' fail, reply error:%s].",
									__FILE__,__LINE__, cmmd, cmmd_reply->str);
			freeReplyObject(cmmd_reply);
			rec = -2;			
		}		
	}	

	if(0==rec)
	{	
		MESA_handle_runtime_log(logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} [redis cluster exec command '%s' succ].",
								__FILE__,__LINE__, cmmd);
	}
	*reply = cmmd_reply;
	return rec;
}

int redis_excute_command(int thread_seq, void* logger, redisReply** reply, char* cmmd, int argc, const char **argv, const size_t *argvlen)
{
	if(frag_rssb.redis_cluster_switch==1)
	{
	 	return redis_cluster_excute_command(thread_seq, logger, reply, cmmd, argc, argv, argvlen);
	}
	else if(frag_rssb.redis_cluster_switch==2)
	{
		return redis_nocluster_excute_command(thread_seq, logger, reply, cmmd, argc, argv, argvlen);
	}
	else
	{
		return -2;
	}
}


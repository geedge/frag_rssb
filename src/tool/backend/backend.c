
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "MESA_handle_logger.h"
#include "AV_interface.h"
#include "my_socket.h"
#include "usm_api.h"

#define UNIX_SOCKET_RECV 	0
#define USM_RECV 			1

typedef struct backend_parameter_s
{	
	void*						logger;						
	/*resp msg*/
	int							recv_data_fd;	             			
	int							loglevel;						
}backend_parameter_t;

backend_parameter_t 		g_run;
int main (int argc, char *argv[])
{
#if UNIX_SOCKET_RECV 
	int 					size = 0;
	char			 		buf[2048] = {0};
	fd_set					rset;
	msg_header_t*			mhead = NULL;
	msg_metainfo_t* 		minfo = NULL;
	msg_data_t* 			mdata = NULL;
	char*					data = NULL;	
	char*					opt_ptr = NULL;
	
	g_run.logger = MESA_create_runtime_log_handle("./log/recv_data",10);
	g_run.recv_data_fd = init_recv_unix_socket("/home/mesasoft/frag_rssb/tool/backend/un_recv");
	if(-1==g_run.recv_data_fd)
	{
		printf("init_recv_unix_socket error.\n");
		return -1;
	}
	FD_ZERO(&rset);
	FD_SET(g_run.recv_data_fd,&rset);
	while(1)
	{
		if(-1==select(g_run.recv_data_fd+1,&rset,NULL,NULL,NULL))
		{
			printf("recv_data select error.\n");
			continue;
		}		
		size = recv_unix_socket_recv(g_run.recv_data_fd, (char*)buf, sizeof(buf));
		mhead = (msg_header_t*)buf;
		minfo = (msg_metainfo_t*)(buf + MSG_HEADER_LEN);
		mdata = (msg_data_t*)(buf + MSG_HEADER_LEN);
		data = (char*)mdata + MSG_DATA_HEAD_LEN;	
		opt_ptr = NULL;
		
		/* Ignore invalid packet */
		if(PROTO_MAGICNUM != mhead->magic_num || PROTO_VERSION != mhead->version|| size <= (int)MSG_HEADER_LEN || size>1600)
		{
			continue;
		}	
		/*prco msg*/
		switch(mhead->msg_type)
		{
			case MSG_DATA_META:
				MESA_handle_runtime_log(g_run.logger, RLOG_LV_INFO, "", 
										"RECV META [PID:%llu, media_len:%llu, meida_type:0x%02x]", 
										*(uint64_t*)minfo->prog_id, minfo->prog_len, minfo->media_type);
				 
				break;
				
			case MSG_DATA_BODY: 
			case MSG_DATA_TAIL: 		
				/*write log*/		
				MESA_handle_runtime_log(g_run.logger, RLOG_LV_DEBUG, "", 
										"RECV DATA [PID:%llu, offset:%llu, datalen:%u]", 
										*(uint64_t*)mdata->prog_id, mdata->offset, mhead->cont_len-MSG_DATA_HEAD_LEN);
				break;		
		}
	}

#endif

#if USM_RECV 	
	char* 	data[372]={0};
    int 	datalen[372] = {0};
    int 	data_cnt = 0;
	int 	rst = 0;
	int 	i = 0;
	msg_header_t*			mhead = NULL;
	msg_metainfo_t* 		minfo = NULL;
	msg_data_t* 			mdata = NULL;
	char*					opt_ptr = NULL;
	g_run.logger = MESA_create_runtime_log_handle("./log/recv_data",10);

	USM_t* a_usm_handle = USM_handle(1635151988,100000,USM_READER);
	unsigned int reader_id = 0;
	USM_set_opt(a_usm_handle,READER_ID,(void*)&reader_id,sizeof(unsigned int),0);	
	unsigned int max_lq_size = 2048*2048;
	USM_set_opt(a_usm_handle,MAX_LQUEUE_SIZE,(void*)&max_lq_size,sizeof(unsigned int),0);	
	char reader_path[] = "/home/mesasoft/frag_rssb/tool/backend/un_recv";
	USM_set_opt(a_usm_handle,READER_PATH,(void*)reader_path,strlen(reader_path),0);
	USM_set_opt(a_usm_handle,LOG_HANDLE,NULL,4,0);
	rst = USM_init(a_usm_handle);
		
	while(1)
	{
		rst = USM_read(a_usm_handle,data,datalen,&data_cnt);
		for(i =0;i< data_cnt;i++)
		{
			mhead = (msg_header_t*)data[i];
			minfo = (msg_metainfo_t*)(data[i] + MSG_HEADER_LEN);
			mdata = (msg_data_t*)(data[i] + MSG_HEADER_LEN);
			opt_ptr = NULL;
			
			/* Ignore invalid packet */
			if(PROTO_MAGICNUM != mhead->magic_num || PROTO_VERSION != mhead->version|| datalen[i] <= (int)MSG_HEADER_LEN || datalen[i]>1600)
			{
				continue;
			}	
			/*prco msg*/
			switch(mhead->msg_type)
			{
				case MSG_DATA_META:
					MESA_handle_runtime_log(g_run.logger, RLOG_LV_INFO, "", 
											"RECV META [PID:%llu, media_len:%llu, meida_type:0x%02x]", 
											*(uint64_t*)minfo->prog_id, minfo->prog_len, minfo->media_type);
					 
					break;
					
				case MSG_DATA_BODY: 
				case MSG_DATA_TAIL: 		
					/*write log*/		
					MESA_handle_runtime_log(g_run.logger, RLOG_LV_DEBUG, "", 
											"RECV DATA [PID:%llu, offset:%llu, datalen:%u]", 
											*(uint64_t*)mdata->prog_id, mdata->offset, mhead->cont_len-MSG_DATA_HEAD_LEN);
					break;		
			}
		}		
	}
#endif
}	


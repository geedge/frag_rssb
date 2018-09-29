#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "usm_api.h"
#include "MESA_handle_logger.h"

#define LOG_NAME			"./log/reader2"

#define LOG_LEVEL			30

#define SUIT_WRAP_NUM 		20
typedef struct msg_header_s 
{
	uint16_t  					magic_num; 
	uint8_t 					version;	 	
	uint8_t						msg_type; 		// msg data type
	uint32_t 					cont_len; 	
}msg_header_t; /* sizeof = 8B */

#define MSG_HEADER_LEN	sizeof(msg_header_t)

typedef struct msg_metainfo_s
{
	char						prog_id[8];	
	char    					flag;
    uint8_t   					hitservice;
	uint64_t  					prog_len:48;
	uint32_t					cap_IP;
	uint8_t						protocol;		
	uint8_t						media_type;	
	uint8_t						data_flag;	
	uint8_t						opt_num;      
}msg_metainfo_t;

int main()
{
	int rst = 0;
	//USM_t* a_usm_handle = USM_handle(1233,100000,USM_READER);
	USM_t* a_usm_handle = USM_handle(1635151988,100000,USM_READER);
	unsigned int reader_id = 2;
	USM_set_opt(a_usm_handle,READER_ID,(void*)&reader_id,sizeof(unsigned int),2);

	unsigned int max_lq_size = 2048*2048;
	USM_set_opt(a_usm_handle,MAX_LQUEUE_SIZE,(void*)&max_lq_size,sizeof(unsigned int),2);

	//char reader_path[] = "./usm_test2";//test_write
	char reader_path[] = "/home/mesasoft/usm_test";//test_frag_rssb
	USM_set_opt(a_usm_handle,READER_PATH,(void*)reader_path,strlen(reader_path),2);
	void* logger = MESA_create_runtime_log_handle(LOG_NAME,LOG_LEVEL);
	USM_set_opt(a_usm_handle,LOG_HANDLE,logger,4,2);


	rst = USM_init(a_usm_handle);
	if(rst < 0)	return 0;
	
	char* data[372]={0};
	int datalen[372] = {0};
	int data_cnt =0;
	
	int j = 0;
	int i = 0;
	msg_header_t* msgheader = NULL;
	msg_metainfo_t* msginfo = NULL;
	//while(1)
	for(j = 0;j<1000000/SUIT_WRAP_NUM;j++)
	{
		/*for(i = 0;i < 372;i++)
		{
			datalen[i] = 2048;
		}*/		
		rst = USM_read(a_usm_handle,data,datalen,&data_cnt);

		//debug for online
		/*for(i =0;i< data_cnt;i++)
		{
			msgheader = (msg_header_t*)data[i];
			msginfo = (msg_metainfo_t*)(data[i] + MSG_HEADER_LEN);
			assert(msgheader->magic_num == 22081);
			if(msgheader->msg_type == 49)
			{
				assert(msginfo->opt_num == 0);
			}
		}*/
	}
	
	MESA_destroy_runtime_log_handle(logger);
	return 0;
}



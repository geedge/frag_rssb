#include <openssl/md5.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "usm_api.h"
#include "MESA_handle_logger.h"

#define LOG_NAME			"./log/md5_reader1"

#define LOG_LEVEL			30

#define SUIT_WRAP_NUM 		20

int main()
{
	int rst = 0;

	USM_t* a_usm_handle = USM_handle(1233,100000,USM_READER);

	unsigned int reader_id = 1;
	USM_set_opt(a_usm_handle,READER_ID,(void*)&reader_id,sizeof(unsigned int),0);

	unsigned int max_lq_size = 2048*2048;
	USM_set_opt(a_usm_handle,MAX_LQUEUE_SIZE,(void*)&max_lq_size,sizeof(unsigned int),1);

	char reader_path[] = "./usm_test1";
	USM_set_opt(a_usm_handle,READER_PATH,(void*)reader_path,strlen(reader_path),1);

	void* logger = MESA_create_runtime_log_handle(LOG_NAME,LOG_LEVEL);
	USM_set_opt(a_usm_handle,LOG_HANDLE,logger,4,1);


	rst = USM_init(a_usm_handle);
	if(rst < 0)	return 0;
	
    MD5_CTX c;  
    unsigned char md5[17]={0};
	MD5_Init(&c);

	char* data[372]={0};
	int datalen[372] = {0};
	int data_cnt = 0;
	int j = 0;
	int i = 0;

	for(j = 0;j<1000000/SUIT_WRAP_NUM;j++)
	{		
		for(i = 0;i < SUIT_WRAP_NUM;i++)
		{
			datalen[i] = 2048;
		}
		
		rst = USM_read(a_usm_handle,data,datalen,&data_cnt);
		
		for(i = 0;i < data_cnt;i++)
		{
			MD5_Update(&c, data[i], datalen[i]);
		}
	
	}
	MD5_Final(md5,&c);
	printf("MD5 is %x%x%x%x.\n",*(int*)md5,*((int*)md5+1),*((int*)md5+2),*((int*)md5+3));
	//free(data);
	MESA_destroy_runtime_log_handle(logger);
	return 0;	
}


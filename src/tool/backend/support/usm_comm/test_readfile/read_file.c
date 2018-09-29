#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "usm_api.h"
#include "MESA_handle_logger.h"

#define LOG_NAME			"./log/reader"

#define LOG_LEVEL			30
#define LOOP_NUM			150//test.txt line_num

int main()
{
	int rst = 0;
	USM_t* a_usm_handle = USM_handle(12345,1000,USM_READER);

	unsigned int reader_id = 0;
	USM_set_opt(a_usm_handle,READER_ID,(void*)&reader_id,sizeof(unsigned int),0);
	
	char reader_path[] = "/home/dumeijie/usm_testfile";
	USM_set_opt(a_usm_handle,READER_PATH,(void*)reader_path,strlen(reader_path),0);
	void* logger = MESA_create_runtime_log_handle(LOG_NAME,LOG_LEVEL);
	USM_set_opt(a_usm_handle,LOG_HANDLE,logger,4,0);


	rst = USM_init(a_usm_handle);
	if(rst < 0)	return 0;
	
	char* data[372]={0};
	int datalen[372] = {0};
	int data_cnt = 0;

	int i = 0;
	int j = 0;
	int k =0;
	FILE *fp;
	if(NULL == (fp = fopen("test_2.txt","wb+")))
	{
		perror("fail to read.\n");
		exit(1);
	}
	
	while(1)
	//for(i =0;i<LOOP_NUM;i++)
	{
		for(j =0;j<372;j++)
		{
			datalen[j] = 1000;
		}
		
		rst = USM_read(a_usm_handle,data,datalen,&data_cnt);
		for(k =0;k<data_cnt;k++)
		{
			fwrite(data[k],datalen[k],1,fp);
			fflush(fp);
		}
	}
	fclose(fp);
	MESA_destroy_runtime_log_handle(logger);
	return 0;
}


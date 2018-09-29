#include <openssl/md5.h> 
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "usm_api.h"
#include "MESA_handle_logger.h"

#define LOG_NAME0			"./log/md5_writer0"
#define LOG_NAME1			"./log/md5_writer1"
#define LOG_NAME2			"./log/md5_writer2"

#define LOG_LEVEL			30

int main()
{
	int rst = 0;
		
	USM_t* a_usm_handle = USM_handle(1233,100000,USM_WRITER);

	unsigned int reader_cnt = 3;
	USM_set_opt(a_usm_handle,READER_CNT,(void*)&reader_cnt,sizeof(unsigned int),0);
	
	unsigned int smooth_time = 1000000;//1ms
	USM_set_opt(a_usm_handle,SMOOTH_TIME,(void*)&smooth_time,sizeof(unsigned int),0);

	unsigned int max_lq_size = 2048*2048;
	USM_set_opt(a_usm_handle,MAX_LQUEUE_SIZE,(void*)&max_lq_size,sizeof(unsigned int),0);

	char reader_path1[] = "./usm_test0";
	USM_set_opt(a_usm_handle,READER_PATH,(void*)reader_path1,strlen(reader_path1),0);

	void* logger_0 = MESA_create_runtime_log_handle(LOG_NAME0,LOG_LEVEL);
	USM_set_opt(a_usm_handle,LOG_HANDLE,logger_0,4,0);

	void* logger_1 = MESA_create_runtime_log_handle(LOG_NAME1,LOG_LEVEL);
	void* logger_2 = MESA_create_runtime_log_handle(LOG_NAME2,LOG_LEVEL);
//reader1
	USM_set_opt(a_usm_handle,MAX_LQUEUE_SIZE,(void*)&max_lq_size,sizeof(unsigned int),1);

	char reader_path2[] = "./usm_test1";
	USM_set_opt(a_usm_handle,READER_PATH,(void*)reader_path2,strlen(reader_path2),1);
	USM_set_opt(a_usm_handle,LOG_HANDLE,logger_1,4,1);
//reader2

	USM_set_opt(a_usm_handle,MAX_LQUEUE_SIZE,(void*)&max_lq_size,sizeof(unsigned int),2);

	char reader_path3[] = "./usm_test2";
	USM_set_opt(a_usm_handle,READER_PATH,(void*)reader_path3,strlen(reader_path3),2);
	USM_set_opt(a_usm_handle,LOG_HANDLE,logger_2,4,2);


	rst = USM_init(a_usm_handle);
	if(rst < 0)	return 0;
	
	srand((int)time(0));
	int seed = rand();
	
	int data[256]={0};
	int k = 0;
	for(k = 0;k<256;k++)
	{
		data[k]= seed;
	}
    MD5_CTX c;  
    unsigned char md5[17]={0};
	MD5_Init(&c);
	
	int i = 0;
	int j = 0;
	unsigned long long wt_pktnum = 0;
	unsigned long long wt_datalen = 0;
	unsigned long long err_wting_pktnum = 0;
	unsigned long long err_wting_datalen = 0;
	unsigned long long sd_pktnum = 0;
	unsigned long long rd_pktnum = 0;
	unsigned long long rd_datalen = 0;
	unsigned long long not_rd_pktnum = 0;
	unsigned long long not_rd_datalen = 0;
	unsigned long long err_rding_pktnum = 0;
	unsigned long long err_rding_datalen = 0;
	unsigned long long lqcount = 0;
	
	struct timespec last_send_time,now;
	clock_gettime(CLOCK_MONOTONIC,&last_send_time);	
	for(j = 0;j<1000000;j++)
	{
		rst = USM_write(a_usm_handle,(const char*)data,1024);
		MD5_Update(&c, data, 1024);
	}
	clock_gettime(CLOCK_MONOTONIC,&now);
	printf("durtion time is %llu ns.\n",(now.tv_sec - last_send_time.tv_sec)*1000000000+(now.tv_nsec - last_send_time.tv_nsec));

	MD5_Final(md5,&c);
	printf("MD5 is %x%x%x%x.\n",*(int*)md5,*((int*)md5+1),*((int*)md5+2),*((int*)md5+3));

	wt_datalen = USM_stat(a_usm_handle,WRITED_CNT,0);
	printf("WRITED_CNT:				%llu.\n",wt_datalen);
	wt_pktnum = USM_stat(a_usm_handle,WRITED_SIZE,0);
	printf("WRITED_SIZE:				%llu.\n",wt_pktnum);
	err_wting_datalen = USM_stat(a_usm_handle,WRITING_CLASH_CNT,0);
	printf("WRITING_CLASH_CNT:			%llu.\n",err_wting_datalen);
	err_wting_pktnum = USM_stat(a_usm_handle,WRITING_CLASH_SIZE,0);
	printf("WRITING_CLASH_SIZE:			%llu.\n\n",err_wting_pktnum);
	for(i = 0;i < reader_cnt;i++)
	{
		sd_pktnum = USM_stat(a_usm_handle,SENDED_CNT,i);
		printf("Reader %d - SENDED_CNT:			%llu.\n",i,sd_pktnum);
		lqcount = USM_stat(a_usm_handle,LQ_COUNT,i);
		printf("Reader %d - LQ_CNT:			%llu.\n",i,lqcount);
		rd_datalen = USM_stat(a_usm_handle,READED_CNT,i);
		printf("Reader %d - READED_CNT:			%llu.\n",i,rd_datalen);
		rd_pktnum = USM_stat(a_usm_handle,READED_SIZE,i);
		printf("Reader %d - READED_SIZE:			%llu.\n",i,rd_pktnum);
		not_rd_datalen = USM_stat(a_usm_handle,READER_DROP_CNT,i);
		printf("Reader %d - READER_DROP_CNT:		%llu.\n",i,not_rd_datalen);
		not_rd_pktnum = USM_stat(a_usm_handle,READER_DROP_SIZE,i);
		printf("Reader %d - READER_DROP_SIZE:		%llu.\n",i,not_rd_pktnum);
		err_rding_datalen = USM_stat(a_usm_handle,READING_CLASH_CNT,i);
		printf("Reader %d - READING_CLASH_CNT:		%llu.\n",i,err_rding_datalen);			
		err_rding_pktnum = USM_stat(a_usm_handle,READING_CLASH_SIZE,i);
		printf("Reader %d - READING_CLASH_SIZE:		%llu.\n\n",i,err_rding_pktnum);
	}
	
	MESA_destroy_runtime_log_handle(logger_0);
	MESA_destroy_runtime_log_handle(logger_1);
	MESA_destroy_runtime_log_handle(logger_2);
	return 0;
}




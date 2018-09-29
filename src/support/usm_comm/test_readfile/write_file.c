#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "usm_api.h"
#include "MESA_handle_logger.h"

#define LOG_NAME			"./log/writer"

#define LOG_LEVEL			30

int main()
{
	int rst = 0;
	USM_t* a_usm_handle = USM_handle(12345,1000,USM_WRITER);

	unsigned int reader_cnt = 1;
	USM_set_opt(a_usm_handle,READER_CNT,(void*)&reader_cnt,sizeof(unsigned int),0);
	
	unsigned int smooth_time = 1000000;//ns
	USM_set_opt(a_usm_handle,SMOOTH_TIME,(void*)&smooth_time,sizeof(unsigned int),0);
//reader0
	unsigned int max_lq_size = 1024;
	USM_set_opt(a_usm_handle,MAX_LQUEUE_SIZE,(void*)&max_lq_size,sizeof(unsigned int),0);

	char reader_path1[] = "/home/dumeijie/usm_testfile";
	USM_set_opt(a_usm_handle,READER_PATH,(void*)reader_path1,strlen(reader_path1),0);
	
	void* logger = MESA_create_runtime_log_handle(LOG_NAME,LOG_LEVEL);
	USM_set_opt(a_usm_handle,LOG_HANDLE,logger,4,0);

	rst = USM_init(a_usm_handle);
	if(rst < 0)	return 0;
	
	char data[1000]={0};
	FILE *fp;
	if(NULL == (fp = fopen("test.txt","r")))
	{
		perror("fail to read.\n");
		exit(1);
	}

	int i = 0;
	int len = 1;
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
	struct timespec last_send_time,now;
	clock_gettime(CLOCK_MONOTONIC,&last_send_time);	

	while(fgets(data,1000,fp)!= NULL)
	{
		len = strlen(data);
		rst = USM_write(a_usm_handle,data,len);
		usleep(1);
	}
	clock_gettime(CLOCK_MONOTONIC,&now);
	printf("durtion time is %llu ns.\n",(now.tv_sec - last_send_time.tv_sec)*1000000000+(now.tv_nsec - last_send_time.tv_nsec));

	wt_datalen = USM_stat(a_usm_handle,WRITED_CNT,0);
	printf("WRITED_CNT:				%d.\n",wt_datalen);
	wt_pktnum = USM_stat(a_usm_handle,WRITED_SIZE,0);
	printf("WRITED_SIZE:				%d.\n",wt_pktnum);
	err_wting_datalen = USM_stat(a_usm_handle,WRITING_CLASH_CNT,0);
	printf("WRITING_CLASH_CNT:			%d.\n",err_wting_datalen);
	err_wting_pktnum = USM_stat(a_usm_handle,WRITING_CLASH_SIZE,0);
	printf("WRITING_CLASH_SIZE:			%d.\n\n",err_wting_pktnum);
	for(i = 0;i < reader_cnt;i++)
	{
		sd_pktnum = USM_stat(a_usm_handle,SENDED_CNT,i);
		printf("Reader %d - SENDED_CNT:			%d.\n",i,sd_pktnum);
		rd_datalen = USM_stat(a_usm_handle,READED_CNT,i);
		printf("Reader %d - READED_CNT:			%d.\n",i,rd_datalen);
		rd_pktnum = USM_stat(a_usm_handle,READED_SIZE,i);
		printf("Reader %d - READED_SIZE:			%d.\n",i,rd_pktnum);
		not_rd_datalen = USM_stat(a_usm_handle,READER_DROP_CNT,i);
		printf("Reader %d - READER_DROP_CNT:		%d.\n",i,not_rd_datalen);
		not_rd_pktnum = USM_stat(a_usm_handle,READER_DROP_SIZE,i);
		printf("Reader %d - READER_DROP_SIZE:		%d.\n",i,not_rd_pktnum);
		err_rding_datalen = USM_stat(a_usm_handle,READING_CLASH_CNT,i);
		printf("Reader %d - READING_CLASH_CNT:		%d.\n",i,err_rding_datalen);			
		err_rding_pktnum = USM_stat(a_usm_handle,READING_CLASH_SIZE,i);
		printf("Reader %d - READING_CLASH_SIZE:		%d.\n\n",i,err_rding_pktnum);
	}
	fclose(fp);
	MESA_destroy_runtime_log_handle(logger);
	//sleep(10);
	return 0;	
}


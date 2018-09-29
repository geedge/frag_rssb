#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "bizman.h"
const int magic=12345678;
struct test_header_t
{
  int magic;
  int len;
};
int recv_test(void)
{
	char d_buf[1024*1024]={1};
	void* b_sd=NULL;
	unsigned int src_ip;
	unsigned short src_port;
	int len=0;
	unsigned int is_complete=0;
	unsigned int new_stream_id=0;
	b_sd=bizman_get_handle();
	bizman_set_handle(b_sd,BIZMAN_RECEIVER_ACTIVE|BIZMAN_CHUNK_RECEIVE|BIZMAN_CHOKE_RECEIVE);
	bizman_listen(b_sd,5210);
	bizman_init_handle(b_sd);
	test_header_t *header=NULL;
	time_t last_stat_time=time(NULL);
	unsigned long long recv_bytes=0,recv_pkts=0;
	unsigned long long old_recv_bytes=0,old_recv_pkts=0;
	while(1){
		len=bizman_recv(b_sd, d_buf, sizeof(d_buf),&src_ip, &src_port,&new_stream_id,&is_complete);
		header=(test_header_t*)d_buf;
		recv_pkts++;
		recv_bytes+=len;
		if(header->magic!=magic)
		{
			printf("magic_error\n");
		}
		if(len!=header->len)
		{
			printf("len_error\n");
		}
		if(time(NULL)-last_stat_time>2)
		{
			printf("%lld bps,%lld pps\n",(recv_bytes-old_recv_bytes)/(time(NULL)-last_stat_time),(recv_pkts - old_recv_pkts)/(time(NULL)-last_stat_time));
			last_stat_time=time(NULL);
			old_recv_bytes=recv_bytes;
			old_recv_pkts= recv_pkts;
		}
	}
	bizman_destroy_handle(b_sd);
	return 1;
}
int send_test(unsigned int dst)
{
	char d_buf[1024*1024]={1};
	void* b_sd=NULL;
	struct test_header_t *header=(struct test_header_t *)d_buf;
	b_sd=bizman_get_handle();
	bizman_set_handle(b_sd, BIZMAN_SENDER_ACTIVE);
	bizman_init_handle(b_sd);
	int i=0,j=0;
	int size=100;
	while(1){
		i++;
		for(j=0;j<10;j++)
   		{
			header->len=size*(j+1);
			header->magic=magic;
			bizman_send(b_sd, j%32, dst, 5210, d_buf,header->len, i%2048, BIZMAN_RELIABLE_SEND);
		//	usleep(10);
		}
		usleep(1000);
	}
	sleep(10);
	//bizman_destroy_handle(b_sd);
	return 1;
}
void send_file_test(const char*file_name)
{
	FILE* fp=NULL;
	char *buff=NULL;
	struct stat file_info;
	unsigned int dst=16777343;
	void* b_sd=NULL;
	b_sd=bizman_get_handle();
	bizman_set_handle(b_sd, BIZMAN_SENDER_ACTIVE);
	bizman_init_handle(b_sd);
	
	if(NULL==(fp=fopen(file_name,"r")))
	{
		printf("fopen error!\n");
		return;
	}
	stat(file_name,&file_info);
	buff=(char*)malloc(sizeof(char)*(file_info.st_size));
	fread(buff,sizeof(char),file_info.st_size,fp);
	fclose(fp);
	bizman_send(b_sd,1, dst, 5210, buff, file_info.st_size, 1, BIZMAN_RELIABLE_SEND|BIZMAN_LAST_PKT);
	free(buff);
}
void recv_file_test(const char*file_name)
{
	FILE* fp=NULL;
	char buff[1024];
	unsigned int src_ip;
	unsigned short src_port;
	int len=0;
	unsigned int is_complete=0;
	unsigned int old_stream_id=0,new_stream_id=0;
	void* b_sd=NULL;
	b_sd=bizman_get_handle();
	bizman_set_handle(b_sd, BIZMAN_RECEIVER_ACTIVE);
	bizman_listen(b_sd,5210);
	bizman_init_handle(b_sd);
	
	if(NULL==(fp=fopen(file_name,"w")))
	{
		printf("fopen error!\n");
		return;
	}
	while(is_complete==0){
		memset(buff,0,sizeof(buff));
		len=bizman_recv(b_sd, buff, sizeof(buff),&src_ip, &src_port,&new_stream_id,&is_complete);
		if(len>0){
			if(old_stream_id==0)
				old_stream_id=new_stream_id;
			if(old_stream_id==new_stream_id)
				fwrite(buff,sizeof(char),len,fp);
		}
		
	}
	fclose(fp);
}
int main(int argc,char*argv[]){

  unsigned int dst=0;
	switch (*argv[1])
	{
		case 's':
			dst=inet_addr(argv[2]);
			send_test(dst);
			break;
		case 'r':		
			recv_test();
			break;
			
		default:
			return 0;
	}
	while(1){};
	
}


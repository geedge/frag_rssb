
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "./common/output.h"
#include "AV_sendback.h"


#define	BUF_SIZE	1024
#define	OPT_IMG_FP	0x10
#define	OPT_IMG_LEN	0x14


struct image_fp_t
{
	unsigned int offset;
	unsigned char fingerprint[128];
};
/***********************************************
*Function: Add option to buf,return actual add size
*Note:This function is used for 505 sendback option create.
*Author:zhengchao@iie.ac.cn
*Date:2012-11-03
***********************************************/
int add_opt_505(char *buf,unsigned int buf_len, unsigned char opt_type,const char*opt_cont,unsigned int opt_cont_len)
{
	char *ptr;
	unsigned int this_opt_len=sizeof(opt_type)+sizeof(unsigned int)+opt_cont_len;
	if(buf==NULL||buf_len<=0)
		return 0;
	if(this_opt_len>buf_len)
	{
		return 0;
	}
	ptr=buf;
	*(unsigned int *)ptr = opt_cont_len+sizeof(opt_type)+sizeof(opt_cont_len);
	ptr += sizeof(unsigned int);
	*(unsigned char *)ptr = opt_type;      
	ptr += sizeof(unsigned char);
	memcpy(ptr, opt_cont, opt_cont_len);
	return this_opt_len;
}
int main (int argc, char *argv[])
{
	if(argc!=6 && argc!=7) 
	{
		printf("input error\n");
		return 0;
	}
    char buf_survey[BUF_SIZE] = {0};	
	int w_offset=sizeof(msg_head_t)+sizeof(msg_result_t);
    msg_head_t *mh_survey = (msg_head_t*)buf_survey;
    msg_result_t *msi_survey = (msg_result_t *)(buf_survey + sizeof(msg_head_t));
	struct sockaddr_un un_backend_addr;	
    memset(buf_survey, 0, BUF_SIZE);
    mh_survey->magic = AV_MAGIC_VALUE;   
    mh_survey->m_type = AV_TYPE_RESULT;
    mh_survey->c_len = sizeof(msg_result_t);

  	*((uint64_t *)msi_survey->pid) = strtoull(argv[2], NULL, 0);
    msi_survey->servicetype = atoi(argv[3]);
	msi_survey->cfgid = atoi(argv[4]);
    msi_survey->level = atoi(argv[5]);	
	
    int fd_survey = init_output_un_socket("./un_tmp");
    if (fd_survey < 0) {
        printf("socket error!");
        exit(1);
    }

	memset(&(un_backend_addr),0,sizeof(un_backend_addr));
	strcpy(un_backend_addr.sun_path,argv[1]);
	un_backend_addr.sun_family=AF_UNIX;    
	printf("param num: %d\n", argc);
	if(argc==7)
	{		
    	w_offset += 1024-strlen(argv[6]) + sprintf(buf_survey + w_offset, argv[6]);
	}
	else
	{
		w_offset += 1024;
	}
	printf("pid:%lu, service:%d, configID:%u, level:%d, logindex:%s\n", *((uint64_t *)msi_survey->pid), msi_survey->servicetype, msi_survey->cfgid, msi_survey->level, argv[6]);
	mh_survey->c_len = w_offset-sizeof(msg_head_t);
    named_unix_domain_socket_send(fd_survey,&un_backend_addr, buf_survey, w_offset);  

    return 0;
}


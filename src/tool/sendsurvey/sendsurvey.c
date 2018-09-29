
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "./common/output.h"
#include "Com_SendToBack.h"


#define	BUF_SIZE	1024
#define	OPT_IMG_FP	0x10
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
    char buf_survey[BUF_SIZE];
	int w_offset=sizeof(struct msg_header_t)+sizeof(struct survey_ind_t);
    struct msg_header_t *mh_survey = (struct msg_header_t*)buf_survey;
    struct survey_ind_t *msi_survey = (struct survey_ind_t *)(buf_survey + MSG_HEADER_LEN);
	struct sockaddr_un un_backend_addr;
	struct image_fp_t img_fp;
    memset(buf_survey, 0, BUF_SIZE);
    mh_survey->magic_num = PROTO_MAGICNUM;
    mh_survey->version = 3;
    mh_survey->msg_type = MSG_RESP_CHECKRESULT;
    mh_survey->cont_len = SURVEY_MSG_FIX_LEN;
    //msi_survey->service = 0x2;	/* LOG_FD_BLACKIP */
    //msi_survey->service = 0x4;	/* LOG_FD_BLACKIP */
#if 1
    msi_survey->service = 0x84;/* LOG_FD_AUDIO_SAMPLE */
#endif
#if 0
    msi_survey->service = 0xB1;/* LOG_JC_VIDEO_ PICTURE */
    msi_survey->service = 0x12;/* LOG_FD_AUDIO_SPEAK */
    msi_survey->service = 0x14;/* LOG_FD_AUDIO_SEX */
    msi_survey->service = 0x21;/* LOG_FD_VIDEO_LABEL */
    msi_survey->service = 0x22;/* LOG_FD_VIDEO_CAPTION */
    msi_survey->service = 0x23;/* LOG_FD_VIDEO_SAMPLE */
    msi_survey->service = 0x25;/* LOG_FD_VIDEO_SEX*/
    msi_survey->service = 0x91;/* LOG_JC_ AUDIO_SAMPLE */
    msi_survey->service = 0x92;/* LOG_JC_ AUDIO_SPEAK */
    msi_survey->service = 0x94;/* LOG_JC_ AUDIO_SEX */
    msi_survey->service = 0xA1;/* LOG_JC_ VIDEO_LABEL */
    msi_survey->service = 0xA2;/* LOG_JC_VIDEO_CAPTION */
    msi_survey->service = 0xA3;/* LOG_JC_VIDEO_SAMPLE */
    msi_survey->service = 0x97;/* LOG_JC_ AUDIO_LANG */
    msi_survey->service = 0x98;/* LOG_JC_ AUDIO_LANG_FULL */
    msi_survey->service = 0x31;/* LOG_FD_IMAGE_SAMPLE */
#endif
	//0x97;//SERVICE_AUDIO_LANG
    //msi_survey->level = 0;
    msi_survey->level = 31;
	msi_survey->cfg_id=17;
    int fd_survey = init_output_un_socket("./un_tmp");
    if (fd_survey < 0) {
        printf("socket error!");
        exit(1);
    }

    //uint32_t ip = inet_addr("127.0.0.1");
	memset(&(un_backend_addr),0,sizeof(un_backend_addr));
	strcpy(un_backend_addr.sun_path,argv[1]);
	un_backend_addr.sun_family=AF_UNIX;
    *((uint64_t *)msi_survey->prog_id) = strtoull(argv[2], NULL, 0);
    printf("%lu\n", *((uint64_t *)msi_survey->prog_id));
	if(msi_survey->service==0x31)//FD_IMAGE_SAMPLE
	{
		//msi_survey->opt_num=1;
		img_fp.offset=5210;
		memset(img_fp.fingerprint,0xAB,sizeof(img_fp.fingerprint));
		w_offset+=add_opt_505(buf_survey + w_offset,BUF_SIZE-w_offset,OPT_IMG_FP,(const char*)&img_fp,sizeof(img_fp));
	}
    w_offset += 1 + sprintf(buf_survey + w_offset, "192.168.1.1:/home/log/xxx.log");
    mh_survey->cont_len = w_offset-sizeof(struct msg_header_t);
    named_unix_domain_socket_send(fd_survey,&un_backend_addr, 
            buf_survey, w_offset);  

    return 0;
}


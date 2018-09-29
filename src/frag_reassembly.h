#ifndef _FRAG_REASSEMBLY_H
#define _FRAG_REASSEMBLY_H


#define RETURN_FRAG 		0x01
#define RETURN_MEDIA 		0x02

typedef enum
{	
	MEDIA_INFO_NAME=0,
	MEDIA_INFO_MAXNUM,
}MEDIA_INFO_TYPE;

/*more opt_unit than msg_metainfo_t*/
typedef struct media_info_s
{		
	uint64_t					mid; 
	char						flag;
#if K_PROJECT
	int 						hitservice;
	uint8_t 					pad[5];
#else
	uint8_t 					hitservice;
#endif
	uint64_t					prog_len:48;
	uint32_t					cap_IP;
	uint8_t 					protocol;		
	uint8_t 					media_type; 
	uint8_t 					data_flag;	
	uint8_t 					opt_num; 
	struct opt_unit_t*			opt_unit;			
	uint32_t					multisrc_bizmanip;
	int8_t						mdi_flag;        //same with media_t	
	int8_t						td_query;			  	
}media_info_t;

/*same with AV_type.h*/
struct opt_unit_t
{
	uint32_t 					opt_len;    //include this variable
	uint8_t						opt_type; 
	char*						opt_value; 
};

#ifdef __cplusplus
extern "C" {
#endif

/*
1. frag_rssb_cfg_dir: path of config file; frag_rssb_log_dir: path of log file
2. return value: (0:succ   -1:error)
*/
int 	frag_reassembly_init(const char* frag_rssb_cfg_dir, const char* frag_rssb_log_dir, int thread_num);
void 	frag_reassembly_release();

/*
1. return value: ( 0: succ  -1:error )
2.data will be copied.
*/
int 	add_media_info(msg_metainfo_t* minfo, char* opt, uint32_t src_ip, int thread_seq);
//int 	add_media_info(uint64_t pid, uint16_t media_type, uint64_t media_len, uint8_t protocol, uint8_t hitservice, uint8_t data_flag, uint32_t cap_IP, char* opt, int opt_num, int thread_seq);
int 	add_frag(uint64_t pid, uint64_t offset, char* data, uint32_t datalen, uint8_t protocol, uint32_t src_ip, int thread_seq);
long 	get_media(void *data, const uint8_t *key, uint size, void *user_arg);

#ifdef __cplusplus
}
#endif


#endif

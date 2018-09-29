#ifndef _SIFTER_H
#define _SIFTER_H


#ifndef U_INT32_H
#define U_INT32_H
typedef unsigned int			uint32;
#endif


typedef void* sifter_t;

typedef struct expect_outcome_s
{
	char* 			data;			//not copy, only point to text	
	int   			datalen;	
}expect_outcome_t;

#define TEXT_NAME_LEN     32
typedef struct text_s
{	
	char*			text;
	char			text_name[TEXT_NAME_LEN];				
	uint32			text_len;		
	uchar			text_type;	
}text_t;

#ifdef __cplusplus
extern "C" {
#endif

//file dir, all file is *.sft.
sifter_t create_sifter(const char* sifter_cfg_dir, const char* sifter_log_dir, int maat_stat_swicth, int maat_perf_swicth, int thread_num, void* logger);
void 		destroy_sifter(sifter_t handle);
//return:res num; 
int 		sifter(sifter_t handle, int sftid, 
				   int expect_type, const char* expect_name, int expect_res_num, expect_outcome_t* res, 
				   const text_t* text, int text_num, const char* cont_type, unsigned int cont_type_len, int thread_seq);

#ifdef __cplusplus
}
#endif


#endif


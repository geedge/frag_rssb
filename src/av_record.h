#ifndef _AV_RECORD_H
#define _AV_RECORD_H

#include "frag_reassembly_in.h"

#define AV_RECORD_TYPR_BASIC	1   //basic information
#define AV_RECORD_TYPR_TD 		2	//TD information

typedef struct av_record_s
{	
	uint64_t				mid;
	uint64_t				media_len;					
	uint8_t					media_type;	
	uint16_t				_pad_;	
	uint32_t 				capIP;	
	char*					addr;		
	char* 					url;	
	char* 					referer;	
	uint32_t 				url_len;	
	uint32_t 				referer_len;	
	uint32_t 				addr_len;	         /*IPV4 IPV6*/
}av_record_t;

typedef struct av_digest_record_s
{	
	uint64_t				mid;
	uint64_t				media_len;	
	uint64_t				digest_len;	
	uint64_t				td_data_len;	
	char*					digest;	
	char* 					url;	
	char* 					td_ori;	   			/*°üÀ¨\0*/
	char* 					td_data;	
	uint32_t 				url_len;	
	uint32_t 				td_ori_len;	
	uint8_t					media_type;		
}av_digest_record_t;

#ifdef __cplusplus
extern "C" {
#endif

int 	av_record(frag_unit_t* frg_unit);
int 	av_digest_record(media_t* mdi);
void* 	av_record_thread(void *param);
void* 	av_digest_record_thread(void *param);

#ifdef __cplusplus
}
#endif

#endif

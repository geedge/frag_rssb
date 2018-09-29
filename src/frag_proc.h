#ifndef _FRAG_PROC_H
#define _FRAG_PROC_H

#include "frag_reassembly_in.h"

#define	PID_TYPE_DEF 		0
#define	PID_TYPE_URL 		1
#define	PID_TYPE_IPLEN	 	2
#define	PID_TYPE_CON 		3
#define	PID_TYPE_ADDR		4
#define	PID_TYPE_COUNT  	5 
#define	PID_TYPE_RESORT		6
#define	PID_TYPE_IMAGE		7

#define FLAG_SET(x, flag) 						((x) |= (flag))
#define FLAG_CLEAR(x, flag) 					((x) &= ~(flag))
#define FLAG_TEST(x, flag) 						(0 != ((x) & (flag))) 

#define SET_BIT(x, n) (  (x) |= (1<<n)   ) 
#define GET_BIT(x, n) (  ((x)>>(n)) & 1 )

#ifdef __cplusplus
extern "C" {
#endif

void create_media_write_to_log(media_t* mdi, int type, void* param);
void expire_media_write_to_log(media_t* mdi, int type, void* param);
void send_ack_to_qd(frag_in_t* frg, uint32_t src_ip, int thread_seq);
int get_serverIP(const char *addr, uint32_t addr_len, char* d_ip);
int save_qd_info(qd_info_t qd_info[], uint32_t qd_info_maxnum, uint8_t* qd_info_idx_last, uint32_t capip, uint64_t mid);
void frag_write_to_log(int type, uint64_t mid, void* param1, void* param2, uint32_t param3);
int caculate_md5(const char *meta, unsigned long meta_len, const char *data, unsigned long len, char *result, unsigned int result_len);
int frag_unit_removal(media_t* mdi, uint64_t reoffset);
int frag_removal_and_merge(IVI_t* ivi, frag_ivi_info_t* frag_ivi_info);
char* merge_addr(char* addr_buf, uint32_t* addr_buflen, char* opt_addr, uint32_t opt_addrlen, int thread_seq);

#ifdef __cplusplus
}
#endif


#endif

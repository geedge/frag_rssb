#ifndef _FRAG_RECV_H
#define _FRAG_RECV_H

/*from preproc*/
#define	BIZMAN_RECV_BUFFSIZE	(1024+512)

typedef struct bizman_recv_data_s
{		
	int			size;
	uint32_t	src_ip;
	char*		data;	
}bizman_recv_data_t;


#ifdef __cplusplus
extern "C" {
#endif

void* bizman_recv_data_from_queue(void *param);
void* bizman_recv_data_to_queue(void *param);
void* bizman_recv_data(void *param);
void* wlb_report(void *param);

#ifdef __cplusplus
}
#endif

#endif

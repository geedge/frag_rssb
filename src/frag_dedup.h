#ifndef _FRAG_DEDUP_H
#define _FRAG_DEDUP_H

#define TD_QUERY_TYPE_DEDUP		0x01					//降载查询标志位
#define TD_QUERY_TYPE_MULTISRC	0x02	                //多源查询标志位
#define TD_QUERY_TYPE_YES		0x04	 				//查询的标志位
#define TD_QUERY_ACK_DEDUP		0x10					//降载查询返回结果标志位
#define TD_QUERY_ACK_MULTISRC	0x20	                //多源查询返回结果标志位
#define TD_QUERY_RES_DEDUP		0x40	 				//降载标识
#define TD_QUERY_RES_MULTISRC	0x80	 				//多源标识
#define TD_QUERY_RES_NOREPORT	0x08	 				//上报标识

struct timer_context_t
{
	uint64_t				mid;
};

#ifdef __cplusplus
extern "C" {
#endif

void generate_td_meta(media_t* mdi);
long set_td_data(media_t* mdi, frag_in_t* frg);
void media_dedup_report(media_t* mdi);
void free_query_detail(query_detail_t* query_detail);
void proc_media_multisrc(media_t* mdi, int timeout);
long soqav_query_callback(const char *td, const query_result_t* result, void *user_arg);

#ifdef __cplusplus
}
#endif


#endif

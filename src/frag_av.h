#ifndef _FRAG_AV_H
#define _FRAG_AV_H

#define 	MAAT_RESULT_NUM						8

#ifdef __cplusplus
extern "C" {
#endif

void frag_set_offset(uint8_t media_type, frag_in_t* frg, frag_unit_t* frg_unit);
int frag_redis_index_twice(frag_unit_t* 	frg_unit);

long converge_mediainfo_search_cb(void *data, const uint8_t *key, uint size, void *user_arg);
int expire_cnvg_hash_node(void *data, int eliminate_type);
void set_frag_unit(rssb_media_info_t* media_info, frag_unit_t* frg_unit);
void proc_index_queue(frag_unit_t* frg_unit);

int is_frag(uint8_t media_type);
void media_stat(media_t* mdi, frag_unit_t* frg_unit);
void media_byte_stat(media_t* mdi, frag_unit_t* frg_unit, frag_in_t* frg);
int media_frag_removal(media_t* mdi, frag_unit_t* frg_unit);
int media_removal(media_t* mdi, frag_unit_t* frg_unit, frag_in_t* frg, frag_ivi_info_t* 	frag_ivi_info);

int av_query(frag_unit_t* frg_unit);
int redis_av_query(frag_unit_t*  frg_unit);
void index_query_timeout_free(void * context);
void index_query_timeout(void * context);
long index_query_timeout_cb(void *data, const uint8_t *key, uint size, void *user_arg);

#ifdef __cplusplus
}
#endif


#endif

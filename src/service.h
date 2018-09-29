#ifndef _SERVICE_H
#define _SERVICE_H

#include "frag_reassembly_in.h"

#define MONITOR_BUFSIZE							1500	//monitor suvery buflen
#define PROG_FLAG_EXCP 							0x01	//data need send to windows deputy system
#define PROG_FLAG_DUMP      					0x02	//monitor hit data by frontier
#define PROG_OFFSET_ZERO      					0x04	//monitor hit data by frontier
#define PROG_SEND_META      					0x08	//monitor hit data by frontier

#define	MAX_MEDIA_CHARACTER_NUM					32		//is_hit_media_detail_character

/*本地监测调试*/
#define ALL_HIT_MONITOR_SWITCH					1		//假设命中监测建议，dump文件，并且发建议
#define ALL_HIT_MONITOR_FULLFILE				2		//dumpfile 所有完整度达到一定阈值的文件，不发建议

#define	MEDIATYPE_MAXNUM					64
#define	VOIP_MEDIATYPE_MAXNUM					64

typedef struct file_id_s
{
	uint64_t 				mid;
	//time_t 					create_time;	
	//uint8_t 				media_type;	
	//uint8_t 				proto;
	FILE*					fp;
	char					filename[MAX_PATH_LEN];
} file_id_t;

typedef struct media_type_s
{
	uint8_t 				media_type;
	char 					media_type_desc[32];	
} media_type_t;

typedef struct file_frag_s
{	
	file_id_t 				key;
	uint64_t				offset_in;	
	char*					data;		
	uint32_t				datalen;
}file_frag_t;

typedef struct _media_character
{
	const unsigned char*	character;
	int 					start_offset;
	int 					len;
}media_character_t;

typedef struct dumpfile_hash_node_s
{
	FILE*					fp;
	char					filename[MAX_PATH_LEN];
}dumpfile_hash_node_t;

#ifdef __cplusplus
extern "C" {
#endif

void monitor_service_dump_file(frag_in_t* frg, media_t* mdi);
void* monitor_service_dump_file_thread(void *param);
void config_monitor_service(uint32_t src_ip, media_t* mdi);
void audio_lang_monitor_service(uint32_t src_ip, media_t* mdi);
long update_special_media_service(media_t*	mdi, frag_in_t* frg);
void set_special_media_service(uint16_t media_type, media_t* mdi);
void free_dumpfile_hash_node(void* data);
int expire_dumpfile_hash_node(void *data, int eliminate_type);
int gen_monitor_file_path(time_t create_time, uint64_t prog_id, char *file_suffix, char *path, int len);
int gen_monitor_file_dir(time_t create_time, uint64_t prog_id, char *path, int len);
char* gen_filesuffix_by_mediatype(char *file_suffix, uint8_t mediatype, uint8_t proto);

#ifdef __cplusplus
}
#endif

#endif

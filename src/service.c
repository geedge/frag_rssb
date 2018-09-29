#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <net/if.h>
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <stddef.h>

#include "MESA_handle_logger.h"

#include "service.h"
#include "common.h"
#include "bizman.h"
#include "my_socket.h"
#include "AV_interface.h"
#include "AV_sendback_in.h"
#include "frag_reassembly_in.h"
#include "log.h"
#include "frag_proc.h"
#include "frag_voip.h"

extern frag_rssb_parameter_t 		g_frag_run;
extern frag_rssb_configure_t 		g_frag_cfg;
extern frag_rssb_status_t 			g_frag_stat;
extern frag_reassembly_t 			frag_rssb;			//use media hash
extern const char* 					hash_eliminate_type[3];

media_type_t g_av_mediatype_des[MEDIATYPE_MAXNUM] = 
{
	{FILE_UNKNOWN, 		"unkonwn"},
	{FILE_VIDEO, 		"video"},	
	{FILE_WMV, 			"wmv"},
	{FILE_MPG, 			"mpg"},	
	{FILE_FLV, 			"flv"},
	{FILE_RMFF, 		"rmff"},
	{FILE_AVI, 			"avi"},
	{FILE_SWF, 			"swf"},
	{FILE_MPG4, 		"mpg4"},
	{FILE_AIFF,			"aiff"},
	{FILE_OGG, 			"ogg"},
	{FILE_DRC, 			"drc"},
	{FILE_DIRECTSHOW, 	"directshow"},
	{FILE_FLIC, 		"flic"},
	{FILE_INDEO, 		"indeo"},
	{FILE_MKV, 			"mkv"},
	{FILE_AUDIO, 		"audio"},
	{FILE_MP3, 			"mp3"},
	{MMS_TYPE, 			"mms"},
	{RTSP_RDT_TYPE, 	"rtsp_rdt"},
	{RTSP_RTP_TYPE, 	"rtsp_rtp"},
	{FILE_HLS, 			"ts"},
	{FILE_OSMF, 		"osmf"},	
	{FILE_IMAGE, 		"image"},
	{FILE_JPG,			"jpg"},
	{FILE_BMP, 			"omp"},
	{FILE_GIF, 			"gif"},
};

media_type_t g_voip_mediatype_des[VOIP_MEDIATYPE_MAXNUM] = 
{
	{AUDIO_UNKNOWN,"unkonwn"},
	{AUDIO_G711_ULAW,"G711_U"},
	{AUDIO_G711_ALAW,"G711_A"},
	{AUDIO_G722,"G722"},
 	{AUDIO_G723,"G723"},
	{AUDIO_G726_40,"G726_40"},
	{AUDIO_G726_32,"G726_32"},
	{AUDIO_G726_24,"G726_24"},
	{AUDIO_G726_16,"G726_16"},
	{AUDIO_AAL2_G726_40,"G726_40"},
	{AUDIO_AAL2_G726_32,"G726_32"},
	{AUDIO_AAL2_G726_24,"G726_24"},
	{AUDIO_AAL2_G726_16,"G726_16"},
	{AUDIO_G728,"G728"},
	{AUDIO_G729D,"G729D"},
	{AUDIO_G729E,"G729E"},
	{AUDIO_GSM,"GSM"},
	{AUDIO_GSM_EFR,"GSM_EFR"},
	{AUDIO_ILBC,"ILBC"},
	{AUDIO_AMR,"AMR"},
	{AUDIO_AMR_WB,"AMR_WB"},
	{AUDIO_SILK,"SILK"},
	{AUDIO_LPC,"LPC"},
	{AUDIO_LPC1016,"LPC1016"},
	{AUDIO_LPC1015,"LPC1015"},
	{AUDIO_L16,"L16"},
	{AUDIO_SPEEX,"SPEEX"},
	{AUDIO_L8,"L8"},
	{AUDIO_MPA,"MPA"},
	{AUDIO_DVI4,"DVI4"},
	{AUDIO_VDVI,"VDVI"},
	{AUDIO_CN,"CN"},
	{AUDIO_RED,"RED"},
	{AUDIO_QCELP,"QCELP"},
	{AUDIO_EVRC0,"EVRC0"},
	{AUDIO_EVRCB0,"EVRCB0"},
	{AUDIO_G729,"G729"},
	{AUDIO_VIVOX,"VIVOX"}
};

/***************************************dumpfile service***************************************************/
int free_file_frag(file_frag_t *data)
{	
	file_frag_t* 	file_frag = (file_frag_t*)data;
	if(NULL!=file_frag)
	{
		if(file_frag->data!=NULL)
		{
			free(file_frag->data);
		}
		free(file_frag);
	}
	return 0;
}

int gen_monitor_file_dir(time_t create_time, uint64_t prog_id, char *path, int len)
{
	if(NULL == path || len < 256 || 0 == prog_id)
	{
		return(-1);
	}
	char 				today[64] = {0};
	static const char*	hex = "0123456789ABCDEF";
	uint16_t*			p16	 = NULL;
	char*				p = NULL;
	int					i = 0;
	int					step = 0;
	int 				write_len = 0;
	struct tm			a_tm;

	localtime_r(&create_time,&a_tm);	
	memset(today, 0, sizeof(today) );
	strftime(today, sizeof(today), "%Y%m%d", &a_tm);
	
	p = path;
	if(g_frag_cfg.monitor_file_root_dir[strlen(g_frag_cfg.monitor_file_root_dir)-1]=='/')
	{
		g_frag_cfg.monitor_file_root_dir[strlen(g_frag_cfg.monitor_file_root_dir)-1]='\0';
	}
	if(g_frag_cfg.monitor_file_days)
	{
		write_len= snprintf(p,len, "%s/%s", g_frag_cfg.monitor_file_root_dir,today);
		if(write_len==len)
		{
			return -1;    
		}
		p += write_len;
	}
	p16 = (uint16_t*)&prog_id;
	for(i = 0; i < 5; i += 1)
	{
		*p++ = '/';
		if(i % 2 == 1)
		{
			*p++ = hex[(0xf000 & *p16) >> 12];
			step = 2;
		}
		*p++ = hex[(0x0f00 & *p16) >> 8];
		*p++ = hex[(0x00f0 & *p16) >> 4];
		if(i % 2 == 0)
		{
			*p++	= hex[(0x000f & *p16)];
			step	= 1;
		}
		*p	= '\0';
		p16	= (uint16_t*)( (uint8_t*)p16 + step);
	}
	return p-path;
}

int gen_monitor_file_path(time_t create_time, uint64_t prog_id, char *file_suffix, char *path, int len)
{
	int offset=0;
	int write_len=0;
	if(NULL == path || len < 256 || 0 == prog_id)
	{
		return -1;
	}
	offset = gen_monitor_file_dir(create_time, prog_id, path, len);
	write_len = snprintf(path+offset,len-offset, "/%" PRIu64 ".%s", prog_id, file_suffix);
	if(write_len==len-offset)
	{
		return -1;
	}
	offset += write_len;
	offset++;
	return offset;
}

void free_dumpfile_hash_node(void* data)
{
	dumpfile_hash_node_t* dump_hnode = (dumpfile_hash_node_t*)data;
	if(NULL!=dump_hnode)
	{
		if(NULL!=dump_hnode->fp)
		{
			fclose(dump_hnode->fp);
		}
		free(dump_hnode);
		dump_hnode = NULL;
	}
}

int expire_dumpfile_hash_node(void *data, int eliminate_type)
{
	dumpfile_hash_node_t* dump_hnode = (dumpfile_hash_node_t*)data;
	switch(eliminate_type)
	{
		case ELIMINATE_TYPE_NUM:			
			atomic_inc(&g_frag_stat.sysinfo_stat[DUMPFILE_HASH][HASH_NUM_EXPIRE]);
			break;
		case ELIMINATE_TYPE_TIME:
			atomic_inc(&g_frag_stat.sysinfo_stat[DUMPFILE_HASH][HASH_TIME_EXPIRE]);
			break;
		default:
			break;

	}
	MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
							"{%s:%d} expire_dumpfile_hash_node %s: [filemane: %s]", 
							__FILE__,__LINE__, 
							hash_eliminate_type[eliminate_type],
							dump_hnode->filename);	
	return 1;
}

char* gen_filesuffix_by_mediatype(char *file_suffix, uint8_t mediatype, uint8_t proto)
{
	if(g_frag_cfg.cpz_type == CPZ_VOIP)
	{
		for(int i=1;i<MEDIATYPE_MAXNUM;i++)
		{
			if(mediatype==g_voip_mediatype_des[i].media_type)
			{
				return g_voip_mediatype_des[i].media_type_desc;
			}
		}
		return g_voip_mediatype_des[0].media_type_desc;		
	}
	else
	{
		if(proto==AV_PROTOCOL_RTMP)
        {
                return (char*)"rtmp";
        }
		for(int i=1;i<MEDIATYPE_MAXNUM;i++)
		{
			if(mediatype==g_av_mediatype_des[i].media_type)
			{
				return g_av_mediatype_des[i].media_type_desc;
			}
		}
		return g_av_mediatype_des[0].media_type_desc;		
	}
}

long dumpfile_hash_search_cb(void *data, const uint8_t *key, uint size, void *user_arg)
{	
	file_frag_t*			file_frag = (file_frag_t*)user_arg;	
	char*					filename = file_frag->key.filename;	
    FILE*					fp = NULL;		
	int						rec = 0;	
	dumpfile_hash_node_t* 	dump_hnode = (dumpfile_hash_node_t*)data;
		
	if(NULL==dump_hnode)
	{
		rec = mkdir_r(filename);
		if(rec>=0)
		{
			fp = fopen(filename,"r+");
			if(NULL==fp)
			{
				fp = fopen(filename,"w");
				if(NULL!=fp)
				{
					fclose(fp);
				}			
				fp = fopen(filename,"r+");
			}			
			if(NULL!=fp)
			{
				dump_hnode = (dumpfile_hash_node_t*)calloc(1, sizeof(dumpfile_hash_node_t));
				dump_hnode->fp = fp;
				memcpy(dump_hnode->filename, filename, strlen(filename));
				rec = MESA_htable_add(g_frag_run.dumpfile_hash, key, size,(const void*)dump_hnode);
				if(0>rec)
				{
					MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
											"{%s:%d} dumpfile_hash MESA_htable_add error: [filename: %s]", 
											__FILE__,__LINE__, filename);
					free_dumpfile_hash_node(dump_hnode);
					dump_hnode = NULL;
				}				
			}
		}
		else
		{
			MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} dumpfile gen_monitor_file_path error: [filename:%s, mid: %llu]", 
									__FILE__,__LINE__, filename, file_frag->key.mid);
		}
	}

	if(NULL!=dump_hnode && NULL!=dump_hnode->fp)
	{
		file_frag->key.fp = dump_hnode->fp;
		MESA_handle_runtime_log(g_frag_run.frag_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
					    		"{%s:%d} DUMP FILE [filename:%s, MID:%llu, offset_in:%llu, datalen:%u]", 
					    		__FILE__,__LINE__,filename, file_frag->key.mid, file_frag->offset_in, file_frag->datalen);
	}	
	else
	{
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} dumpfile open monitor_file_path error, maybe fd is full: [filename:%s, mid: %llu]", 
								__FILE__,__LINE__, filename, file_frag->key.mid);
	}
	return 0;
}

/*---------------monitor_service_dump_file_thread---------------*/
void* monitor_service_dump_file_thread(void *param)
{
	file_frag_t*	file_frag = NULL;
	long			file_frag_len = sizeof(file_frag);	
	long 			rec_cb = 0;	
	long			tid = (long)param;
		
	while(1)
	{	
		file_frag = NULL;
		MESA_lqueue_read_head(g_frag_run.monitor_file_lq[tid], &file_frag, &file_frag_len); 
		
		MESA_htable_search_cb(g_frag_run.dumpfile_hash, (const uint8_t*)&file_frag->key.mid, sizeof(file_frag->key.mid),
							  dumpfile_hash_search_cb, file_frag, &rec_cb);		
		if(NULL!=file_frag->key.fp)
		{			
			fseek(file_frag->key.fp, file_frag->offset_in, SEEK_SET);
			fwrite(file_frag->data, file_frag->datalen, 1, file_frag->key.fp);	
			//fflush(file_frag->key.fp);
			atomic_inc(&g_frag_stat.sysinfo_stat[DUMPFILE_QUEUE][QUEUE_OUT]);
			atomic_inc(&g_frag_stat.stat_info[MONITOR_DUMP_FILE][TOTAL_PKTS]);
			atomic_add(&g_frag_stat.stat_info[MONITOR_DUMP_FILE][TOTAL_BYTES], file_frag->datalen);			
			MESA_lqueue_get_head(g_frag_run.monitor_file_lq[tid], &file_frag, &file_frag_len); 	
			MESA_handle_runtime_log(g_frag_run.frag_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
					    			"{%s:%d} DUMP FILE from queue [MID:%llu, offset_in:%llu, datalen:%u]", 
					    			__FILE__,__LINE__,file_frag->key.mid, file_frag->offset_in, file_frag->datalen);
			free_file_frag(file_frag);
		}
		else
		{
			/*file  full, sleep to wait expire*/
			sleep(g_frag_cfg.dumpfile_hash_expire_time/2);
		}
		file_frag = NULL;
	}
	return NULL;
}

void monitor_service_dump_file(frag_in_t* frg, media_t* mdi)
{
	if(mdi->monitor_path==NULL)  return;
	file_frag_t* file_frag = (file_frag_t*)calloc(1, sizeof(file_frag_t));

	if(g_frag_cfg.cpz_type==CPZ_VOIP)
	{
		/*SIP的数据包含8个字节的time+SEQ,在写本地文件的时候过滤掉*/
		file_frag->data = (char*)malloc(frg->datalen-VOIP_DATA_TIME_SEQ_LEN);
		file_frag->datalen = frg->datalen-VOIP_DATA_TIME_SEQ_LEN;	
		memcpy(file_frag->data, frg->data+VOIP_DATA_TIME_SEQ_LEN, file_frag->datalen);
	}
	else
	{
		file_frag->data = (char*)malloc(frg->datalen);
		file_frag->datalen = frg->datalen;	
		memcpy(file_frag->data, frg->data, file_frag->datalen);
	}
	file_frag->offset_in = frg->offset;
	file_frag->key.mid	= frg->mid;	
	//file_frag->key.proto = mdi->proto;	
	//file_frag->key.media_type = mdi->media_type;			
	//file_frag->key.create_time = mdi->create_time;	
	memcpy(file_frag->key.filename, mdi->monitor_path, strlen(mdi->monitor_path));
	MESA_lqueue_join_tail(g_frag_run.monitor_file_lq[frg->thread_seq], &file_frag, sizeof(file_frag));			
	atomic_inc(&g_frag_stat.sysinfo_stat[DUMPFILE_QUEUE][QUEUE_IN]);
	MESA_handle_runtime_log(g_frag_run.frag_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
							"{%s:%d} DUMP FILE to queue [MID:%llu, offset_in:%llu, datalen:%u]", 
							__FILE__,__LINE__,file_frag->key.mid, file_frag->offset_in, file_frag->datalen);
}

/**************************************monitor  service************************************/
int create_monitor(char *buf, int bufsize, media_t* mdi, uint8_t cfg_id, uint8_t service)
{
	char 						ip_str[64] = {0};
	int 						local_path_len = 0;
	int 						path_len = 0;
	msg_header_t*				monitor_header = NULL;
	resp_checkresult_t*			monitor_msg = NULL;	
	char*						file_suffix =NULL;
	
	monitor_header = (msg_header_t*)buf;
	monitor_msg = (resp_checkresult_t*)((char*)monitor_header + MSG_HEADER_LEN);

	monitor_header->magic_num = PROTO_MAGICNUM;
	monitor_header->version	= PROTO_VERSION;
	monitor_header->msg_type = MSG_RESP_CHECKRESULT;
	monitor_header->cont_len = 0;
	
	memcpy(monitor_msg->prog_id, &mdi->mid, sizeof(mdi->mid));
	monitor_msg->service = service;
	monitor_msg->level = 100; /* any number >0 is OK */
	monitor_msg->cfg_id = (uint32_t)cfg_id;

	/*文件路径*/
	if(mdi->monitor_path==NULL)
	{
		mdi->monitor_path = (char*)calloc(1, MAX_PATH_LEN);
		/*根据media_type命名文件后缀*/
		if(g_frag_cfg.all_hit_filename)
		{
			file_suffix = gen_filesuffix_by_mediatype(file_suffix, mdi->media_type, mdi->proto);
		}
		else
		{
			if(mdi->proto==AV_PROTOCOL_HTTP_STREAM)
			{
				file_suffix = (char*)"jpeg";
			}
			else
			{
				file_suffix = (char*)"flv";
			}
		}
		local_path_len = gen_monitor_file_path(mdi->create_time, mdi->mid, file_suffix, mdi->monitor_path, 1460-MSG_HEADER_LEN-MSG_RESP_CHECKRESULT_LEN);
		if(local_path_len < 0)
		{		
			return 0;
		}
	}
	inet_ntop(AF_INET, &g_frag_cfg.local_ip_nr, ip_str, sizeof(ip_str));	
	path_len = snprintf((char*)monitor_msg + MSG_RESP_CHECKRESULT_LEN, 
						1460-MSG_HEADER_LEN-MSG_RESP_CHECKRESULT_LEN, 
						"%s:%s", ip_str, mdi->monitor_path);
	path_len++; //snprintf's return lenth not including '\0',add one.	
	
	monitor_header->cont_len = path_len + MSG_RESP_CHECKRESULT_LEN;
	return  monitor_header->cont_len + MSG_HEADER_LEN;
}

long search_monitor_hash_cb(void *data, const uint8_t *key, uint size, void *user_arg)
{
	if(NULL!=data)
	{
		return 1;
	}
	return 0;
}

/*---------------audio_lang_monitor_service when create_media for every frag_unit ---------------*/
void audio_lang_monitor_service(uint32_t src_ip, media_t* mdi)
{
	long 	rec_cb = 0;	
	char 	monitor_buf[MONITOR_BUFSIZE] = {0};
	int 	monitor_len = 0;
	char	pbuf[32] = {0};	
	int 	buf_len = 32;

	MESA_htable_search_cb(g_frag_run.media_monitor_hash, (const unsigned char*)&mdi->mid, sizeof(mdi->mid), search_monitor_hash_cb, NULL, &rec_cb);
	if(rec_cb)
	{		
		monitor_len = create_monitor(monitor_buf, MONITOR_BUFSIZE, mdi, 1 , SERVICE_AUDIO_LANG_FULL);
		FLAG_SET(mdi->flag, PROG_FLAG_DUMP);
		bizman_send(g_frag_run.answer_sapp_bizman, 
					mdi->thread_seq,
					src_ip , 
					g_frag_cfg.msg_port, 
					monitor_buf, 
					monitor_len,
					1,BIZMAN_RELIABLE_SEND|BIZMAN_SMOOTH_DEST|BIZMAN_PUSH_SEND);
		
		inet_ntop(AF_INET, &src_ip, pbuf, buf_len);				
		resp_write_to_log(SEND_LANG_MONITOR, NULL, mdi, pbuf,0);
		atomic_inc(&g_frag_stat.stat_info[AUDIO_LANG_MONITOR_OUT][TOTAL_PKTS]); 
		atomic_add(&g_frag_stat.stat_info[AUDIO_LANG_MONITOR_OUT][TOTAL_BYTES], monitor_len);	
	}
}

/*---------------config_monitor_service when create_media if frag_unit hit service 0x80 form sapp---------------*/
void config_monitor_service(uint32_t src_ip, media_t* mdi)
{
	char 	monitor_buf[MONITOR_BUFSIZE] = {0};	
	int 	monitor_len = 0;	
	char	pbuf[32] = {0};	
	int 	buf_len = 32;
		
	/*回复多源之前的建议，否则前端找不到节目*/
	monitor_len = create_monitor(monitor_buf, MONITOR_BUFSIZE, mdi, 0, mdi->hit_service);

	FLAG_SET(mdi->flag, PROG_FLAG_DUMP);	
	bizman_send(g_frag_run.answer_sapp_bizman, 
				mdi->thread_seq,
				src_ip, 
				g_frag_cfg.msg_port, 
				monitor_buf, 
				monitor_len,
				1,BIZMAN_RELIABLE_SEND|BIZMAN_SMOOTH_DEST|BIZMAN_PUSH_SEND);	
			
	inet_ntop(AF_INET, &src_ip, pbuf, buf_len);				
	resp_write_to_log(SEND_CONFIG_MONITOR, NULL, mdi, pbuf,0);
	atomic_inc(&g_frag_stat.stat_info[CONFIG_MONITOR_OUT][TOTAL_PKTS]); 
	atomic_add(&g_frag_stat.stat_info[CONFIG_MONITOR_OUT][TOTAL_BYTES] , monitor_len);	
}

/******************************************************sendto wins system service****************************************************/
int is_hit_media_detail_character(const unsigned char* buf,int buf_len)
{
	int i=0,inuse_character_num=1;
	const unsigned char mp4_character[]={0x66,0x74,0x79,0x70};//mp4 from zhangdongming 20120903
	media_character_t accept_media_character[MAX_MEDIA_CHARACTER_NUM];
	accept_media_character[0].character=mp4_character;
	accept_media_character[0].start_offset=4;
	accept_media_character[0].len=sizeof(mp4_character);
	for(i=0;i<inuse_character_num;i++)
	{
		if(buf_len>=accept_media_character[i].len+accept_media_character[i].start_offset
			&&0==memcmp(buf+accept_media_character[i].start_offset,accept_media_character[i].character,accept_media_character[i].len))
		{
			return 1;
		}
	}
	return 0;
}

/*---------------update_special_media_service when frag offset=0-----------------*/
long update_special_media_service(media_t*	mdi, frag_in_t* frg)
{
	//deep judge media type to windows
	if(0==is_hit_media_detail_character((const unsigned char*)frg->data, frg->datalen))
	{
		FLAG_CLEAR(mdi->flag, PROG_FLAG_EXCP);
	}
	return 0;
}

/*---------------set_special_media_service when create_media--------------------*/
void set_special_media_service(uint16_t media_type, media_t* mdi)
{
	if(g_frag_run.special_media_table[media_type])
	{
		FLAG_SET(mdi->flag, PROG_FLAG_EXCP);
	}
}


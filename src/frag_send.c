#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <net/if.h>
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>

#include "MESA_handle_logger.h"

#include "bizman.h"

#include "main.h"
#include "frag_reassembly.h"
#include "AV_interface.h"
#include "AV_sendback.h"
#include "AV_sendback_in.h"
#include "my_socket.h"
#include "common.h"
#include "service.h"
#include "usm_api.h"
#include "frag_proc.h"

extern frag_rssb_parameter_t 		g_frag_run;
extern frag_rssb_configure_t 		g_frag_cfg;
extern frag_rssb_status_t 			g_frag_stat;
extern frag_reassembly_t 			frag_rssb;

extern "C" void* frag_forward(void *param);

msg_map_t g_av_mediatype_map[AV_MEDAI_TYPE_MAXNUM] = 
{
	{FILE_UNKNOWN,			MEDIA_TYPE_UNKNOWN},		
	{FILE_VIDEO,			MEDIA_TYPE_VIDEO},
	{FILE_WMV,				MEDIA_TYPE_WMV},
	{FILE_MPG,				MEDIA_TYPE_MPG},
	{FILE_FLV,				MEDIA_TYPE_FLV},
	{FILE_RMFF,				MEDIA_TYPE_RMFF},
	{FILE_AVI,				MEDIA_TYPE_AVI},
	{FILE_SWF,				MEDIA_TYPE_SWF},
	{FILE_MPG4,				MEDIA_TYPE_MPG4},	
	{FILE_AIFF,				MEDIA_TYPE_AIFF},
	{FILE_OGG,				MEDIA_TYPE_OGG},
	{FILE_DRC,				MEDIA_TYPE_DRC},
	{FILE_DIRECTSHOW,		MEDIA_TYPE_DIRECTSHOW},
	
	{FILE_FLIC,				MEDIA_TYPE_FLIC},
	{FILE_INDEO,			MEDIA_TYPE_INDEO},
	{FILE_MKV,				MEDIA_TYPE_MKV},
	{FILE_AUDIO,			MEDIA_TYPE_AUDIO},
	{FILE_MP3,				MEDIA_TYPE_MP3},
	{FILE_OSMF,				MEDIA_TYPE_OSMF},
	{FILE_HLS,				MEDIA_TYPE_HLS},
	{FILE_IOS,				MEDIA_TYPE_UNKNOWN},
	{FILE_ANDRIOD,			MEDIA_TYPE_UNKNOWN},
	{FILE_APP,				MEDIA_TYPE_UNKNOWN},	

	{AUDIO_UNKNOWN,			MEDIA_TYPE_AUDIO_UNKNOWN},
	{AUDIO_G711_ULAW,		MEDIA_TYPE_AUDIO_G711_ULAW},	
	{AUDIO_G711_ALAW,		MEDIA_TYPE_AUDIO_G711_ALAW},
	{AUDIO_G722,			MEDIA_TYPE_AUDIO_G722},
	{AUDIO_G723,			MEDIA_TYPE_AUDIO_G723},
	{AUDIO_G726_40,			MEDIA_TYPE_AUDIO_G726_40},
	{AUDIO_G726_32,			MEDIA_TYPE_AUDIO_G726_32},
	{AUDIO_G726_24,			MEDIA_TYPE_AUDIO_G726_24},
	{AUDIO_G726_16,			MEDIA_TYPE_AUDIO_G726_16},
	{AUDIO_AAL2_G726_40,	MEDIA_TYPE_AUDIO_AAL2_G726_40},
	{AUDIO_AAL2_G726_32,	MEDIA_TYPE_AUDIO_AAL2_G726_32},
	{AUDIO_AAL2_G726_24,	MEDIA_TYPE_AUDIO_AAL2_G726_24},
	{AUDIO_AAL2_G726_16,	MEDIA_TYPE_AUDIO_AAL2_G726_16},
	{AUDIO_G728,			MEDIA_TYPE_AUDIO_G728},
	{AUDIO_G729D,			MEDIA_TYPE_AUDIO_G729D},
	{AUDIO_G729E,			MEDIA_TYPE_AUDIO_G729E},
	{AUDIO_GSM,				MEDIA_TYPE_AUDIO_GSM},
	{AUDIO_GSM_EFR,			MEDIA_TYPE_AUDIO_GSM_EFR},
	{AUDIO_ILBC,			MEDIA_TYPE_AUDIO_ILBC},
	{AUDIO_AMR,				MEDIA_TYPE_AUDIO_AMR},
	{AUDIO_AMR_WB,			MEDIA_TYPE_AUDIO_AMR_WB},
	{AUDIO_SILK,			MEDIA_TYPE_AUDIO_SILK},
	{AUDIO_LPC,				MEDIA_TYPE_AUDIO_LPC},	
	{AUDIO_LPC1016,			MEDIA_TYPE_AUDIO_LPC1016},
	{AUDIO_LPC1015, 		MEDIA_TYPE_AUDIO_LPC1015},
	{AUDIO_L16, 			MEDIA_TYPE_AUDIO_L16},
	{AUDIO_SPEEX, 			MEDIA_TYPE_AUDIO_SPEEX},
	{AUDIO_L8, 				MEDIA_TYPE_AUDIO_L8},
	{AUDIO_MPA, 			MEDIA_TYPE_AUDIO_MPA},
	{AUDIO_DVI4, 			MEDIA_TYPE_AUDIO_DVI4},
	{AUDIO_VDVI, 			MEDIA_TYPE_AUDIO_VDVI},
	{AUDIO_CN, 				MEDIA_TYPE_AUDIO_CN},
	{AUDIO_RED, 			MEDIA_TYPE_AUDIO_RED},
	{AUDIO_QCELP, 			MEDIA_TYPE_AUDIO_QCELP},
	{AUDIO_EVRC0, 			MEDIA_TYPE_AUDIO_EVRC0},
	{AUDIO_EVRCB0, 			MEDIA_TYPE_AUDIO_EVRCB0},
	{AUDIO_G729, 			MEDIA_TYPE_AUDIO_G729},
	{AUDIO_VIVOX, 			MEDIA_TYPE_AUDIO_VIVOX},
	
	{FILE_IMAGE, 			MEDIA_TYPE_IMAGE},
	{FILE_JPG, 				MEDIA_TYPE_JPG},
	{FILE_BMP, 				MEDIA_TYPE_BMP},
	{FILE_GIF, 				MEDIA_TYPE_GIF},
	
	{MMS_TYPE, 				MEDIA_TYPE_MMS},
	{RTSP_RDT_TYPE, 		MEDIA_TYPE_RTSP_RDT},
	{RTSP_RTP_TYPE, 		MEDIA_TYPE_RTSP_RTP}	
};

/*
msg_map_t g_av_mediatype_map[AV_MEDAI_TYPE_MAXNUM] = 
{
	{FILE_UNKNOWN,			MEDIA_TYPE_UNKNOWN},		
	{FILE_VIDEO,			MEDIA_TYPE_VIDEO},
	{FILE_WMV,				MEDIA_TYPE_WMV},
	{FILE_MPG,				MEDIA_TYPE_MPG},
	{FILE_FLV,				MEDIA_TYPE_FLV},
	{FILE_RMFF,				MEDIA_TYPE_RMFF},
	{FILE_AVI,				MEDIA_TYPE_AVI},
	{FILE_SWF,				MEDIA_TYPE_SWF},
	{FILE_MPG4,				MEDIA_TYPE_MPG4},	
	{FILE_AIFF,				MEDIA_TYPE_AIFF},
	{FILE_OGG,				MEDIA_TYPE_OGG},
	{FILE_DRC,				MEDIA_TYPE_DRC},
	{FILE_DIRECTSHOW,		MEDIA_TYPE_DIRECTSHOW},
	
	{FILE_FLIC,				MEDIA_TYPE_FLIC},
	{FILE_INDEO,			MEDIA_TYPE_INDEO},
	{FILE_MKV,				MEDIA_TYPE_MKV},
	{FILE_AUDIO,			MEDIA_TYPE_AUDIO},
	{FILE_MP3,				MEDIA_TYPE_MP3},
	{FILE_OSMF,				MEDIA_TYPE_OSMF},
	{FILE_HLS,				MEDIA_TYPE_HLS},
	{FILE_IOS,				MEDIA_TYPE_UNKNOWN},
	{FILE_ANDRIOD,			MEDIA_TYPE_UNKNOWN},
	{FILE_APP,				MEDIA_TYPE_UNKNOWN},
	
	{AUDIO_UNKNOWN,			MEDIA_TYPE_VOIP},
	{AUDIO_G711_ULAW,		MEDIA_TYPE_VOIP},	
	{AUDIO_G711_ALAW,		MEDIA_TYPE_VOIP},
	{AUDIO_G722,			MEDIA_TYPE_VOIP},
	{AUDIO_G723,			MEDIA_TYPE_VOIP},
	{AUDIO_G726_40,			MEDIA_TYPE_VOIP},
	{AUDIO_G726_32,			MEDIA_TYPE_VOIP},
	{AUDIO_G726_24,			MEDIA_TYPE_VOIP},
	{AUDIO_G726_16,			MEDIA_TYPE_VOIP},
	{AUDIO_AAL2_G726_40,	MEDIA_TYPE_VOIP},
	{AUDIO_AAL2_G726_32,	MEDIA_TYPE_VOIP},
	{AUDIO_AAL2_G726_16,	MEDIA_TYPE_VOIP},
	{AUDIO_G728,			MEDIA_TYPE_VOIP},
	{AUDIO_G729D,			MEDIA_TYPE_VOIP},
	{AUDIO_G729E,			MEDIA_TYPE_VOIP},
	{AUDIO_GSM,				MEDIA_TYPE_VOIP},
	{AUDIO_GSM_EFR,			MEDIA_TYPE_VOIP},
	{AUDIO_ILBC,			MEDIA_TYPE_VOIP},
	{AUDIO_AMR,				MEDIA_TYPE_VOIP},
	{AUDIO_AMR_WB,			MEDIA_TYPE_VOIP},
	{AUDIO_SILK,			MEDIA_TYPE_VOIP},
	{AUDIO_LPC,				MEDIA_TYPE_VOIP},	
	{AUDIO_LPC1016,			MEDIA_TYPE_VOIP},
	{AUDIO_LPC1015, 		MEDIA_TYPE_VOIP},
	{AUDIO_L16, 			MEDIA_TYPE_VOIP},
	{AUDIO_SPEEX, 			MEDIA_TYPE_VOIP},
	{AUDIO_L8, 				MEDIA_TYPE_VOIP},
	{AUDIO_MPA, 			MEDIA_TYPE_VOIP},
	{AUDIO_DVI4, 			MEDIA_TYPE_VOIP},
	{AUDIO_VDVI, 			MEDIA_TYPE_VOIP},
	{AUDIO_CN, 				MEDIA_TYPE_VOIP},
	{AUDIO_RED, 			MEDIA_TYPE_VOIP},
	{AUDIO_QCELP, 			MEDIA_TYPE_VOIP},
	{AUDIO_EVRC0, 			MEDIA_TYPE_VOIP},
	{AUDIO_EVRCB0, 			MEDIA_TYPE_VOIP},
	{AUDIO_G729, 			MEDIA_TYPE_VOIP},
	{AUDIO_VIVOX, 			MEDIA_TYPE_VOIP},
	
	{FILE_IMAGE, 			MEDIA_TYPE_IMAGE},
	{FILE_JPG, 				MEDIA_TYPE_JPG},
	{FILE_BMP, 				MEDIA_TYPE_BMP},
	{FILE_GIF, 				MEDIA_TYPE_GIF},
	
	{MMS_TYPE, 				MEDIA_TYPE_MMS},
	{HTTP_STREAM_TYPE, 		MEDIA_TYPE_HTTP_STREAM},
	{RTSP_RDT_TYPE, 		MEDIA_TYPE_RTSP_RDT},
	{RTSP_RTP_TYPE, 		MEDIA_TYPE_RTSP_RTP}	
};
*/

msg_map_t g_av_proto_map[AV_PROTO_MAXNUM] = 
{
	{AV_PROTOCOL_HTTP,			PROTOCOL_HTTP},		
	{AV_PROTOCOL_SMTP,			PROTOCOL_SMTP},
	{AV_PROTOCOL_POP3,			PROTOCOL_POP3},
	{AV_PROTOCOL_IMAP,			PROTOCOL_IMAP},
	{AV_PROTOCOL_FTP,			PROTOCOL_FTP},
	{AV_PROTOCOL_HTTP_STREAM,	PROTOCOL_HTTP_PIC},
	{AV_PROTOCOL_RTSP_RDT,		PROTOCOL_RTSP_RDT},
	{AV_PROTOCOL_RTSP_RTP,		PROTOCOL_RTSP_RTP},
	{AV_PROTOCOL_MMS,			PROTOCOL_MMS},
	{AV_PROTOCOL_RTMP,			PROTOCOL_RTMP},
	{AV_PROTOCOL_SIP,			PROTOCOL_SIP},
};

void send_data_by_udp(const char* data, uint32_t datalen, int thread_id)
{
	uint32_t i = 0;	
	int send_rec = 0;

	atomic_inc(&g_frag_stat.stat_info[SRC_WINS_SEND][TOTAL_PKTS]);
	atomic_add(&g_frag_stat.stat_info[SRC_WINS_SEND][TOTAL_BYTES], datalen);	
	for(i=0;i<g_frag_cfg.send_dest_udp_ip_num;i++)
	{		
		send_rec = send_udp_socket_send(g_frag_run.send_sd[thread_id],
										g_frag_cfg.send_dest_udp_iplist[i], 
										g_frag_cfg.send_dest_udp_port[i],
										(char*)data,datalen);
		if(-1==send_rec)
		{
			/*static output*/					
			atomic_inc(&g_frag_stat.send_stat[i][FAIL_PKTS]);
			atomic_add(&g_frag_stat.send_stat[i][FAIL_BYTES], datalen);	
			
		}
		else
		{
			/*static output*/
			atomic_inc(&g_frag_stat.send_stat[i][TOTAL_PKTS]);
			atomic_add(&g_frag_stat.send_stat[i][TOTAL_BYTES], datalen);						
		}
	}	
}

void send_data_by_unixsocket(const char* data, uint32_t datalen, int thread_id)
{
	uint32_t i = 0;	
	int send_rec = 0;
	// send data to linux backend
	if(0<g_frag_cfg.send_dest_addr_num)
	{
		//static output
		atomic_inc(&g_frag_stat.stat_info[SRC_SEND_LOCAL][TOTAL_PKTS]);
		atomic_add(&g_frag_stat.stat_info[SRC_SEND_LOCAL][TOTAL_BYTES], datalen);	
		for(i=0;i<g_frag_cfg.send_dest_addr_num;i++)
		{
			send_rec = unix_socket_send(g_frag_run.send_fd[thread_id],&g_frag_cfg.send_dest_addr[i],data,datalen);

			if(-1==send_rec)
			{
				//static output				
				atomic_inc(&g_frag_stat.send_stat[i][FAIL_PKTS]);
				atomic_add(&g_frag_stat.send_stat[i][FAIL_BYTES], datalen);					
			}
			else
			{
				//static output					
				atomic_inc(&g_frag_stat.send_stat[i][TOTAL_PKTS]);
				atomic_add(&g_frag_stat.send_stat[i][TOTAL_BYTES], datalen);	
			}
		}
	}
}

void send_data(const char* data, uint32_t datalen, int thread_id)
{
	/*采用UDP回传*/
#if K_PROJECT
	send_data_by_udp(data, datalen, thread_id);		
	/*采用unix socket回传*/
#else	
	send_data_by_unixsocket(data, datalen, thread_id);
#endif
}

void send_data_bizman(const char* data, uint32_t datalen, uint64_t mid,uint32_t ip, int thread_id)
{
	atomic_inc(&g_frag_stat.stat_info[SRC_SEND_CPZ][TOTAL_PKTS]);
	atomic_add(&g_frag_stat.stat_info[SRC_SEND_CPZ][TOTAL_BYTES], datalen);	
	int i = mid%g_frag_cfg.thread_num;
	bizman_send(g_frag_run.cpz_send_bizman,
				thread_id, 
				ip, 
				g_frag_cfg.bizman_port+i, 
				data,
				datalen, 
				1,BIZMAN_RELIABLE_SEND|BIZMAN_SMOOTH_DEST|BIZMAN_PUSH_SEND);
}

void send_data_usm(const char* data, uint32_t datalen, int thread_id)
{
	uint32_t i = 0;	
	int send_rec = 0;	
	/* send data to linux backend*/
	if(0<g_frag_cfg.send_dest_addr_num)
	{
		/*static output*/
		atomic_inc(&g_frag_stat.stat_info[SRC_SEND_LOCAL][TOTAL_PKTS]);
		atomic_add(&g_frag_stat.stat_info[SRC_SEND_LOCAL][TOTAL_BYTES], datalen);	

		send_rec = USM_write(g_frag_run.a_usm_handle,data,datalen);
	
		for(i=0;i<g_frag_cfg.send_dest_addr_num;i++)
		{			
			g_frag_stat.send_stat[i][TOTAL_PKTS] = USM_stat(g_frag_run.a_usm_handle,READED_CNT,i);
			g_frag_stat.send_stat[i][TOTAL_BYTES] = USM_stat(g_frag_run.a_usm_handle,READED_SIZE,i);
			g_frag_stat.send_stat[i][FAIL_PKTS] = USM_stat(g_frag_run.a_usm_handle,READER_DROP_CNT,i);
			g_frag_stat.send_stat[i][FAIL_BYTES] = USM_stat(g_frag_run.a_usm_handle,READER_DROP_SIZE,i);
			g_frag_stat.send_lq_stat[i][TOTAL_PKTS] = USM_stat(g_frag_run.a_usm_handle,LQ_COUNT,i);
		}		
	}
}

long check_sendto_wins_cb(void *data, const uint8_t *key, uint size, void *user_arg)
{
	media_t* mdi = (media_t*)data;

	if(NULL!=mdi)
	{
		*(char*)user_arg = mdi->wins_dest_disabled_bit;		
	}
	return 0;
}

int check_sendto_wins(uint64_t mid, char* wins_dest_disabled_bit)
{
	long rec_cb = 0;
	MESA_htable_search_cb(g_frag_run.media_hash, (const uint8_t*)&mid, sizeof(mid),
						  check_sendto_wins_cb, wins_dest_disabled_bit, &rec_cb);
	return 0;
}

void send_data_to_wins(uint64_t mid, const char* data, uint32_t datalen, int thread_id)
{
	uint32_t	i = 0;	
	int 		send_rec = 0;
	uint32_t 	sendip = g_frag_cfg.special_media_wins_ip[mid % g_frag_cfg.special_media_wins_ip_num];
    char 		wins_dest_disabled_bit = 0;
    char 		wins_dest_disabled_flag[DEST_MAXNUM] = {0};

	check_sendto_wins(mid, &wins_dest_disabled_bit);
	if(FLAG_TEST(wins_dest_disabled_bit, AUDIO_WINS_DISABLE))
	{
		wins_dest_disabled_flag[0] = 1;
	}
	if(FLAG_TEST(wins_dest_disabled_bit, VEDIO_WINS_DISABLE))
	{
		wins_dest_disabled_flag[2] = 1;
	}	
	/* send data to windows backend*/
	if(g_frag_cfg.special_media_wins_port_num)
	{
		atomic_inc(&g_frag_stat.stat_info[SRC_WINS_SEND][TOTAL_PKTS]);
		atomic_add(&g_frag_stat.stat_info[SRC_WINS_SEND][TOTAL_BYTES], datalen);	
		for(i=0;i<g_frag_cfg.special_media_wins_port_num;i++)
		{
			if(wins_dest_disabled_flag[i])
			{
				continue;
			}
			send_rec = send_udp_socket_send(g_frag_run.send_windows_sd[thread_id],
											sendip,g_frag_cfg.special_media_wins_port[i],
											(char*)data,datalen);
			if(-1==send_rec)
			{
				/*static output*/					
				atomic_inc(&g_frag_stat.wins_send_stat[i][FAIL_PKTS]);
				atomic_add(&g_frag_stat.wins_send_stat[i][FAIL_BYTES], datalen);	
				
			}
			else
			{
				/*static output*/
				atomic_inc(&g_frag_stat.wins_send_stat[i][TOTAL_PKTS]);
				atomic_add(&g_frag_stat.wins_send_stat[i][TOTAL_BYTES], datalen);						
			}
		}	
	}	
}

void pack_and_send_data(frag_in_t* frg, int thread_id)
{	
	msg_head_t*	msghead = NULL;
	av_data_t* 	msgdata = NULL;
	char* 		sendbuf = NULL;
	int			sendbuflen = 0;

	/*calculate sendbuflen*/
	sendbuflen = sizeof(msg_head_t)+sizeof(av_data_t)+frg->datalen;
	sendbuf = (char*)malloc(sendbuflen);
	memset(sendbuf, 0, sendbuflen);

	/*set msg header*/
	msghead = (msg_head_t*)sendbuf;
	msghead->magic = AV_MAGIC_VALUE;
	msghead->m_type = AV_TYPE_DATA;
	msghead->c_len = sizeof(av_data_t)+frg->datalen;

	/*set msg data*/
	msgdata = (av_data_t*)(sendbuf + sizeof(msg_head_t));
	memcpy(msgdata->pid, &frg->mid, sizeof(frg->mid));	
	msgdata->frag_seq = frg->seq;
	msgdata->offset = frg->offset;

	/*send data*/
	memcpy(sendbuf+sizeof(msg_head_t)+sizeof(av_data_t), frg->data, frg->datalen);
	
	if(FLAG_TEST(frg->frag_flag, FRAG_FLAG_MULTISRC))
	{
		send_data_bizman(sendbuf, sendbuflen, frg->mid, frg->multisrc_bizmanip, thread_id);
	}
	else
	{
		if(g_frag_run.usm_on_flag)
		{
			send_data_usm(sendbuf,sendbuflen,thread_id);	
		}
		else
		{
			send_data(sendbuf, sendbuflen, thread_id);
		}
		if(g_frag_cfg.special_media_fwd_switch && FLAG_TEST(frg->frag_flag, FRAG_FLAG_WINS))
		{
			send_data_to_wins(frg->mid, sendbuf, sendbuflen, thread_id);
		}		
	}
	/*free*/
	if(NULL!=sendbuf)
	{
		free(sendbuf);
	}	
}

void pack_and_send_frag(frag_in_t* frg, int thread_id)
{	
	msg_header_t*	msghead = NULL;
	msg_data_t* 	msgdata = NULL;
	char* 			sendbuf = NULL;
	int				sendbuflen = 0;

	/*calculate sendbuflen*/
	sendbuflen = sizeof(msg_header_t)+sizeof(msg_data_t)+frg->datalen;
	sendbuf = (char*)malloc(sendbuflen);
	memset(sendbuf, 0, sendbuflen);

	/*set msg header*/
	msghead = (msg_header_t*)sendbuf;
	msghead->magic_num	= PROTO_MAGICNUM;
	msghead->version = PROTO_VERSION;
	msghead->msg_type = MSG_DATA_BODY;
	msghead->cont_len = MSG_DATA_HEAD_LEN+frg->datalen;

	/*set msg data*/
	msgdata = (msg_data_t*)(sendbuf + MSG_HEADER_LEN);
	memcpy(msgdata->prog_id, &frg->mid, sizeof(frg->mid));
	msgdata->offset = frg->offset;
	msgdata->frag_seq = frg->seq;

	/*send data*/
	memcpy(sendbuf+MSG_HEADER_LEN+MSG_DATA_HEAD_LEN, frg->data, frg->datalen);
	
	if(FLAG_TEST(frg->frag_flag, FRAG_FLAG_MULTISRC))
	{
		send_data_bizman(sendbuf, sendbuflen, frg->mid, frg->multisrc_bizmanip, thread_id);
	}
	else
	{
		if(g_frag_run.usm_on_flag)
		{
			send_data_usm(sendbuf,sendbuflen,thread_id);	
		}
		else
		{
			send_data(sendbuf, sendbuflen, thread_id);
		}
		if(g_frag_cfg.special_media_fwd_switch && FLAG_TEST(frg->frag_flag, FRAG_FLAG_WINS))
		{
			send_data_to_wins(frg->mid, sendbuf, sendbuflen, thread_id);
		}		
	}
	/*free*/
	if(NULL!=sendbuf)
	{
		free(sendbuf);
	}	
}

void send_frag(frag_in_t* frg, int thread_id)
{	
	/*采用AV_sendback.h定义的回传接口*/
#if K_PROJECT
	pack_and_send_data(frg, thread_id);		
	/*采用AV_interface.h定义的回传接口*/
#else
	pack_and_send_frag(frg, thread_id);		
#endif
}

uint8_t msg_convertion(uint8_t src, msg_map_t* map_list, int list_size)
{
	for(int i=0;i<list_size;i++)
	{
		if(src==map_list[i].nodeA)
		{
			return map_list[i].nodeB;
		}
	}
	return 0;
}

void pack_and_send_meta_info(media_info_t* media_info, frag_in_t* frg, int thread_id)
{	
	msg_head_t*		msghead = NULL;
	msg_meta_t* 	msginfo = NULL;
	char* 			sendbuf = NULL;
	int				sendbuflen = 0;	
	char* 			ptr = NULL;
	int				opt_len = 0;

	/*calculate sendbuflen*/
	sendbuflen = sizeof(msg_head_t)+sizeof(msg_meta_t);

	/*calculate opts len*/
	for(int i=0;i<media_info->opt_num;i++)
	{
		sendbuflen += media_info->opt_unit[i].opt_len;
		opt_len += media_info->opt_unit[i].opt_len;
	}
	
	/*get mem*/
	sendbuf = (char*)malloc(sendbuflen);
	memset(sendbuf, 0, sendbuflen);

	/*set msg header*/
	msghead = (msg_head_t*)sendbuf;
	msghead->magic = AV_MAGIC_VALUE;
	msghead->m_type = AV_TYPE_META;
	msghead->c_len = sizeof(msg_meta_t);

	/*set msg media info*/
	msginfo = (msg_meta_t*)(sendbuf + sizeof(msg_head_t));	
	memcpy(msginfo->pid, &media_info->mid, sizeof(media_info->mid));
	msginfo->proglen = media_info->prog_len;		
	msginfo->capip = media_info->cap_IP;
	msginfo->data_flag = media_info->data_flag;
	/*协议类型在两种结构体之间转换av_interface->av_sendback*/
	msginfo->protocol = msg_convertion(media_info->protocol, g_av_proto_map, AV_PROTO_MAXNUM);	
	/*媒体类型在两种结构体之间转换av_interface->av_sendback*/
	msginfo->mediatype = msg_convertion(media_info->media_type, g_av_mediatype_map, AV_MEDAI_TYPE_MAXNUM);	

	/*opt*/
	msginfo->opt_num = media_info->opt_num;
	ptr = sendbuf + sizeof(msg_head_t)+sizeof(msg_meta_t);
	for(int i=0;i<media_info->opt_num;i++)
	{		
		*(unsigned int *)ptr = media_info->opt_unit[i].opt_len;
		ptr += sizeof(unsigned int);
		*(unsigned char *)ptr = media_info->opt_unit[i].opt_type;
		ptr += sizeof(unsigned char);
		memcpy(ptr, media_info->opt_unit[i].opt_value, media_info->opt_unit[i].opt_len-sizeof(uint8_t)-sizeof(uint32_t));		
		ptr += media_info->opt_unit[i].opt_len-sizeof(uint8_t)-sizeof(uint32_t);
	}	
	
	/*send data*/	
	if(FLAG_TEST(frg->frag_flag, FRAG_FLAG_MULTISRC))
	{
		send_data_bizman(sendbuf, sendbuflen, media_info->mid, frg->multisrc_bizmanip, thread_id);
	}
	else
	{
		if(g_frag_run.usm_on_flag)
		{
			send_data_usm(sendbuf,sendbuflen,thread_id);	
		}
		else
		{
			send_data(sendbuf, sendbuflen, thread_id);
		}
		if(g_frag_cfg.special_media_fwd_switch && FLAG_TEST(frg->frag_flag, FRAG_FLAG_WINS))
		{
			send_data_to_wins(frg->mid, sendbuf, sendbuflen, thread_id);
		}		
	}
	/*free*/
	if(NULL!=sendbuf)
	{
		free(sendbuf);
	}	
}

void pack_and_send_media_info(media_info_t* media_info, frag_in_t* frg, int thread_id)
{	
	msg_header_t*	msghead = NULL;
	msg_metainfo_t* msginfo = NULL;
	char* 			ptr = NULL;
	char* 			sendbuf = NULL;
	int				sendbuflen = 0;
	int 			i=0;
	int				opt_len = 0;

	/*calculate sendbuflen*/
	sendbuflen = sizeof(msg_header_t)+sizeof(msg_metainfo_t);
	
	/*calculate opts len*/
	for(i=0;i<media_info->opt_num;i++)
	{
		sendbuflen += media_info->opt_unit[i].opt_len;
		opt_len += media_info->opt_unit[i].opt_len;
	}	

	/*get mem*/
	sendbuf = (char*)malloc(sendbuflen);
	memset(sendbuf, 0, sendbuflen);

	/*set msg header*/
	msghead = (msg_header_t*)sendbuf;
	msghead->magic_num	= PROTO_MAGICNUM;
	msghead->version	= PROTO_VERSION;
	msghead->msg_type = MSG_DATA_META;
	msghead->cont_len = MSG_MEDIAINFO_HEAD_LEN+opt_len;

	/*set msg media info*/
	msginfo = (msg_metainfo_t*)(sendbuf + MSG_HEADER_LEN);	
	memcpy(msginfo->prog_id, &media_info->mid, sizeof(media_info->mid));
	msginfo->prog_len = media_info->prog_len;		
	msginfo->hitservice = media_info->hitservice;	
	msginfo->cap_IP = media_info->cap_IP;
	msginfo->protocol = media_info->protocol;		
	msginfo->media_type = media_info->media_type;	
	msginfo->data_flag = media_info->data_flag;	
	msginfo->flag = media_info->flag;	
	msginfo->opt_num = media_info->opt_num;	

	ptr = sendbuf + sizeof(msg_header_t) + sizeof(msg_metainfo_t);
	for(i=0;i<media_info->opt_num;i++)
	{		
		*(unsigned int *)ptr = media_info->opt_unit[i].opt_len;
		ptr += sizeof(unsigned int);
		*(unsigned char *)ptr = media_info->opt_unit[i].opt_type;
		ptr += sizeof(unsigned char);
		memcpy(ptr, media_info->opt_unit[i].opt_value, media_info->opt_unit[i].opt_len-sizeof(uint8_t)-sizeof(uint32_t));		
		ptr += media_info->opt_unit[i].opt_len-sizeof(uint8_t)-sizeof(uint32_t);
	}	

	/*send data*/	
	if(FLAG_TEST(frg->frag_flag, FRAG_FLAG_MULTISRC))
	{
		send_data_bizman(sendbuf, sendbuflen, media_info->mid, frg->multisrc_bizmanip, thread_id);
	}
	else
	{
		if(g_frag_run.usm_on_flag)
		{
			send_data_usm(sendbuf,sendbuflen,thread_id);	
		}
		else
		{
			send_data(sendbuf, sendbuflen, thread_id);
		}
		if(g_frag_cfg.special_media_fwd_switch && FLAG_TEST(frg->frag_flag, FRAG_FLAG_WINS))
		{
			send_data_to_wins(frg->mid, sendbuf, sendbuflen, thread_id);
		}		
	}
	/*free*/
	if(NULL!=sendbuf)
	{
		free(sendbuf);
	}	
}

void send_media_info(media_info_t* media_info, frag_in_t* frg, int thread_id)
{
	/*采用AV_sendback.h定义的回传接口*/
#if K_PROJECT
		pack_and_send_meta_info(media_info, frg, thread_id);
#else
	/*采用AV_interface.h定义的回传接口*/
		pack_and_send_media_info(media_info, frg, thread_id);
#endif
}

void* frag_forward(void *param)
{
	long			thread_seq = (long)param;
	frag_in_t* 		frg_in = NULL;
	long 			frglen = sizeof(frg_in);	
	media_info_t 	media_info;	
	long			rec_cb = 0;

	while(1)
	{
		int rec = MESA_lqueue_get_head(frag_rssb.wait_lq[thread_seq], &frg_in, &frglen);
		if (rec<0)
		{		
			usleep(10);
		}	
		else
		{	
			memset(&media_info,0,sizeof(media_info_t));	
			if(FLAG_TEST(frg_in->frag_flag, FRAG_FLAG_SEND_META))
			{
				/*borrow only*/
				media_info.prog_len = frg_in->datalen;
				MESA_htable_search_cb(g_frag_run.media_hash, (const uint8_t *)&frg_in->mid,sizeof(frg_in->mid),
									  get_media, (void*)&media_info, &rec_cb);			
				/*多源，使用旧的mid查询节目信息*/
				/*
				if(FLAG_TEST(frg_in->frag_flag, FRAG_FLAG_MULTISRC))
				{
					MESA_htable_search_cb(g_frag_run.media_hash, (const uint8_t *)&frg_in->old_mid,sizeof(frg_in->old_mid),
										  get_media, (void*)&media_info, &rec_cb);
				}
				else
				{
					MESA_htable_search_cb(g_frag_run.media_hash, (const uint8_t *)&frg_in->mid,sizeof(frg_in->mid),
										  get_media, (void*)&media_info, &rec_cb);
				}
				*/
				if(rec_cb)
				{					
					frag_write_to_log(GET_META, media_info.mid, &media_info, NULL, 0);				
					send_media_info(&media_info, frg_in, thread_seq);	
					MESA_handle_runtime_log(g_frag_run.frag_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
				    						"{%s:%d} SEND META [MID:%llu, proto:%hu, media_len:%llu, media_type:0x%02x]", 
				    						__FILE__,__LINE__, media_info.mid, media_info.protocol, media_info.prog_len, media_info.media_type);									
					atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_OUTPUT]);
					/*free mediainfo opt , this is not good free outside*/
					if(NULL!=media_info.opt_unit)
					{
						for(int i=0;i<media_info.opt_num;i++)
						{
							if(NULL!=media_info.opt_unit[i].opt_value)
							{
								free(media_info.opt_unit[i].opt_value);
								media_info.opt_unit[i].opt_value = NULL;
							}
						}
						free(media_info.opt_unit);
						media_info.opt_unit = NULL;					
					}	
				}				
			}
			/*多源的情况下，需要修改mid为新的mid*/
			frg_in->mid = frg_in->new_mid;			
			send_frag(frg_in, thread_seq); 				
			frag_write_to_log(GET_FRAG, frg_in->mid, frg_in, NULL, 0);	
			atomic_inc(&frag_rssb.sysinfo_stat[RSSB_WAIT_QUEUE][QUEUE_OUT]);
			MESA_handle_runtime_log(g_frag_run.frag_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} SEND DATA [MID:%llu, PID:%llu, seq:%u, offset:%llu, datalen:%u]", 
									__FILE__,__LINE__, frg_in->mid, frg_in->pid, frg_in->seq, frg_in->offset, frg_in->datalen); 								
			free_frag_in(frg_in, 0, NULL);
			frg_in = NULL;						
		}
	}	
	return NULL;
}

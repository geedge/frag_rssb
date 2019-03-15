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

#include "MESA_htable.h"

#include "field_stat2.h"
#include "frag_reassembly_in.h"
#include "log.h"
#include "main.h"

extern frag_rssb_parameter_t 		g_frag_run;
extern frag_rssb_configure_t 		g_frag_cfg;
extern frag_rssb_status_t 			g_frag_stat;
extern frag_reassembly_t 			frag_rssb;

void get_queuelog_count()
{
	uint32_t i=0;
	g_frag_stat.sysinfo_stat[BIZMAN_RECV_QUEUE][QUEUE_CURRENT]=0;
	g_frag_stat.sysinfo_stat[APP_QUEUE][QUEUE_CURRENT]=0;
	g_frag_stat.sysinfo_stat[DUMPFILE_QUEUE][QUEUE_CURRENT]=0;
	g_frag_stat.sysinfo_stat[AV_RECORD_QUEUE][QUEUE_CURRENT]=0;
	g_frag_stat.sysinfo_stat[AV_DIGEST_RECORD_QUEUE][QUEUE_CURRENT]=0;
	if(g_frag_cfg.bizman_queue_mode)
	{
		for(i=0;(g_frag_cfg.bizman_queue_mode && i<g_frag_cfg.thread_num);i++)
		{
			g_frag_stat.sysinfo_stat[BIZMAN_RECV_QUEUE][QUEUE_CURRENT] += MESA_lqueue_get_count(g_frag_run.recv_bizman_lq[i]);
		}
	}	
	if(g_frag_cfg.app_switch)
	{
		g_frag_stat.sysinfo_stat[APP_QUEUE][QUEUE_CURRENT] += MESA_lqueue_get_count(g_frag_run.app_lq);
	}
	if(g_frag_cfg.monitor_file_switch)
	{
		for(i=0;i<g_frag_cfg.thread_num;i++)
		{
			g_frag_stat.sysinfo_stat[DUMPFILE_QUEUE][QUEUE_CURRENT] += MESA_lqueue_get_count(g_frag_run.monitor_file_lq[i]);
		}
	}
	if(g_frag_cfg.avrecord_switch)
	{
		for(i=0;i<g_frag_cfg.thread_num;i++)
		{
			g_frag_stat.sysinfo_stat[AV_RECORD_QUEUE][QUEUE_CURRENT] += MESA_lqueue_get_count(g_frag_run.av_record_lq[i]);
		}
		for(i=0;i<g_frag_cfg.thread_num;i++)
		{
			g_frag_stat.sysinfo_stat[AV_DIGEST_RECORD_QUEUE][QUEUE_CURRENT] += MESA_lqueue_get_count(g_frag_run.av_digest_record_lq[i]);
		}
	}
}

void get_hashlog_count()
{
	g_frag_stat.sysinfo_stat[MONITOR_HASH][HASH_CURRENT] = MESA_htable_get_elem_num(g_frag_run.media_monitor_hash);
	g_frag_stat.sysinfo_stat[DUMPFILE_HASH][HASH_CURRENT] = MESA_htable_get_elem_num(g_frag_run.dumpfile_hash);	
	g_frag_stat.sysinfo_stat[MEDIA_HASH][HASH_CURRENT] = MESA_htable_get_elem_num(g_frag_run.media_hash);
}

void* thread_sysinfo_output(void *param)
{
	g_frag_stat.syslog_column_id[NUM_CUR] = FS_register(g_frag_stat.sysfs_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"queue_cur(hash_cur)");
	g_frag_stat.syslog_column_id[NUM_IN] = FS_register(g_frag_stat.sysfs_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"queue_in(hash_num_expire)");
	g_frag_stat.syslog_column_id[NUM_OUT] = FS_register(g_frag_stat.sysfs_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"queue_out(hash_time_expire)");

	
	g_frag_stat.syslog_line_id[BIZMAN_RECV_QUEUE] = FS_register(g_frag_stat.sysfs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"bizman_queue");
	g_frag_stat.syslog_line_id[MULTISRC_QUEUE] = FS_register(g_frag_stat.sysfs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"multi_queue");
	g_frag_stat.syslog_line_id[APP_QUEUE] = FS_register(g_frag_stat.sysfs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"app_queue");
	g_frag_stat.syslog_line_id[DUMPFILE_QUEUE] = FS_register(g_frag_stat.sysfs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"dumpfile_queue");
	g_frag_stat.syslog_line_id[AV_RECORD_QUEUE] = FS_register(g_frag_stat.sysfs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"av_record_queue");
	g_frag_stat.syslog_line_id[AV_DIGEST_RECORD_QUEUE] = FS_register(g_frag_stat.sysfs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"av_digest_record_queue");
	g_frag_stat.syslog_line_id[MONITOR_HASH] = FS_register(g_frag_stat.sysfs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"monitor_hash");
	g_frag_stat.syslog_line_id[DUMPFILE_HASH] = FS_register(g_frag_stat.sysfs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"dumpfile_hash");
	g_frag_stat.syslog_line_id[MEDIA_HASH] = FS_register(g_frag_stat.sysfs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"media_hash");

	FS_start(g_frag_stat.sysfs_handle);
	uint32_t i=0, j=0;
	while(g_frag_stat.sysinfo_interval>0)
	{
		sleep(g_frag_stat.sysinfo_interval);
		get_queuelog_count();		
		get_hashlog_count();	
		for(i=0;i<SYSLOG_TYPE_MAXNUM;i++)
        {
            for(j=0;j<SYSLOG_STAT_MAXNUM;j++)
            {
                FS_operate(g_frag_stat.sysfs_handle, g_frag_stat.syslog_line_id[i],g_frag_stat.syslog_column_id[j],FS_OP_SET, g_frag_stat.sysinfo_stat[i][j]);
            }
        }        
		FS_passive_output(g_frag_stat.sysfs_handle);
	}	
	return NULL;	
	
}

void* thread_stat_output(void *param)
{	
	g_frag_stat.log_field_id[LOG_MEDIA_CREATE] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"create_media");
	g_frag_stat.log_field_id[LOG_MEDIA_RENEW] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"renew_media");
	g_frag_stat.log_field_id[LOG_MEDIA_EXPIRE] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"expire_media");
	g_frag_stat.log_field_id[LOG_MEDIA_OUTPUT] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"output_media");	
	g_frag_stat.log_field_id[LOG_MEDIA_HTTP] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"media_http");
	g_frag_stat.log_field_id[LOG_MEDIA_OFFSET_ZERO] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"media_offset_0");
	g_frag_stat.log_field_id[LOG_MEDIA_DEDUP_QUERY] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"dedup_query");	
	g_frag_stat.log_field_id[LOG_MEDIA_TD_QUERY] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"TD_query");	
	g_frag_stat.log_field_id[LOG_MEDIA_DEDUP_ACK] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"dedup_ack");	
	g_frag_stat.log_field_id[LOG_MEDIA_DEDUP] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"media_dedup");	
	g_frag_stat.log_field_id[LOG_MEDIA_MULTI] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"media_multi");	
	g_frag_stat.log_field_id[LOG_MEDIA_DEDUP_ACK_TIMEOUT] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"ack_timeout");	
	g_frag_stat.log_field_id[LOG_MEDIA_DEDUP_TIMEOUT] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"dedup_timeout");	
	g_frag_stat.log_field_id[LOG_MEDIA_DEDUP_REPORT] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"dedup_report_all");	
	g_frag_stat.log_field_id[LOG_MEDIA_DEDUP_REPORT_COMPLETE] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"dedup_report_normal");	
	g_frag_stat.log_field_id[LOG_AV_RECORD] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"AV_record");
	g_frag_stat.log_field_id[LOG_AV_DIGEST_RECORD] = FS_register(g_frag_stat.fs_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"AV_digest_record");
	
	g_frag_stat.log_column_id[TOTAL_PKTS] = FS_register(g_frag_stat.fs_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"total_pkts");
	g_frag_stat.log_column_id[TOTAL_BYTES] = FS_register(g_frag_stat.fs_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"total_len");
	g_frag_stat.log_column_id[PPS] = FS_register(g_frag_stat.fs_handle,FS_STYLE_COLUMN,FS_CALC_SPEED,"pps");
	g_frag_stat.log_column_id[BPS] = FS_register(g_frag_stat.fs_handle,FS_STYLE_COLUMN,FS_CALC_SPEED,"Bps");
	g_frag_stat.log_column_id[FAIL_PKTS] = FS_register(g_frag_stat.fs_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"fail_pkts");
	g_frag_stat.log_column_id[FAIL_BYTES] = FS_register(g_frag_stat.fs_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"fail_len");
	g_frag_stat.log_column_id[FAIL_PPS] = FS_register(g_frag_stat.fs_handle,FS_STYLE_COLUMN,FS_CALC_SPEED,"fail_pps");
	g_frag_stat.log_column_id[FAIL_BPS] = FS_register(g_frag_stat.fs_handle,FS_STYLE_COLUMN,FS_CALC_SPEED,"fail_Bps");

	g_frag_stat.log_line_id[RECV]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"all_recv");	
	g_frag_stat.log_line_id[RECV_DROP]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"recv_drop");	
	g_frag_stat.log_line_id[INVALID_RECV]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"invalid_recv");
	g_frag_stat.log_line_id[META_RECV]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"meta_recv");
	g_frag_stat.log_line_id[DATA_RECV]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"data_recv");
	g_frag_stat.log_line_id[OTHER_RECV]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"other_recv");
	g_frag_stat.log_line_id[MEDIA_NOMETA]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"media_nometa");
	g_frag_stat.log_line_id[RESP_UNRECOGNIZED]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"unknow_response_msg");
	g_frag_stat.log_line_id[RESP_CHARACTER]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"resp_character");
	
	g_frag_stat.log_line_id[INVALID_RESP_CHECKRESULT]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"invalid_response_msg");	
	g_frag_stat.log_line_id[RESP_CHECKRESULT]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"normal_response_msg");
	g_frag_stat.log_line_id[RESP_CHECKRESULT_WHITELIST_IN]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"whitelist_recv");
	g_frag_stat.log_line_id[RESP_CHECKRESULT_BLACKLIST_IN]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"blocklist_recv");
	g_frag_stat.log_line_id[RESP_CHECKRESULT_BLACKLIST_IN_NOFOUND]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"blocklist_nofound");

	g_frag_stat.log_line_id[RESP_CHECKRESULT_VOIP_FULLLOG]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"voip_fulllog");
	g_frag_stat.log_line_id[RESP_CHECKRESULT_VOIP_SURVEYLOG]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"voip_surveylog");	
	g_frag_stat.log_line_id[RESP_CHECKRESULT_BLACKLIST_IN_REPEAT]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"blocklist_repeat");
	g_frag_stat.log_line_id[RESP_CHECKRESULT_BLACKLIST_IN_MONITOR_LEVEL1]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"blocklist_monitor_1");
	g_frag_stat.log_line_id[RESP_CHECKRESULT_BLACKLIST_IN_MONITOR_LEVEL2]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"blocklist_monitor_2");
	g_frag_stat.log_line_id[RESP_CHECKRESULT_FRAG_SURVEY]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"frag_survey");
	g_frag_stat.log_line_id[RESP_CHECKRESULT_INDEX_SURVEY]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"index_survey");
	g_frag_stat.log_line_id[RESP_CHECKRESULT_BLACKLIST_IN_BLOCK]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"blocklist_block");

	g_frag_stat.log_line_id[INVALID_RESP_PROG_SYNC]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"invalid_prog_sync");	
	g_frag_stat.log_line_id[RESP_PROG_SYNC]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"normal_prog_sync");	
	g_frag_stat.log_line_id[RESP_PROG_SYNC_IN_NOFOUND]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"sync_prog_nofound");	
	g_frag_stat.log_line_id[RESP_PROG_SYNC_AUDIO]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"prog_sync_audio");	
	g_frag_stat.log_line_id[RESP_PROG_SYNC_VIDEO]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"prog_sync_video");	

	g_frag_stat.log_line_id[RESP_CHECKRESULT_BLACKLIST_OUT_BLOCK]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"block_to_qd");
	g_frag_stat.log_line_id[AUDIO_LANG_MONITOR_OUT]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"monitor_audio_lang");
	g_frag_stat.log_line_id[CONFIG_MONITOR_OUT]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"monitor_config");
	g_frag_stat.log_line_id[MONITOR_DUMP_FILE]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"monitor_dumpfile");	

	g_frag_stat.log_line_id[RESP_CHARACTER_ACK]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"prog_ack");	

	g_frag_stat.log_line_id[TO_SEND]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"to_send");
	g_frag_stat.log_line_id[FRAG_DEDUP]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"frag_dedup");
	g_frag_stat.log_line_id[DEDUP_DROP]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"dedup_drop");
	g_frag_stat.log_line_id[WAIT_QUEUE_FULL_DROP]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"waitlq_fulldrop");
	g_frag_stat.log_line_id[SRC_SEND_LOCAL]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"send_local");
	g_frag_stat.log_line_id[SRC_SEND_CPZ]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"send_cpz");
	g_frag_stat.log_line_id[SRC_WINS_SEND]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"send_wins");
	g_frag_stat.log_line_id[MEDIA_JSON]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"media_json");
	g_frag_stat.log_line_id[SURVEY_JSON]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"survey_json");	

	
	char		buf[32] = {0};
	int 		send_dest_num = 0;
	if(g_frag_cfg.send_udp_switch)
	{
		send_dest_num = g_frag_cfg.send_dest_udp_ip_num;
		for(uint32_t m=0;m<g_frag_cfg.send_dest_udp_ip_num;m++)
		{		
			inet_ntop(AF_INET, &g_frag_cfg.send_dest_udp_iplist[m], buf, sizeof(buf));
			g_frag_stat.sendlog_line_id[2*m]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT, "udp_send");
			g_frag_stat.sendlog_line_id[2*m+1]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT, "lq_count");
		}
	}
	else
	{	
		send_dest_num = g_frag_cfg.send_dest_addr_num;
		for(uint32_t m=0;m<g_frag_cfg.send_dest_addr_num;m++)
		{
			g_frag_stat.sendlog_line_id[2*m]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT, (char*)g_frag_cfg.send_dest_addr[m].sun_path);
	        g_frag_stat.sendlog_line_id[2*m+1]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT, "lq_count");
		}
	}
	memset(buf, 0, sizeof(buf));
	for(uint32_t m=0;m<g_frag_cfg.special_media_wins_port_num;m++)
	{
		snprintf(buf, sizeof(buf), "%hu", ntohs(g_frag_cfg.special_media_wins_port[m]));
		g_frag_stat.wins_sendlog_line_id[m]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,buf);
	}	
	memset(buf, 0, sizeof(buf));
	for(uint32_t m=0;m<g_frag_cfg.whitelist_addr_num;m++)
	{
		inet_ntop(AF_INET, &g_frag_cfg.whitelist_addr[m].sin_addr.s_addr, buf, sizeof(buf));
		g_frag_stat.whitelist_sendlog_line_id[m]=FS_register(g_frag_stat.fs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"whitelist");
	}	

	FS_start(g_frag_stat.fs_handle);
	uint32_t i=0, j=0, k=0;
	while(g_frag_stat.stat_interval>0)
	{
		sleep(g_frag_stat.stat_interval);	
			
		for(i=0;i<MEDIALOG_TYPE_MAXNUM;i++)
		{
			FS_operate(g_frag_stat.fs_handle, g_frag_stat.log_field_id[i],0,FS_OP_SET, g_frag_stat.media_stat[i]);
		}
		
		for(i=0;i<LOG_TYPE_MAXNUM;i++)
		{			
			for(j=0,k=0;j<LOG_STAT_MAXNUM;j++)
			{
				switch(j)
				{
					case PPS:
						k=TOTAL_PKTS;
						break;

					case BPS:						
						k=TOTAL_BYTES;
						break;

					case FAIL_PPS:						
						k=FAIL_PKTS;
						break;

					case FAIL_BPS:						
						k=FAIL_BYTES;
						break;

					default:
						k=j;
						break;		
				}				
				FS_operate(g_frag_stat.fs_handle, g_frag_stat.log_line_id[i],g_frag_stat.log_column_id[j],FS_OP_SET, g_frag_stat.stat_info[i][k]);
			}
		}

		for(i=0;i<send_dest_num;i++)
		{
			for(j=0,k=0;j<LOG_STAT_MAXNUM;j++)
			{
				switch(j)
				{
					case PPS:
						k=TOTAL_PKTS;
						break;

					case BPS:						
						k=TOTAL_BYTES;
						break;

					case FAIL_PPS:						
						k=FAIL_PKTS;
						break;

					case FAIL_BPS:						
						k=FAIL_BYTES;
						break;

					default:
						k=j;
						break;		
				}		
				FS_operate(g_frag_stat.fs_handle, g_frag_stat.sendlog_line_id[2*i],g_frag_stat.log_column_id[j],FS_OP_SET, g_frag_stat.send_stat[i][k]);
			}
		}
		for(i=0;i<send_dest_num;i++)
        {
        	FS_operate(g_frag_stat.fs_handle, g_frag_stat.sendlog_line_id[2*i+1],g_frag_stat.log_column_id[TOTAL_PKTS],FS_OP_SET, g_frag_stat.send_lq_stat[i][TOTAL_PKTS]);
        }
		for(i=0;i<g_frag_cfg.special_media_wins_port_num;i++)
		{
			for(j=0,k=0;j<LOG_STAT_MAXNUM;j++)
			{	
				switch(j)
				{
					case PPS:
						k=TOTAL_PKTS;
						break;

					case BPS:						
						k=TOTAL_BYTES;
						break;

					case FAIL_PPS:						
						k=FAIL_PKTS;
						break;

					case FAIL_BPS:						
						k=FAIL_BYTES;
						break;

					default:
						k=j;
						break;		
				}		
				FS_operate(g_frag_stat.fs_handle, g_frag_stat.wins_sendlog_line_id[i],g_frag_stat.log_column_id[j],FS_OP_SET, g_frag_stat.wins_send_stat[i][k]);
			}
		}
		for(i=0;i<g_frag_cfg.whitelist_addr_num;i++)
		{
			for(j=0,k=0;j<LOG_STAT_MAXNUM;j++)
			{
				switch(j)
				{
					case PPS:
						k=TOTAL_PKTS;
						break;

					case BPS:						
						k=TOTAL_BYTES;
						break;

					case FAIL_PPS:						
						k=FAIL_PKTS;
						break;

					case FAIL_BPS:						
						k=FAIL_BYTES;
						break;

					default:
						k=j;
						break;		
				}		
				FS_operate(g_frag_stat.fs_handle, g_frag_stat.whitelist_sendlog_line_id[i],g_frag_stat.log_column_id[j],FS_OP_SET, g_frag_stat.whitelist_send_stat[i][k]);
			}
		}			
		FS_passive_output(g_frag_stat.fs_handle);
	}	
	return NULL;	
}

void get_rssb_queuelog_count()
{
	/*cnvg queue and index queue is frag_unit, not gloabl*/
	frag_rssb.sysinfo_stat[RSSB_WAIT_QUEUE][QUEUE_CURRENT]=0;
	for(uint32_t i=0;i<g_frag_cfg.thread_num;i++)
	{
		frag_rssb.sysinfo_stat[RSSB_WAIT_QUEUE][QUEUE_CURRENT] += MESA_lqueue_get_count(frag_rssb.wait_lq[i]);
	}
	double queue_usage = (double)frag_rssb.sysinfo_stat[RSSB_WAIT_QUEUE][QUEUE_CURRENT]/(double)(g_frag_cfg.thread_num*frag_rssb.wait_lq_num)*100;
	if(queue_usage < g_frag_cfg.bfd_threshold)
	{
		clrbit(g_frag_cfg.bfd_down_stat,g_frag_cfg.bfd_down_index);
	}
	else
	{
		setbit(g_frag_cfg.bfd_down_stat,g_frag_cfg.bfd_down_index);
	}
	g_frag_cfg.bfd_down_index++;
	g_frag_cfg.bfd_down_index = g_frag_cfg.bfd_down_index%g_frag_cfg.bfd_down_num;	
}

void get_rssb_hashlog_count()
{
	frag_rssb.sysinfo_stat[RSSB_CNVG_HASH][HASH_CURRENT] = MESA_htable_get_elem_num(frag_rssb.converge_hash);
}

void* thread_frag_rssb_sysinfo_output(void *param)
{
	frag_rssb.syslog_column_id[NUM_CUR] = FS_register(frag_rssb.sysinfo_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"queue_cur(hash_cur)");
	frag_rssb.syslog_column_id[NUM_IN] = FS_register(frag_rssb.sysinfo_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"queue_in(hash_num_expire)");
	frag_rssb.syslog_column_id[NUM_OUT] = FS_register(frag_rssb.sysinfo_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"queue_out(hash_time_expire)");

	frag_rssb.syslog_line_id[RSSB_CNVG_QUEUE]=FS_register(frag_rssb.sysinfo_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"cnvg_queue");
	frag_rssb.syslog_line_id[RSSB_INDEX_QUEUE]=FS_register(frag_rssb.sysinfo_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"index_queue");
	frag_rssb.syslog_line_id[RSSB_WAIT_QUEUE]=FS_register(frag_rssb.sysinfo_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"wait_queue");	
	frag_rssb.syslog_line_id[RSSB_CNVG_HASH]=FS_register(frag_rssb.sysinfo_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"cnvg_hash");

	FS_start(frag_rssb.sysinfo_handle);
	int i=0, j=0;
	while(frag_rssb.sysinfo_interval>0)
	{
		sleep(frag_rssb.sysinfo_interval);
		get_rssb_queuelog_count();
		get_rssb_hashlog_count();
		for(i=0;i<RSSB_SYSLOG_TYPE_MAXNUM;i++)
        {
            for(j=0;j<SYSLOG_STAT_MAXNUM;j++)
            {
                FS_operate(frag_rssb.sysinfo_handle, frag_rssb.syslog_line_id[i],frag_rssb.syslog_column_id[j],FS_OP_SET, frag_rssb.sysinfo_stat[i][j]);
            }
        }        
		FS_passive_output(frag_rssb.sysinfo_handle);		
	}
	return NULL;
}

void* thread_frag_rssb_stat_output(void *param)
{
	frag_rssb.log_field_id[RSSB_CREATE_FRAG_UNIT] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"add_frag_unit");
	frag_rssb.log_field_id[RSSB_FREE_FRAG_UNIT] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"free_frag_unit");
	frag_rssb.log_field_id[RSSB_FRAG_UNIT_EXIST] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"frag_unit_exist");
	frag_rssb.log_field_id[RSSB_FRAG_UNIT_ERROR] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"free_frag_error");	
	frag_rssb.log_field_id[RSSB_FRAG_FORECAST] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"frag_forecast");
	frag_rssb.log_field_id[RSSB_FRAG_FORECAST_OK] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"frag_forecast_ok");
	frag_rssb.log_field_id[RSSB_CNVG_ONCE_QUERY_SEND] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"cnvg_query_1_send");
	frag_rssb.log_field_id[RSSB_CNVG_TWICE_QUERY_SEND] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"cnvg_query_2_send");
	frag_rssb.log_field_id[RSSB_CNVG_ONCE_QUERY_RECV] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"cnvg_query_1_succ");
	frag_rssb.log_field_id[RSSB_CNVG_TWICE_QUERY_RECV] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"cnvg_query_2_succ");
	frag_rssb.log_field_id[RSSB_INDEX_ONCE_QUERY_SEND] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"index_query_1_send");
	frag_rssb.log_field_id[RSSB_INDEX_TWICE_QUERY_SEND] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"index_query_2_send");
	frag_rssb.log_field_id[RSSB_INDEX_ONCE_QUERY_RECV] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"index_query_1_succ");	
	frag_rssb.log_field_id[RSSB_INDEX_TWICE_QUERY_RECV] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"index_query_2_succ");	
	frag_rssb.log_field_id[RSSB_AV_ONCE_QUERY_SEND] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"av_query_1_send");
	frag_rssb.log_field_id[RSSB_AV_TWICE_QUERY_SEND] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"av_query_2_send");
	frag_rssb.log_field_id[RSSB_AV_ONCE_QUERY_RECV] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"av_query_1_succ");	
	frag_rssb.log_field_id[RSSB_AV_TWICE_QUERY_RECV] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"av_query_2_succ");	
	frag_rssb.log_field_id[RSSB_SIP_ACK_KEY1] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"sip_ack_key1");	
	frag_rssb.log_field_id[RSSB_SIP_ACK_KEY2] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"sip_ack_key2");	
	frag_rssb.log_field_id[RSSB_SIP_ACK_KEY3] = FS_register(frag_rssb.stat_handle,FS_STYLE_FIELD,FS_CALC_CURRENT,"sip_ack_key3");	

	frag_rssb.datalog_column_id[TOTAL_PKTS] = FS_register(frag_rssb.stat_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"total_media(pkt)");
	frag_rssb.datalog_column_id[TOTAL_BYTES] = FS_register(frag_rssb.stat_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"total_bytes");
	frag_rssb.datalog_column_id[PPS] = FS_register(frag_rssb.stat_handle,FS_STYLE_COLUMN,FS_CALC_SPEED,"media/s(pkt/s)");
	frag_rssb.datalog_column_id[BPS] = FS_register(frag_rssb.stat_handle,FS_STYLE_COLUMN,FS_CALC_SPEED,"Bps");

	frag_rssb.datalog_line_id[RSSB_RECV_OTHER_MEDIA] = FS_register(frag_rssb.stat_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"recv_media");
	frag_rssb.datalog_line_id[RSSB_RECV_HLS_MEDIA] = FS_register(frag_rssb.stat_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"recv_hls_media");
	frag_rssb.datalog_line_id[RSSB_HLS_TO_OTHER] = FS_register(frag_rssb.stat_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"recv_hls_other");
	frag_rssb.datalog_line_id[RSSB_RECV_OSMF_MEDIA] = FS_register(frag_rssb.stat_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"recv_osmf_media");
	frag_rssb.datalog_line_id[RSSB_OSMF_TO_OTHER] = FS_register(frag_rssb.stat_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"recv_osmf_other");	
	frag_rssb.datalog_line_id[RSSB_RECV_FRAG_MEDIA] = FS_register(frag_rssb.stat_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"recv_frag_media");	
	frag_rssb.datalog_line_id[RSSB_FRAG_TO_OTHER] = FS_register(frag_rssb.stat_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"recv_frag_other");

	
	FS_start(frag_rssb.stat_handle);
	int i=0, j=0, k=0;
	while(frag_rssb.stat_interval>0)
	{
		sleep(frag_rssb.stat_interval);
		
		for(i=0;i<RSSB_LOG_TYPE_MAXNUM;i++)
		{
			FS_operate(frag_rssb.stat_handle, frag_rssb.log_field_id[i],0,FS_OP_SET, frag_rssb.stat_info[i]);
		}
		
		for(i=0;i<RSSB_DATALOG_TYPE_MAXNUM;i++)
		{
			for(j=0,k=0;j<=BPS;j++)
			{
				switch(j)
				{
					case PPS:
						k=TOTAL_PKTS;
						break;

					case BPS:						
						k=TOTAL_BYTES;
						break;			

					default:
						k=j;
						break;		
				}		
				FS_operate(frag_rssb.stat_handle, frag_rssb.datalog_line_id[i],frag_rssb.datalog_column_id[j],FS_OP_SET, frag_rssb.data_info[i][k]);
			}
		}		
		FS_passive_output(frag_rssb.stat_handle);		
	}
	return NULL;
}



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

#include "MESA_handle_logger.h"
#include "MESA_prof_load.h"
#include "MESA_trace.h"
#include "soqav_dedup.h"
#include "app_detect.h"

#include "main.h"
#include "common.h"
#include "frag_reassembly_in.h"
#include "frag_recv.h"
#include "frag_proc.h"
#include "frag_json.h"
#include "frag_app.h"
#include "service.h"
#include "message.h"
#include "av_record.h"

#include "bizman.h"
#include "AV_interface.h"
#include "field_stat2.h"
#include "my_socket.h"
#include "usm_api.h"
#include "asmis_log.h"
#include "wired_cfg.h"
#include "wiredLB.h"
#include "hard_keepalive.h"

const char* frag_rssb_version = "2018-08-13T09:00:00";
const char* frag_rssb_version_time = "2018-08-13T09:00:00";
const char* frag_rssb_version_des = "MESA@iie rssb_maskey";

int FRAG_RSSB_VERSION_1_0_20181008 = 0;
const char* frag_rssb_version_time_in = "2018-10-08";
const char* frag_rssb_version_des_in = "hard balance";
void frag_rssb_history()
{
	//2015.11.15  v1.0  create the project	
	//2016.04.15  v1.0  converge , can return index info
	//2016.05.11  v1.0  add live heart beat;  
	//2016.05.31  v1.0  local file use  aboffset_in; stat orderby day 
	//2016.06.15  v1.0  change log level; index_hash
	//2016.08.16  v1.0  add content-type, support netdisk and webmail
	//2016.09.03  v1.0  av record
	//2016.09.07  v1.0  frag removal
	//2016.09.22  v1.0  modify maat init
	//2016.10.10  v1.0  IVI bug 
	//2016.11.03  v1.0  1. av_record add SFH		2. delete HLS relative offset	
	//2016.11.08  v1.0  1. monitor file name use hash	
	//2016.11.11  v1.0  1. add digiest_len
	//2016.12.15  v1.0  1. add more detail log 2. hash expire not free because of return 1 3. change whitelist and wins addr	
	//2016.12.22  v1.0  1. bizman_recv uncomplete_chunk. 2. no mediainfo , not create media when muti-down 3. add renew media	
	//2017.01.09  v1.0  1. trace log  2. add field_stat 
	//2017.02.13  v1.0  1. add Transfer Descriptor	
	//2017.02.23  v1.0  1. support APP
	//2017.02.24  v1.0  1. time bug 2. dedup tuple4 3. modift create and expire 4 dedup addr
	//2017.02.25  v1.0 1, log level
	//2017.03.03  v1.0 1, monitor: data_flag,  not hitservice
	//2017.03.06  v1.0 1. av_dedup , set media_len	
	//2017.03.09  v2.0 use redis to query	
	//2017.03.14  v2.0  1. mulit-bizman  多个bizman接受数据,与线程数保持一致，监听端口BizmanPort->BizmanPort+thread_num-1
					  //2. json to kafka 将粗瓶装信息以JSON文件回传给KAFKA，供跨媒体分析
					  //3. redis connect bug 解决redis 段错误的问题
					  //4. send mateinfo add localIP 回传的元信息增加IP选项
	//2017.03.15  v2.0 合并共享内存版本	
					  //2017.02.14  v1.0	  alter unix socket to usm by dumeijie
					  //2017.03.09  v1.0	  add lqnum opt by dumeijie
					  //2017.03.14  v1.0      support switch usm or unix socket 
	//2017.03.16 V2.0 
					  //fuzzy_hash增加开关FuzzyDigestSwitch
					  //没有元信息不创建frag_unit
					  //hash thread_safe = 512
	//2017.03.17 V2.0 
					  //JSON增加开关
    //2017.03.28 V2.0 
                      //log file open and close thread safe
                      //query fail or succ when once and twice
                      //record maxoffset
                      //pid and offset ack to do
                      //prog sync to do
    //2017.03.31  V2.0
    				  //dedup permit media_len=0
    //2017.04.01  V3.0
    				  //create media by metainfo
    //2017.04.06  v3.0
    				  //set_frag_unit_after_sifter bug
    //2017.04.07  v3.0
    				  //redis replay is REDIS_REPLY_STRING not REDIS_REPLY_INTEGER
    //2017.04.10  v3.0
    				  //distinguish http and frag
    //2017.04.11  v3.0
    				  //avdedup bug: too much query
    //2017.04.14   v3.0 
                      //1. redis  2. hls and osmf don't avdedup query 3. Rediscommandv
    //2017.04.26   v3.0
   					  //stat create_media who is not hls_osmf
    //2017.05.02   V3.0
    				  //redis master and slave mode
    //2017.05.04   V3.0
    				  //IVI memsize
    //2017.05.22   V3.0
    				  //FRAG_FD: survey add opt, frag index add CAPIP
    //2017.05.31   V3.0
      				  //IVI seg merge
    				  //APP asynchronous call
   //2017.06.07    V3.0 
   					  //create media and recv survey time to usec    
   //2017.06.12    V3.0 
   					  //media_create.json    media_expire.json  
   //2017.06.14    V3.0 
   					  //support SIP 
   //2017.06.21    V3.0
   					 //wait queue add limit
   //2017.06.23    V3.0
   					 //add wait queue try_join; mid insert JSON
   //2017.07.03    V3.0 
   					//writing error : pid pid_array
   //2017.07.13    V3.0 
   					//frag survey opt OPT_FRAG_URL not OPT_FRAG_SUBSTR
   					//picture_service set opt_num==0
   	//2017.07.13    V3.0 
   					//frag survey opt OPT_FRAG_URL not OPT_FRAG_SUBSTR
   					//picture_service set opt_num==0
   //2017.07.24    V3.0 
   					//kafka json add cpz_ip  
   //2017.07.30    V4.0 
   					//send VOIP log
   //2017.08.01    V4.0 
   					//support multi-source 
   //2017.08.16    V4.0 
   					//renew_media only reset IVI and so on
   //2017.08.18    V4.0 
   					//soq_dedup_query interface 
   //2017.08.23    V4.0 
   					//multisrc : change mid according urlid
   //2017.08.29    V4.0 
   					//dedup interface: return mid
   //2017.09.04    V4.0 
   					//add fwdIP
   //2017.09.07    V4.0 
   					//recv multi_src survey and send survey
   //2017.09.08    V4.0 
   					//stat log
   //2017.09.19    V4.0 
   					//fieldstat
   //2017.09.20    V4.0 
   					//1. pic do not need IVI 2. only common av renew pid
   //2017.09.22    V4.0 
   					//1. add frag survey stats 2. add voip full and survey
   //2017.09.23    V4.0
   					//1. voip data_sip_dir as rtp_tuple4
   //2017.09.29    V4.0
   					//1. voip survey dedup
   //2017.10.09    V4.0
   					//1. voip send fdlog and jclog when expire
   //2017.10.13    V4.0
   					//1. set bizman maxnum 2. free_frag_unit
   //2017.10.16    V4.0 
   					//1. sip add query stat
   //2017.10.26    V4.0 
   					//1. dedup
   //2017.11.14    V4.0 
   					//1. add TD record 2. voip log configID 3. do not send VOIP whose media_type unknown 
   //2017.11.30    v4.0 //1. redis switch 2. redis reconnect
   //2017.12.01    v4.0 //1. queue->bloclk_queue 
   //2017.12.05    v4.0 //1. add voip_json 2.dedup query_flag 3. voip cmmd prit
   //2017.12.07    v4.0 //1. VOIP full_log add pid as opt
   //2017.12.11	   v4.0 //1. add network admin and HeartBeat
   //2017.12.13	   v4.0 //1. TD data write into json kafka
   //2017.12.14	   v4.0 //1. add voip fulllog duration time opt
   //2017.12.18	   v4.0 //1. multi_src 2.redis interfece change
   //2017.12.22    v4.0 //1. query stat 2.voip pid 3. voip query 
   //2017.12.25	   v4.0 //1. redis addr_len 2. change_pid log 
   //2017.12.26	   v4.0 //1. voip add duration 2. send voip_log when expire
   //2017.12.27	   v4.0 //1. free_frag_in when is dedup 2. bizman queue
   //2018.01.02	   v4.0 //1. support VOIP, timer support multi-thread   
   //2018.01.14	   v4.0 //1. frag_cnvg_query usr av_query 2.send muliti even if dedup is not send
   //2018.01.15	   v4.0 //1. add cnvg_query_fail   
   //2018.01.17	   v4.0 //1. voip query 3 times 2. VOIP monitor: data contain seq 3. data_log:record seq
   //2018.01.18	   v4.0 //1. query log : query_1 query_2   
   //2018.01.31	   v4.0 //1. av query all but use the first url ack
   //2018.02.06	   v4.0 //1. add asmis
   //2018.02.08    v4.0 //1. sip first query when create_media
   //2018.02.28	   v4.0 //1. record all monitor file, but not send monitor suvey. implement this fuc when setting AllHitMonitorSwitch=2   
   //2018.03.15	   v4.0 //1. support frag whose offset is in URL and its media_type=0XA6
   //2018.04.24    v4.0 //1. add opt server and cont_type,ip port etc; 2.move genrate_td by dumeijie
   //2018.04.27	   v4.0 //1. add wired_cfg_create and init,2. add wiredLB report
   //2018.05.09	   v4.0 //1. frag forecast   
   //2018.05.10	   v4.0 //1. add new sendback interface, defined in AV_sendback.h
   //2018.05.17	   v4.0 //1. modify VOIP_VERSION   
   //2018.05.28	   v4.0 //1. add PIC_VERSION  
   //2018.06.01	   v4.0 //1. multi-thread   
   //2018.06.04	   v4.0 //1. set thread_safe of hash in conf   
   //2018.06.05	   v4.0 //1. test dedup_query timeout   
   //2018.06.12    v4.0 //1. add mediatype to dump filename; main.conf add, close frag forecast ; json addr
   //2018.06.27    v4.0 //1. store filename   
   //2018.06.28	   v4.0 //1. add cross-media log to kafka 2. asmis flow 1min=60s
   //2018.07.16	   v4.0 //1. media_monitor_hash set thread_safe 2. add opt in AV_sendback.h	
   //2018.07.20	   v4.0 //1. asmis_switch unit 1minutes

   //2018.07.24	   v4.0 //1. voip add data_flag 2. service set int      
   //2018.07.30	   v4.0 //1.add save_media by dmj
   //2018.08.07	   v4.0 //1. pic file monitor .jpeg   2. dumpfile before ivi
   //2018.08.16	   v4.0 //1. send config monitor when multisrc
   //2018.09.01	   v4.0 //1.alter voip sav_media; 2.add send_json_log  for K_PROJECT 
   //2018.09.13	   v4.0 //1.frag removal 
   //2018.09.20    v4.0//1 voip_fulllog add voice_dir opt
   //2018.09.27	   v4.0 //1. hard balance
}

frag_rssb_parameter_t 		g_frag_run;
frag_rssb_configure_t 		g_frag_cfg;
frag_rssb_status_t 			g_frag_stat;

const char* hash_eliminate_type[3] = 
{
	"",
	"ELIMINATE_TYPE_NUM",
	"ELIMINATE_TYPE_TIME"
};

extern "C" void* frag_forward(void *param);
extern void read_app_to_decord();

int dedup_read_conf_and_init(const char* filename)
{
	int		log_level = 0;
	char	conf_buf[MAX_PATH_LEN]={0};	
	int 	value = 0;	
	
	/*av_dedup log*/	
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_short_def(filename, "DEDUP", "DedupLogLevel", (short*)&log_level,30);
	MESA_load_profile_string_def(filename, "DEDUP", "DedupLogPath", conf_buf , sizeof(conf_buf),"./log/dedup.log");
	g_frag_run.dedup_logger = MESA_create_runtime_log_handle(conf_buf,log_level);
	if(NULL==g_frag_run.dedup_logger)
	{
		printf("[%s] dedup_logger MESA_create_runtime_log_handle error.\n", FRAG_REASSEMBLY_MODULE_NAME);
		MESA_handle_runtime_log(g_frag_run.logger,RLOG_LV_FATAL,FRAG_REASSEMBLY_MODULE_NAME,
								(char*)"[%s:%d] dedup_logger MESA_create_runtime_log_handle error." , __FILE__,__LINE__);
		return -1;
	}

	MESA_load_profile_short_def(filename, "DEDUP", "AVDedupSwitch", (short*)&g_frag_cfg.av_dedup_switch,0); 
	MESA_load_profile_short_def(filename, "DEDUP", "AVDedupInvalid", (short*)&g_frag_cfg.dedup_invalid,0);		
	MESA_load_profile_uint_def(filename, "DEDUP", "DedupTdDataSize", (uint32_t*)&g_frag_cfg.td_data_maxsize, 1024);
	MESA_load_profile_short_def(filename, "DEDUP", "MultiWaitTimeout", (short*)&g_frag_cfg.multisrc_wait_timeout, 30);
	MESA_load_profile_short_def(filename, "DEDUP", "MultiTimerCbMaxNum", (short*)&g_frag_cfg.multisrc_timer_cb_maxtime, 500);
	for(uint32_t i=0;i<g_frag_cfg.thread_num;i++)
	{
		g_frag_run.multisrc_timer[i] = MESA_timer_create(0, TM_TYPE_QUEUE);	
	}

	/*av_dedup thrift IP*/
	uint16_t		dedup_port = 0;
	uint32_t*		dedup_ip_serial = NULL;
	unsigned int 	dedup_ipnum = 0;
	addr_node_t*	addr_array = NULL;
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_short_def(filename, "DEDUP", "ThriftPort", (short*)&dedup_port, 0);	
	MESA_load_profile_uint_def(filename, "DEDUP", "ThriftIPNum", (uint32_t*)&dedup_ipnum, 0);		
	MESA_load_profile_string_def(filename, "DEDUP", "ThriftIP", conf_buf, sizeof(conf_buf),"");
	if(dedup_ipnum)
	{
		dedup_ip_serial = (unsigned int *)malloc(dedup_ipnum*sizeof(unsigned int));
		MESA_split_read_IP(conf_buf, dedup_ipnum, dedup_ip_serial);
		addr_array = (addr_node_t*)malloc(dedup_ipnum*sizeof(addr_node_t));
		for(unsigned int i=0;i<dedup_ipnum;i++)
		{
			addr_array[i].ip_nr = dedup_ip_serial[i];
			addr_array[i].port_nr = htons(dedup_port);
		}
	}	
	/*LocalIP for thrift*/
	g_frag_run.dedup_hd = soqav_dedup_plug_init(addr_array, dedup_ipnum, g_frag_cfg.local_ip_nr, g_frag_cfg.fwd_ip_nr, g_frag_cfg.thread_num, g_frag_run.dedup_logger);
	if(NULL!=dedup_ip_serial)
	{
		free(dedup_ip_serial);
		dedup_ip_serial = NULL;
	}
	if(NULL!=addr_array)
	{
		free(addr_array);
		addr_array = NULL;
	}	
	value = 0;	
	MESA_load_profile_int_def(filename, "DEDUP", "DedupBalanceNum", &value, 1000);			
	soqav_dedup_set_opt(g_frag_run.dedup_hd, DEDUP_TYPE_BALANCE_NUM, &value, sizeof(value));
	MESA_load_profile_int_def(filename, "DEDUP", "DedupQueueNum", &value, 100000);			
	soqav_dedup_set_opt(g_frag_run.dedup_hd, DEDUP_TYPE_QUEUE_NUM, &value, sizeof(value));
	MESA_load_profile_int_def(filename, "DEDUP", "DedupStatInterval", &value, 60);			
	soqav_dedup_set_opt(g_frag_run.dedup_hd, DEDUP_TYPE_STAT_INTERVAL, &value, sizeof(value));
	MESA_load_profile_int_def(filename, "DEDUP", "DedupThriftTime", &value, 5);			
	soqav_dedup_set_opt(g_frag_run.dedup_hd, DEDUP_TYPE_THRIFT_TIME, &value, sizeof(value));
	MESA_load_profile_int_def(filename, "DEDUP", "DedupHtableArg", &value, 1000);			
	soqav_dedup_set_opt(g_frag_run.dedup_hd, DEDUP_TYPE_HTABLE_ARG, &value, sizeof(value));	

	g_frag_run.cpz_send_bizman = bizman_get_handle();
	bizman_set_handle(g_frag_run.cpz_send_bizman, BIZMAN_SENDER_ACTIVE);
	bizman_init_handle(g_frag_run.cpz_send_bizman);	
	return 0;
}

int multimedia_read_conf_and_init(const char* filename)
{
	MESA_load_profile_short_def(filename, "MULTIMEDIA", "MediaJSONSwitch", (short*)&g_frag_cfg.media_json_switch,0); 
	// Kafka初始化
	if(MESA_load_profile_string_nodef(filename, "MULTIMEDIA", "MediaJSONKafkaBrokers", g_frag_cfg.kafka_brokers, sizeof(g_frag_cfg.kafka_brokers))<0)
	{
		printf("get [NETWORK]KafkaBrokers error.\n");
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} get [NETWORK]KafkaBrokers error.", 
								__FILE__,__LINE__, g_frag_cfg.kafka_brokers);
	}
	g_frag_run.kafka_producer = new KafkaProducer(g_frag_cfg.kafka_brokers);
	if(NULL==g_frag_run.kafka_producer)
	{
		printf("KafkaProducer error.\n");
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} KafkaProducer %s error.", 
								__FILE__,__LINE__, g_frag_cfg.kafka_brokers);
	}
	if(0!=g_frag_run.kafka_producer->KafkaConnection())
	{	
		printf("KafkaConnection %s error.\n", g_frag_cfg.kafka_brokers);
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} KafkaConnection %s error.", 
								__FILE__,__LINE__, g_frag_cfg.kafka_brokers);
	}
	else
	{
		printf("KafkaConnection %s succ.\n", g_frag_cfg.kafka_brokers);
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} KafkaConnection %s succ.", 
								__FILE__,__LINE__, g_frag_cfg.kafka_brokers);
	}
	//创建topic
	if((g_frag_run.kafka_producer->CreateTopicHandle(TOPIC_MEDIA_CREATE_JSON)) == NULL)
	{
		printf("Kafka CreateTopicHandle %s failed.", TOPIC_MEDIA_CREATE_JSON);
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} Kafka CreateTopicHandle %s failed.", 
								__FILE__,__LINE__, TOPIC_MEDIA_CREATE_JSON);
	}	
	if((g_frag_run.kafka_producer->CreateTopicHandle(TOPIC_MEDIA_EXPIRE_JSON)) == NULL)
	{
		printf("Kafka CreateTopicHandle %s failed.", TOPIC_MEDIA_EXPIRE_JSON);
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} Kafka CreateTopicHandle %s failed.", 
								__FILE__,__LINE__, TOPIC_MEDIA_EXPIRE_JSON);
	}
	if((g_frag_run.kafka_producer->CreateTopicHandle(TOPIC_SURVEY_JSON)) == NULL)
	{
		printf("Kafka CreateTopicHandle %s failed.", TOPIC_SURVEY_JSON);
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} Kafka CreateTopicHandle %s failed.", 
								__FILE__,__LINE__, TOPIC_SURVEY_JSON);
	}
	if((g_frag_run.kafka_producer->CreateTopicHandle(TOPIC_VOIP_CREATE_JSON)) == NULL)
        {
                printf("Kafka CreateTopicHandle %s failed.", TOPIC_VOIP_CREATE_JSON);
                MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME,
                                                                "{%s:%d} Kafka CreateTopicHandle %s failed.",
                                                                __FILE__,__LINE__, TOPIC_VOIP_CREATE_JSON);
        }
        if((g_frag_run.kafka_producer->CreateTopicHandle(TOPIC_VOIP_EXPIRE_JSON)) == NULL)
        {
                printf("Kafka CreateTopicHandle %s failed.", TOPIC_VOIP_EXPIRE_JSON);
                MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME,
                                                                "{%s:%d} Kafka CreateTopicHandle %s failed.",
                                                                __FILE__,__LINE__, TOPIC_VOIP_EXPIRE_JSON);
        }
        if((g_frag_run.kafka_producer->CreateTopicHandle(TOPIC_VOIP_SURVEY_JSON)) == NULL)
        {
                printf("Kafka CreateTopicHandle %s failed.", TOPIC_VOIP_SURVEY_JSON);
                MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME,
                                                                "{%s:%d} Kafka CreateTopicHandle %s failed.",
                                                                __FILE__,__LINE__, TOPIC_VOIP_SURVEY_JSON);
        }
	return 0;
}

int voip_read_conf_and_init(const char* filename)
{	
	int 						log_level = 0;
	char 						conf_buf[MAX_PATH_LEN]={0};	
	
    MESA_load_profile_short_def(filename, "VOIP", "VOIPFilterSwitch", (short*)&g_frag_cfg.voip_filter_switch,0);
	/*voip log*/	
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_short_def(filename, "VOIP", "VOIPLogLevel", (short*)&log_level,30);
	MESA_load_profile_string_def(filename, "VOIP", "VOIPLogPath", conf_buf , sizeof(conf_buf),"./log/voip.log");
	g_frag_run.voip_logger = MESA_create_runtime_log_handle(conf_buf,log_level);
	if(NULL==g_frag_run.voip_logger)
	{
		printf("[%s] voip_logger MESA_create_runtime_log_handle error.\n", FRAG_REASSEMBLY_MODULE_NAME);
		MESA_handle_runtime_log(g_frag_run.logger,RLOG_LV_FATAL,FRAG_REASSEMBLY_MODULE_NAME,
								(char*)"[%s:%d] voip_logger MESA_create_runtime_log_handle error." , __FILE__,__LINE__);
		return -1;
	}
	
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_short_def(filename, "VOIP", "VOIPSurveyLogPort", (short*)&g_frag_cfg.voip_survey_log_port,0);		
	MESA_load_profile_short_def(filename, "VOIP", "VOIPSurveyLogIPNum", (short*)&g_frag_cfg.voip_survey_log_ipnum,0);		
	g_frag_cfg.voip_survey_log_iplist= (uint32*)malloc(g_frag_cfg.voip_survey_log_ipnum*sizeof(unsigned int));
	MESA_load_profile_string_nodef(filename, "VOIP", "VOIPSurveyLogIP", conf_buf, sizeof(conf_buf));
	MESA_split_read_IP(conf_buf,  g_frag_cfg.voip_survey_log_ipnum, g_frag_cfg.voip_survey_log_iplist);
		
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_short_def(filename, "VOIP", "VOIPFullLogPort", (short*)&g_frag_cfg.voip_full_log_port,0);		
	MESA_load_profile_short_def(filename, "VOIP", "VOIPFullLogIPNum", (short*)&g_frag_cfg.voip_full_log_ipnum,0);		
	g_frag_cfg.voip_full_log_iplist= (uint32*)malloc(g_frag_cfg.voip_full_log_ipnum*sizeof(unsigned int));
	MESA_load_profile_string_nodef(filename, "VOIP", "VOIPSurveyLogIP", conf_buf, sizeof(conf_buf));
	MESA_split_read_IP(conf_buf,  g_frag_cfg.voip_full_log_ipnum, g_frag_cfg.voip_full_log_iplist);
	return 0;
}

int app_read_conf_and_init(const char* filename)
{
    MESA_load_profile_short_def(filename, "SYSTEM", "AppSwitch", (short*)&g_frag_cfg.app_switch,0);
	if(g_frag_cfg.app_switch)
	{
#if APP_FUNC
		g_frag_run.appdtc_handle = APPDETECT_PLUG_INIT(g_frag_cfg.thread_num, "./conf/app_detect.conf");
		if(NULL==g_frag_run.appdtc_handle) g_frag_cfg.app_switch = 0;
#endif
	}
	return 0;
}

void rssb_stats_init(const char* filename)
{
	int 	value = 0;		
	char	conf_buf[MAX_PATH_LEN]={0};		

	memset(conf_buf,0,sizeof(conf_buf));
	g_frag_stat.fs_handle = FS_create_handle();	
	MESA_load_profile_string_def(filename, "LOG", "StatFile", conf_buf, sizeof(conf_buf), "./log/rssb_stat.log");
	FS_set_para(g_frag_stat.fs_handle, OUTPUT_DEVICE, conf_buf, strlen(conf_buf)+1);
	value = 1;//flush by date
	FS_set_para(g_frag_stat.fs_handle, FLUSH_BY_DATE, &value, sizeof(value));	
	value = 2;//append
	FS_set_para(g_frag_stat.fs_handle, PRINT_MODE, &value, sizeof(value));	
	MESA_load_profile_short_def(filename, "LOG", "StatCycle", (short*)&g_frag_stat.stat_interval,0);
	FS_set_para(g_frag_stat.fs_handle, STAT_CYCLE, &g_frag_stat.stat_interval, sizeof(g_frag_stat.stat_interval));	
	value = (g_frag_stat.stat_interval!=0) ? 1 : 0;
	FS_set_para(g_frag_stat.fs_handle, PRINT_TRIGGER, &value, sizeof(value));
	value = 0;
	FS_set_para(g_frag_stat.fs_handle, CREATE_THREAD, &value, sizeof(value));		
	MESA_load_profile_string_def(filename, "LOG", "AppName", g_frag_stat.fs_app, sizeof(g_frag_stat.fs_app), "rssb");
	FS_set_para(g_frag_stat.fs_handle, APP_NAME, g_frag_stat.fs_app, strlen(g_frag_stat.fs_app)+1);		
	MESA_load_profile_short_def(filename, "LOG", "StatRemoteSwitch", (short*)&g_frag_stat.fs_remote_switch,0);
	if(g_frag_stat.fs_remote_switch)
	{
		MESA_load_profile_string_def(filename, "LOG", "StatServerIP", g_frag_stat.fs_ip, sizeof(g_frag_stat.fs_ip), "127.0.0.1");
		FS_set_para(g_frag_stat.fs_handle, STATS_SERVER_IP, g_frag_stat.fs_ip, strlen(g_frag_stat.fs_ip)+1);	
		MESA_load_profile_short_def(filename, "LOG", "StatServerPort", (short*)&g_frag_stat.fs_port,0);
		FS_set_para(g_frag_stat.fs_handle, STATS_SERVER_PORT, &g_frag_stat.fs_port, sizeof(g_frag_stat.fs_port));
	}

	memset(conf_buf,0,sizeof(conf_buf));
	g_frag_stat.sysfs_handle = FS_create_handle();	
	MESA_load_profile_string_def(filename, "LOG", "SysinfoFile", conf_buf, sizeof(conf_buf), "./log/rssb_sysinfo.log");
	FS_set_para(g_frag_stat.sysfs_handle, OUTPUT_DEVICE, conf_buf, strlen(conf_buf)+1);
	value = 1;//flush by date
	FS_set_para(g_frag_stat.sysfs_handle, FLUSH_BY_DATE, &value, sizeof(value));	
	value = 2;//append
	FS_set_para(g_frag_stat.sysfs_handle, PRINT_MODE, &value, sizeof(value));	
	MESA_load_profile_short_def(filename, "LOG", "SysinfoCycle", (short*)&g_frag_stat.sysinfo_interval,0);
	FS_set_para(g_frag_stat.sysfs_handle, STAT_CYCLE, &g_frag_stat.sysinfo_interval, sizeof(g_frag_stat.sysinfo_interval));	
	value = (g_frag_stat.sysinfo_interval!=0) ? 1 : 0;
	FS_set_para(g_frag_stat.sysfs_handle, PRINT_TRIGGER, &value, sizeof(value));
	value = 0;
	FS_set_para(g_frag_stat.sysfs_handle, CREATE_THREAD, &value, sizeof(value));
}

int read_conf_and_init(const char* filename)
{
	int 						log_level = 0;
	uint32_t					i = 0;
	uint32_t					front_bizman_acc_msec = 0;
	uint32_t 					front_bizman_acc_num = 0;
	uint32_t					front_bizman_smooth_msec = 0;
	char 						conf_buf[MAX_PATH_LEN]={0};	
	char 						buf[MAX_PATH_LEN]={0};	
	char 						split_buf[DEST_MAXNUM][MAX_PATH_LEN];
	uint32_t					special_media_type_num;
	uint32_t					special_media_type[SPECIAL_MEDIA_TYPE_MAXNUM] = {0};	
	MESA_htable_create_args_t 	hash_args;
	uint32_t					hash_thread_safe = 512;
	uint32_t					hash_size = 0;
	uint32_t					hash_max_elem_num = 0;
	uint32_t					hash_expire_time = 0;	
	char						table_info_filename [MAX_PATH_LEN]={0};	
	
	/*main log*/
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_short_def(filename, "LOG", "LogLevel", (short*)&log_level,30);
	MESA_load_profile_string_def(filename, "LOG", "LogPath", conf_buf , sizeof(conf_buf),"./log/runtime.log");
	g_frag_run.logger = MESA_create_runtime_log_handle(conf_buf,log_level);
	if(NULL==g_frag_run.logger)
	{
		printf("[%s] MESA_create_runtime_log_handle error.\n", FRAG_REASSEMBLY_MODULE_NAME);
		return -1;
	}
	
	/*frag log*/	
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_short_def(filename, "LOG", "FragLogLevel", (short*)&g_frag_run.frag_loglevel,30);
	MESA_load_profile_string_def(filename, "LOG", "FragLogPath", conf_buf , sizeof(conf_buf),"./log/frag.log");
	g_frag_run.frag_logger = MESA_create_runtime_log_handle(conf_buf,g_frag_run.frag_loglevel);
	if(NULL==g_frag_run.frag_logger)
	{
		printf("[%s] frag_logger MESA_create_runtime_log_handle error.\n", FRAG_REASSEMBLY_MODULE_NAME);
		MESA_handle_runtime_log(g_frag_run.logger,RLOG_LV_FATAL,FRAG_REASSEMBLY_MODULE_NAME,
								(char*)"[%s:%d] frag_logger MESA_create_runtime_log_handle error." , __FILE__,__LINE__);
		return -1;
	}	

	/*resp log*/	
	MESA_load_profile_string_def(filename, "LOG", "RespLogPath", g_frag_cfg.resp_filename, sizeof(g_frag_cfg.resp_filename),"./log/survey.log");

	/*create media log*/	
	MESA_load_profile_string_def(filename, "LOG", "MediaCreateLogPath", g_frag_cfg.media_create_filename , sizeof(g_frag_cfg.media_create_filename),"./log/media_create.log");
	
	/*expire media log*/	
	MESA_load_profile_string_def(filename, "LOG", "MediaExpireLogPath", g_frag_cfg.media_expire_filename , sizeof(g_frag_cfg.media_expire_filename),"./log/media_expire.log");
	
	/*stat log*/
	rssb_stats_init(filename);

	/*类型 0:AV_PIC 1:voip*/	
	MESA_load_profile_short_def(filename, "SYSTEM", "CPZTpye", (short*)&g_frag_cfg.cpz_type, 0);	
	
	/*thread_num*/
	MESA_load_profile_uint_def(filename, "SYSTEM", "ThreadNum", &g_frag_cfg.thread_num, 1);	
	if(MAX_THREAD_NUM<g_frag_cfg.thread_num)
	{
		printf("[%s] thread_num is more than MAXT_THREAD_NUM:%d.\n", FRAG_REASSEMBLY_MODULE_NAME, MAX_THREAD_NUM);
		MESA_handle_runtime_log(g_frag_run.logger,RLOG_LV_FATAL,FRAG_REASSEMBLY_MODULE_NAME,
								(char*)"[%s:%d] thread_num is more than MAXT_THREAD_NUM:%s." , __FILE__,__LINE__,MAX_THREAD_NUM);

	}	
	MESA_load_profile_short_def(filename, "SYSTEM", "BizmanQueueMode", (short*)&g_frag_cfg.bizman_queue_mode, 0);	
	MESA_load_profile_uint_def(filename, "SYSTEM", "BizmanQueueMaxnum", &g_frag_cfg.bizman_queue_maxnum, 2000000);	
	MESA_load_profile_short_def(filename, "SYSTEM", "AckSwitch", (short*)&g_frag_cfg.ack_switch, 1);	
	MESA_load_profile_short_def(filename, "SYSTEM", "IVISwitch", (short*)&g_frag_cfg.IVI_switch, 1);	
	MESA_load_profile_short_def(filename, "SYSTEM", "AsmisSwitch", (short*)&g_frag_cfg.asmis_switch, 1);	

	MESA_load_profile_short_def(filename, "SYSTEM", "IndexQueryTime", (short*)&g_frag_cfg.index_query_timeout, 10);
	MESA_load_profile_short_def(filename, "SYSTEM", "IndexQueryTimerCbMaxNum", (short*)&g_frag_cfg.index_query_timer_cb_maxtime, 500);
	for(uint32_t i=0;i<g_frag_cfg.thread_num;i++)
	{
		g_frag_run.index_query_timer[i] = MESA_timer_create(0, TM_TYPE_QUEUE);	
	}
	
	/*media renew time*/	
	MESA_load_profile_short_def(filename, "SYSTEM", "RenewTimeMax", (short*)&g_frag_cfg.renew_time_max,0);	
	MESA_load_profile_short_def(filename, "SYSTEM", "RenewTimeMin", (short*)&g_frag_cfg.renew_time_min,0);	
	MESA_load_profile_short_def(filename, "SYSTEM", "RenewTimeStep", (short*)&g_frag_cfg.renew_time_step,0);	
		
	/*av record*/
	MESA_load_profile_short_def(filename, "SYSTEM", "AVRecordFileSwitch", (short*)&g_frag_cfg.avrecord_switch,1);	
	MESA_load_profile_string_def(filename, "SYSTEM", "AVRecordFileRootDir", g_frag_cfg.avrecord_filepath, sizeof(g_frag_cfg.avrecord_filepath),"./AVrecord/");	
	MESA_load_profile_uint_def(filename, "SYSTEM", "AVRecordFileMaxNum", &g_frag_cfg.avrecord_maxnum,100000);	
	mkdir_r(g_frag_cfg.avrecord_filepath);

	/*special media to windows*/	
	memset(conf_buf,0,sizeof(conf_buf));
	memset(split_buf,0,sizeof(split_buf));
	MESA_load_profile_short_def(filename, "SYSTEM", "ForwardSpecialMediaSwitch", (short*)&g_frag_cfg.special_media_fwd_switch,0);	
	MESA_load_profile_string_def(filename, "SYSTEM", "SpecialMediaType", conf_buf, sizeof(conf_buf),"");	
	special_media_type_num = string_split(conf_buf, split_buf, DEST_MAXNUM, ',');	
	for(i=0;i<special_media_type_num;i++)
	{
		sscanf(split_buf[i], "0x%02x", &special_media_type[i]);		
		g_frag_run.special_media_table[special_media_type[i]] = 1;
	}

	/*模糊hash摘要计算开关*/
	MESA_load_profile_short_def(filename, "SYSTEM", "FuzzyDigestSwitch", (short*)&g_frag_cfg.fuzzy_digest_switch,0);		

	/*修改元信息capIP*/
	MESA_load_profile_short_def(filename, "SYSTEM", "ModifyCapIPSwitch", (short*)&g_frag_cfg.modify_capIP_switch,1);		

	/*media hash param*/	
	MESA_load_profile_uint_def(filename, "SYSTEM", "MediaHashThreadSafe", &hash_thread_safe, 512);
	MESA_load_profile_uint_def(filename, "SYSTEM", "MediaHashSize", &hash_size, 1024*1024);
	MESA_load_profile_uint_def(filename, "SYSTEM", "MediaHashElemNum", &hash_max_elem_num, 1024*1024*16);
	MESA_load_profile_uint_def(filename, "SYSTEM", "MediaHashExpireTime", &hash_expire_time, 120);
	memset(&hash_args,0,sizeof(MESA_htable_create_args_t));
	hash_args.thread_safe = hash_thread_safe; //group lock
	hash_args.recursive = 1;
	hash_args.hash_slot_size = hash_size;
	hash_args.max_elem_num = hash_max_elem_num;
	hash_args.eliminate_type = HASH_ELIMINATE_ALGO_LRU;
	hash_args.expire_time = hash_expire_time;
	hash_args.key_comp = NULL;
	hash_args.key2index = NULL;
	hash_args.data_free = free_media; 
	hash_args.data_expire_with_condition = expire_media_hash_node;	 
	g_frag_run.media_hash = MESA_htable_create(&hash_args, sizeof(hash_args));	
	if(NULL==g_frag_run.media_hash)
	{		
		printf("create media_hash error.\n");
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} create media_hash error", __FILE__,__LINE__);
		return -1;
	}

	/*monitor hash param*/	
	MESA_load_profile_uint_def(filename, "SYSTEM", "MonitorHashSize", &hash_size, 1024*1024);
	MESA_load_profile_uint_def(filename, "SYSTEM", "MonitorHashElemNum", &hash_max_elem_num, 1024*1024*16);
	MESA_load_profile_uint_def(filename, "SYSTEM", "MonitorHahsExpireTime", &hash_expire_time, 60*60*12);
	memset(&hash_args,0,sizeof(MESA_htable_create_args_t));
	hash_args.thread_safe = 1;  //group lock
	hash_args.recursive = 1;
	hash_args.hash_slot_size = hash_size;
	hash_args.max_elem_num = hash_max_elem_num;
	hash_args.eliminate_type = HASH_ELIMINATE_ALGO_LRU;
	hash_args.expire_time = hash_expire_time;
	hash_args.key_comp = NULL;
	hash_args.key2index = NULL;
	hash_args.data_free = free_monitor_hash_node; 
	hash_args.data_expire_with_condition = expire_monitor_hash_node;   
	g_frag_run.media_monitor_hash = MESA_htable_create(&hash_args, sizeof(hash_args));	
	if(NULL==g_frag_run.media_monitor_hash)
	{		
		printf("create media_monitor_hash error.\n");
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} create media_monitor_hash error.", __FILE__,__LINE__);
		return -1;
	}
	
	/*dumpfile file*/	
	MESA_load_profile_uint_def(filename, "SYSTEM", "DumpfileHashSize", &hash_size, 1024*1024);
	MESA_load_profile_uint_def(filename, "SYSTEM", "DumpfileHashElemNum", &hash_max_elem_num, 1024*1024*16);
	MESA_load_profile_uint_def(filename, "SYSTEM", "DumpfileHahsExpireTime", &hash_expire_time, 120);
	memset(&hash_args,0,sizeof(MESA_htable_create_args_t));
	hash_args.thread_safe = hash_thread_safe;  //group lock
	hash_args.recursive = 1;
	hash_args.hash_slot_size = hash_size;
	hash_args.max_elem_num = hash_max_elem_num;
	hash_args.eliminate_type = HASH_ELIMINATE_ALGO_LRU;
	hash_args.expire_time = hash_expire_time;
	hash_args.key_comp = NULL;
	hash_args.key2index = NULL;
	hash_args.data_free = free_dumpfile_hash_node; 
	hash_args.data_expire_with_condition = expire_dumpfile_hash_node;   
	g_frag_run.dumpfile_hash = MESA_htable_create(&hash_args, sizeof(hash_args));	
	if(NULL==g_frag_run.dumpfile_hash)
	{		
		printf("create dumpfile_hash error.\n");
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} create dumpfile_hash error.", __FILE__,__LINE__);
		return -1;
	}	
	MESA_load_profile_string_def(filename, "SYSTEM", "MonitorFileRootDir", g_frag_cfg.monitor_file_root_dir, sizeof(g_frag_cfg.monitor_file_root_dir),"/home/yspdata/qd_monitor/");
	MESA_load_profile_short_def(filename, "SYSTEM", "MonitorFileSwitch", (short*)&g_frag_cfg.monitor_file_switch,0);	
	MESA_load_profile_short_def(filename, "SYSTEM", "MonitorFileDay", (short*)&g_frag_cfg.monitor_file_days,0);	
	mkdir_r(g_frag_cfg.monitor_file_root_dir);	

	/*maat初始化*/		
	MESA_load_profile_short_def(filename, "SYSTEM", "FragForecastSwitch", (short*)&g_frag_cfg.forecast_switch,0);	
	if(g_frag_cfg.forecast_switch)
	{
		MESA_load_profile_string_def(filename,"SYSTEM","MaatTableInfo",table_info_filename,sizeof(table_info_filename),"./conf/table_info.conf");	
		MESA_load_profile_string_def(filename,"SYSTEM","FragMonitorJSON",conf_buf,sizeof(conf_buf),"./conf/frag_monitor.json");	
		g_frag_run.feather = Maat_feather(g_frag_cfg.thread_num, table_info_filename, g_frag_run.logger);
		Maat_set_feather_opt(g_frag_run.feather, MAAT_OPT_JSON_FILE_PATH, conf_buf, strlen(conf_buf)+1);
		MESA_load_profile_string_def(filename,"SYSTEM","MaatStatFile",conf_buf,sizeof(conf_buf),"./log/maat_stat.log");	
		Maat_set_feather_opt(g_frag_run.feather, MAAT_OPT_STAT_FILE_PATH, conf_buf, strlen(conf_buf)+1);
		int temp = 0;	
		MESA_load_profile_int_def(filename, "SYSTEM", "MaatStatSwitch", &temp,  0);
		if(temp)
		{
			Maat_set_feather_opt(g_frag_run.feather, MAAT_OPT_STAT_ON, NULL, 0);
		}
		MESA_load_profile_int_def(filename, "SYSTEM", "MaatPerfSwitch", &temp,  0);	
		if(temp)
		{
			Maat_set_feather_opt(g_frag_run.feather, MAAT_OPT_PERF_ON, NULL, 0);
		}
		int scan_detail = 0;
		Maat_set_feather_opt(g_frag_run.feather, MAAT_OPT_SCAN_DETAIL, &scan_detail, sizeof(scan_detail));
		Maat_initiate_feather(g_frag_run.feather);
		if(NULL==g_frag_run.feather)
		{
			printf("Maat_summon_feather_json error.\n");		
		}
		g_frag_run.expr_tableid = Maat_table_register(g_frag_run.feather,"FRAG_MONITOR_KEYWORDS");
	}
	
	/*管理口*/
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_string_nodef(filename, "NETWORK", "LocalIP", conf_buf, sizeof(conf_buf)); 	
	g_frag_cfg.local_ip_nr = get_ip_by_ifname(conf_buf);

	/*负载口*/
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_string_nodef(filename, "NETWORK", "FwdIP", conf_buf, sizeof(conf_buf)); 	
	g_frag_cfg.fwd_ip_nr = get_ip_by_ifname(conf_buf);

	/*recv_bizman init:粗拼装接收数据*/	
	MESA_load_profile_short_def(filename, "NETWORK", "BizmanPort", (short*)&g_frag_cfg.bizman_port,22082);	
	MESA_load_profile_uint_def(filename, "NETWORK", "BizmanAckSmoothTime", &front_bizman_smooth_msec,10);	
	MESA_load_profile_uint_def(filename, "NETWORK", "BizmanAckAccumulateTime", &front_bizman_acc_msec,10);	
	MESA_load_profile_uint_def(filename, "NETWORK", "BizmanAckAccumulateNum", &front_bizman_acc_num,5);	
	for(uint32_t i =0;i<g_frag_cfg.thread_num;i++)
	{
		g_frag_run.recv_bizman[i] = bizman_get_handle();		
		bizman_set_handle(g_frag_run.recv_bizman[i], BIZMAN_RECEIVER_ACTIVE|BIZMAN_CHOKE_RECEIVE | BIZMAN_CHUNK_RECEIVE);
		bizman_set_handle_parameter(g_frag_run.recv_bizman[i],BIZMAN_PARAMETER_ACK_ACCUMULATE_MSEC,front_bizman_acc_msec);
		bizman_set_handle_parameter(g_frag_run.recv_bizman[i], BIZMAN_PARAMETER_ACK_ACCUMULATE_NUM,front_bizman_acc_num);
		bizman_set_handle_parameter(g_frag_run.recv_bizman[i],BIZMAN_PARAMETER_ACK_SMOOTH_TIME_MSEC,front_bizman_smooth_msec);
		bizman_listen(g_frag_run.recv_bizman[i], g_frag_cfg.bizman_port+i);
		bizman_init_handle(g_frag_run.recv_bizman[i]); 
	}	

	/*负载均衡*/
	MESA_load_profile_short_def(filename, "WLB", "wlb_on", (short*)&g_frag_cfg.wlb_on,1);
	MESA_load_profile_string_def(filename, "WLB", "wlb_topic", g_frag_cfg.wlb_topic, sizeof(g_frag_cfg.wlb_topic),"AV");
	MESA_load_profile_string_def(filename, "WLB", "wlb_group_name", g_frag_cfg.wlb_group_name, sizeof(g_frag_cfg.wlb_group_name),"group1");
	MESA_load_profile_string_def(filename, "WLB", "user_tag", g_frag_cfg.user_tag, sizeof(g_frag_cfg.user_tag),"1");
	MESA_load_profile_uint_def(filename, "WLB", "health_check_port", (unsigned int*)&g_frag_cfg.health_check_port,52100);
	MESA_load_profile_short_def(filename, "WLB", "health_check_interval", (short*)&g_frag_cfg.health_check_interval,10);		
	MESA_load_profile_uint_def(filename, "WLB", "capacity", (unsigned int*)&g_frag_cfg.capacity,32);		
	MESA_load_profile_uint_def(filename, "WLB", "cost", (unsigned int*)&g_frag_cfg.cost,32);		
	MESA_load_profile_uint_def(filename, "WLB", "wlb_report_interval", (unsigned int*)&g_frag_cfg.wlb_report_interval,10);		
	MESA_load_profile_uint_def(filename, "WLB", "enable_override", (unsigned int*)&g_frag_cfg.enable_override,0);	
	MESA_load_profile_uint_def(filename, "WLB", "bfd_recv_port", (unsigned int*)&g_frag_cfg.bfd_recv_port,0);	

	/*send bizman :粗拼装往前端发数据，监听响应端口*/	
	MESA_load_profile_short_def(filename, "NETWORK", "BizmanAckPort", (short*)&g_frag_cfg.bizman_ack_port,22084);		
	g_frag_run.answer_sapp_bizman = bizman_get_handle();
	bizman_set_handle(g_frag_run.answer_sapp_bizman, BIZMAN_SENDER_ACTIVE);
	bizman_listen(g_frag_run.answer_sapp_bizman, g_frag_cfg.bizman_ack_port);
	bizman_init_handle(g_frag_run.answer_sapp_bizman);

	/*unix socket : backward data send*/	
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_string_def(filename, "NETWORK", "UnixSocketSendSrcAddr", conf_buf, sizeof(conf_buf),"/home/mesasoft/frag_rssb/un_sender");
	for(i=0;i<g_frag_cfg.thread_num;i++)
	{
		memset(buf,0,sizeof(buf));
		snprintf(buf,sizeof(buf),"%s_%02d",conf_buf,i);
		g_frag_run.send_fd[i] = init_unix_socket(buf);
		if(-1==g_frag_run.send_fd[i])
		{
			printf("[%s] init_unix_socket error.\n", FRAG_REASSEMBLY_MODULE_NAME);
			MESA_handle_runtime_log(g_frag_run.logger,RLOG_LV_FATAL,FRAG_REASSEMBLY_MODULE_NAME,
								(char*)"[%s:%d] init_unix_socket error." , __FILE__,__LINE__);
			return -1;
		}
	}
	memset(conf_buf,0,sizeof(conf_buf));
	memset(split_buf,0,sizeof(split_buf));
	MESA_load_profile_string_def(filename, "NETWORK", "UnixSocketSendDestAddr", conf_buf, sizeof(conf_buf),"");
	g_frag_cfg.send_dest_addr_num = string_split(conf_buf, split_buf, DEST_MAXNUM, ';');	
	for(i=0;i<(uint32_t)g_frag_cfg.send_dest_addr_num;i++)
	{
		strcpy(g_frag_cfg.send_dest_addr[i].sun_path,split_buf[i]);
		g_frag_cfg.send_dest_addr[i].sun_family = AF_UNIX;
	}
	
	/*----USM send read conf and init----*/
	MESA_load_profile_int_def(filename, "NETWORK", "USM_ON_FLAG", &g_frag_run.usm_on_flag, 0);	
	int shm_key = 0;
	unsigned long long shm_size = 0;
	unsigned int smooth_time = 0;
	unsigned int max_qsize = 0;
	char usm_reader_path[MAX_PATH_LEN]={0};
	char usm_r_path_split[DEST_MAXNUM][MAX_PATH_LEN];
	int usm_log_level = 0;
	char usm_log_path[MAX_PATH_LEN]={0};
	char usm_log_path_split[DEST_MAXNUM][MAX_PATH_LEN];
	memset(usm_r_path_split,0,sizeof(usm_r_path_split));
	memset(usm_log_path_split,0,sizeof(usm_log_path_split));
	void* logger[DEST_MAXNUM]={0};
	MESA_load_profile_int_def(filename, "NETWORK", "USM_SHM_KEY", &shm_key, 12345);
	MESA_load_profile_int_def(filename, "NETWORK", "USM_SHM_SIZE", (int*)&shm_size, 100);
	MESA_load_profile_int_def(filename, "NETWORK", "USM_SMOOTH_TIME", (int*)&smooth_time,2000000);
	MESA_load_profile_int_def(filename, "NETWORK", "USM_Q_SIZE", (int*)&max_qsize,4*1024*1024);
	MESA_load_profile_string_def(filename, "NETWORK", "UnixSocketSendDestAddr", usm_reader_path ,sizeof(usm_reader_path),"");
	string_split(usm_reader_path, usm_r_path_split, DEST_MAXNUM, ';');
	MESA_load_profile_string_def(filename, "NETWORK", "USM_LOG_PATH", usm_log_path ,sizeof(usm_log_path),"./log/usm_writer0");
	string_split(usm_log_path, usm_log_path_split, DEST_MAXNUM, ';');
	MESA_load_profile_int_def(filename, "NETWORK", "USM_LOG_LEVEL", &usm_log_level, 30);
	if(g_frag_run.usm_on_flag)
	{
		g_frag_run.a_usm_handle = USM_handle(shm_key,shm_size,USM_WRITER);
		g_frag_run.reader_cnt = g_frag_cfg.send_dest_addr_num;
		USM_set_opt(g_frag_run.a_usm_handle,READER_CNT,(void*)&g_frag_run.reader_cnt,sizeof(unsigned int),0);
		for(i=0;i<g_frag_run.reader_cnt;i++)
		{
			USM_set_opt(g_frag_run.a_usm_handle,READER_PATH,(void*)usm_r_path_split[i],MAX_PATH_LEN,i);
			USM_set_opt(g_frag_run.a_usm_handle,SMOOTH_TIME,(void*)&smooth_time,sizeof(unsigned int),i);
			USM_set_opt(g_frag_run.a_usm_handle,MAX_LQUEUE_SIZE,(void*)&max_qsize, sizeof(unsigned int), i);
			logger[i] = MESA_create_runtime_log_handle(usm_log_path_split[i],usm_log_level);
			USM_set_opt(g_frag_run.a_usm_handle,LOG_HANDLE,logger[i],sizeof(void*),i);
		}
		
		if(USM_init(g_frag_run.a_usm_handle) < 0)
		{
			return -1;
		}
	}
	
	/*unix socket : response msg recv*/		
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_string_def(filename, "NETWORK", "UnixSocketRecvAddr", conf_buf, sizeof(conf_buf),"/home/mesasoft/frag_rssb/un_recv");
	g_frag_run.recv_msg_fd = init_recv_unix_socket(conf_buf);
	if(-1==g_frag_run.recv_msg_fd)
	{
		printf("[%s] init_recv_unix_socket error.\n", FRAG_REASSEMBLY_MODULE_NAME);
		MESA_handle_runtime_log(g_frag_run.logger,RLOG_LV_FATAL,FRAG_REASSEMBLY_MODULE_NAME,
								(char*)"[%s:%d] init_recv_unix_socket." , __FILE__,__LINE__);
		return -1;
	}

	/*msg port:粗拼装接收建议的端口，前端接收建议的端口*/
	MESA_load_profile_short_def(filename, "NETWORK", "MsgPort", (short*)&g_frag_cfg.msg_port, 22080);

	/*udp socket : response msg recv*/
	g_frag_run.recv_msg_sd = init_recv_udp_socket(htons(g_frag_cfg.msg_port));
	if(-1==g_frag_run.recv_msg_sd)
	{
		printf("[%s] init_recv_udp_socket error.\n", FRAG_REASSEMBLY_MODULE_NAME);
		MESA_handle_runtime_log(g_frag_run.logger,RLOG_LV_FATAL,FRAG_REASSEMBLY_MODULE_NAME,
								(char*)"[%s:%d] init_recv_udp_socket." , __FILE__,__LINE__);
		return -1;
	}
	
	/*white list*/		
	uint16_t	whitelist_port = 0;
	uint32_t*	whitelist_ip_serial = NULL;
	MESA_load_profile_short_def(filename, "NETWORK", "WhiteListPort", (short*)&whitelist_port, 0);	
	MESA_load_profile_uint_def(filename, "NETWORK", "WhiteListIPNum", (uint32_t*)&g_frag_cfg.whitelist_addr_num, 0);		
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_string_def(filename, "NETWORK", "WhiteListIP", conf_buf, sizeof(conf_buf),"");
	if(g_frag_cfg.whitelist_addr_num>0)
	{
		whitelist_ip_serial = (unsigned int *)malloc(g_frag_cfg.whitelist_addr_num*sizeof(unsigned int));
		MESA_split_read_IP(conf_buf, g_frag_cfg.whitelist_addr_num, whitelist_ip_serial);
		for(i=0;i<(uint32_t)g_frag_cfg.whitelist_addr_num;i++)
		{
			g_frag_cfg.whitelist_addr[i].sin_addr.s_addr = whitelist_ip_serial[i];
			g_frag_cfg.whitelist_addr[i].sin_port = htons(whitelist_port);
		}
		if(NULL!=whitelist_ip_serial)
		{
			free(whitelist_ip_serial);
		}
	}	

	/*udp socket : send av data by udp socket*/	
	uint32_t*	send_udp_ip_serial = NULL;
	MESA_load_profile_uint_def(filename, "NETWORK", "UdpSendIPNum", (uint32_t*)&g_frag_cfg.send_dest_udp_ip_num,0);		
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_string_nodef(filename, "NETWORK", "UdpSendPort", conf_buf, sizeof(conf_buf));	
	memset(split_buf,0,sizeof(split_buf));
	string_split(conf_buf, split_buf, DEST_MAXNUM, ';');	
	for(i=0;i<(uint32_t)g_frag_cfg.send_dest_udp_ip_num;i++)
	{
		g_frag_cfg.send_dest_udp_port[i] = htons(atoi(split_buf[i]));
	}		
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_string_def(filename, "NETWORK", "UdpSendIP", conf_buf, sizeof(conf_buf),"");	
	if(g_frag_cfg.send_dest_udp_ip_num>0)
	{
		send_udp_ip_serial = (unsigned int *)malloc(g_frag_cfg.send_dest_udp_ip_num*sizeof(unsigned int));
		MESA_split_read_IP(conf_buf, g_frag_cfg.send_dest_udp_ip_num, send_udp_ip_serial);
		for(i=0;i<(uint32_t)g_frag_cfg.send_dest_udp_ip_num;i++)
		{
			g_frag_cfg.send_dest_udp_iplist[i] = send_udp_ip_serial[i];
		}
		if(NULL!=send_udp_ip_serial)
		{
			free(send_udp_ip_serial);
		}
	}
	
#if K_PROJECT
	if(g_frag_cfg.send_dest_udp_ip_num>0)
	{
		for(i=0; i<g_frag_cfg.thread_num; i++)
		{
			g_frag_run.send_sd[i] = init_send_udp_socket();
			if(-1==g_frag_run.send_sd[i])
			{
				printf("[%s] init_send_udp_socket error.\n", FRAG_REASSEMBLY_MODULE_NAME);
				MESA_handle_runtime_log(g_frag_run.logger,RLOG_LV_FATAL,FRAG_REASSEMBLY_MODULE_NAME,
										(char*)"[%s:%d] init_send_udp_socket." , __FILE__,__LINE__);
				return -1;
			}
		}
	}
#endif
		
	/*udp socket : data msg send to windows system*/	
	uint32_t*   wins_ip_serial = NULL;
	MESA_load_profile_uint_def(filename, "NETWORK", "SpecialMediaWindowsIPNum", (uint32_t*)&g_frag_cfg.special_media_wins_ip_num,0);		
	memset(conf_buf,0,sizeof(conf_buf));
	memset(split_buf,0,sizeof(split_buf));
	MESA_load_profile_string_nodef(filename, "NETWORK", "SpecialMediaWindowsPort", conf_buf, sizeof(conf_buf));
	g_frag_cfg.special_media_wins_port_num = string_split(conf_buf, split_buf, DEST_MAXNUM, ';');	
	for(i=0;i<(uint32_t)g_frag_cfg.special_media_wins_port_num;i++)
	{
		g_frag_cfg.special_media_wins_port[i] = htons(atoi(split_buf[i]));
	}		
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_string_def(filename, "NETWORK", "SpecialMediaWindowsIP", conf_buf, sizeof(conf_buf),"");	
	if(g_frag_cfg.special_media_wins_ip_num>0)
	{
		wins_ip_serial = (unsigned int *)malloc(g_frag_cfg.special_media_wins_ip_num*sizeof(unsigned int));
		MESA_split_read_IP(conf_buf, g_frag_cfg.special_media_wins_ip_num, wins_ip_serial);
		for(i=0;i<(uint32_t)g_frag_cfg.special_media_wins_ip_num;i++)
		{
			g_frag_cfg.special_media_wins_ip[i] = wins_ip_serial[i];
		}
		if(NULL!=wins_ip_serial)
		{
			free(wins_ip_serial);
		}
	}	
	if(g_frag_cfg.special_media_fwd_switch && g_frag_cfg.special_media_wins_ip_num>0 && g_frag_cfg.special_media_wins_port_num>0)
	{
		for(i=0; i<g_frag_cfg.thread_num; i++)
		{
			g_frag_run.send_windows_sd[i] = init_send_udp_socket();
			if(-1==g_frag_run.send_windows_sd[i])
			{
				printf("[%s] init_send_wins_udp_socket error.\n", FRAG_REASSEMBLY_MODULE_NAME);
				MESA_handle_runtime_log(g_frag_run.logger,RLOG_LV_FATAL,FRAG_REASSEMBLY_MODULE_NAME,
										(char*)"[%s:%d] init_send_wins_udp_socket." , __FILE__,__LINE__);
				return -1;
			}
		}
	}	

	MESA_load_profile_short_def(filename, "DEBUG", "save_media", &g_frag_cfg.save_media, 0);			
	MESA_load_profile_string_def(filename, "DEBUG", "save_media_path", g_frag_cfg.save_media_path, sizeof(g_frag_cfg.save_media_path),"./log/save_media/");

	/*store file*/
	MESA_load_profile_short_def(filename, "DEBUG", "FileStoreSwitch", (short*)&g_frag_cfg.store_filepath_switch,0); 
	MESA_load_profile_string_def(filename, "DEBUG", "FileStorePath", g_frag_cfg.store_filepath, sizeof(g_frag_cfg.store_filepath),"./log/file/");
	/*debug:AllHitMonitorSwitch*/
	MESA_load_profile_short_def(filename, "DEBUG", "AllHitMonitorSwitch", (short*)&g_frag_cfg.all_hit_monitor_switch,0);	
	MESA_load_profile_short_def(filename, "DEBUG", "AllHitMonitorCompleteRate", (short*)&g_frag_cfg.all_hit_monitor_complete_rate,0);	
	MESA_load_profile_short_def(filename, "DEBUG", "AllHitMonitorFilenameType", (short*)&g_frag_cfg.all_hit_filename,0);	

	MESA_load_profile_short_def(filename, "DEBUG", "HlsAboffsetInMode", (short*)&g_frag_cfg.hls_in_aboffset_mode,1);		

	MESA_load_profile_short_def(filename, "DEBUG", "JSONLocalSwitch", (short*)&g_frag_cfg.json_local_switch,0);		
	/*碎片建议失效开关*/
	MESA_load_profile_short_def(filename, "DEBUG", "FragSurveyInvalid", (short*)&g_frag_cfg.frag_survey_invalid,0);		
	/*trace path*/
	MESA_load_profile_string_def(filename, "DEBUG", "TracePath", g_frag_cfg.trace_filepath, sizeof(g_frag_cfg.trace_filepath),"./log/");

	/*JSON信息回传跨媒体业务*/
	if(-1==multimedia_read_conf_and_init(filename))
	{		
		return -1;
	}				
			
	/*音视频降载业务td_data*/	
	if(-1==dedup_read_conf_and_init(filename))
	{		
		return -1;
	}		

	/*VOIP 日志发送业务*/	
	if(-1==voip_read_conf_and_init(filename))
	{		
		return -1;
	}

	/*	APP发送业务*/	
#ifdef APP_FUNC
	if(-1==app_read_conf_and_init(filename))
	{		
		return -1;
	}	

#endif

	return 0;
}

void* rssb_asmis_task(void *param)
{
	static unsigned long long nValue_last = 0;

	while(1)
	{
		struct info_rtd_flow info;
		memcpy(info.sValType, "input", strlen("input"));	
		memcpy(info.sBDType, "av_data", strlen("av_data"));
		info.nRTTS = time(NULL);
		info.nDuration = g_frag_cfg.asmis_switch;
		info.nValue = g_frag_stat.stat_info[RECV][TOTAL_BYTES]-nValue_last;
		asmis_log_RtdFlow(g_frag_run.asmis_log_handle, time(NULL), g_frag_cfg.asmis_switch, &info, 1);
		nValue_last = g_frag_stat.stat_info[RECV][TOTAL_BYTES];		
		sleep(g_frag_cfg.asmis_switch*60);					
	}
	return NULL;
}

void rssb_asmis_init()
{
	g_frag_run.asmis_log_handle =  asmis_log_Init("GJYSP/rssb_maskey");
	asmis_log_AppVer(g_frag_run.asmis_log_handle, frag_rssb_version, frag_rssb_version_time, frag_rssb_version_des);
	/*22082 22080 */
	int nPort = 2;
	struct info_port_used info[2];
	info[0].nPort = g_frag_cfg.bizman_port;	
	info[0].nPortType = 1;
	info[0].nProtocolType = 6;
	memcpy(info[0].sPortDesc, "recv data", strlen("recv data"));
	info[0].sPortDesc[strlen("recv data")] = '\0';
	info[1].nPort = g_frag_cfg.msg_port;	
	info[1].nPortType = 1;
	info[1].nProtocolType = 6;
	memcpy(info[1].sPortDesc, "recv survey", strlen("recv survey"));
	info[1].sPortDesc[strlen("recv survey")] = '\0';
	asmis_log_PortUsed(g_frag_run.asmis_log_handle, info, nPort);
	asmis_log_RunStart(g_frag_run.asmis_log_handle, 0);
}

int   main(int argc, char **argv)
{	
	uint32_t 	i=0;	
	void		*main_conf_handle=NULL, *rssb_conf_handle=NULL;

	memset(&g_frag_run, 0, sizeof(frag_rssb_parameter_t));	
	memset(&g_frag_cfg, 0, sizeof(frag_rssb_configure_t));
	memset(&g_frag_stat, 0, sizeof(frag_rssb_status_t));

	pthread_mutex_init(&g_frag_run.media_create_file_lock, NULL);
	pthread_mutex_init(&g_frag_run.media_expire_file_lock, NULL);
	pthread_mutex_init(&g_frag_run.resp_file_lock, NULL);

#if VOIP_FUNC
	main_conf_handle=wired_cfg_create("VOIP_RSSB_MAIN_CONF", "./conf/main.conf");
	wired_cfg_init(main_conf_handle);	
	rssb_conf_handle=wired_cfg_create("VOIP_RSSB_CONF", "./conf/frag_reassembly.conf");
	wired_cfg_init(rssb_conf_handle);
#else
	main_conf_handle=wired_cfg_create("RSSB_MAIN_CONF", "./conf/main.conf");
	wired_cfg_init(main_conf_handle);	
	rssb_conf_handle=wired_cfg_create("RSSB_CONF", "./conf/frag_reassembly.conf");
	wired_cfg_init(rssb_conf_handle);
#endif
		
	/*read main.conf and init*/
	if(-1==read_conf_and_init("./conf/main.conf"))
	{		
		return -1;
	}
	
	//wire_ld		
	if(g_frag_cfg.wlb_on)
	{
		g_frag_cfg.rssb_wlb_handle = wiredLB_create(g_frag_cfg.wlb_topic,g_frag_cfg.wlb_group_name, WLB_CONSUMER);
		wiredLB_set_opt(g_frag_cfg.rssb_wlb_handle, WLB_OPT_HEALTH_CHECK_PORT, (const void*)&g_frag_cfg.health_check_port, sizeof(g_frag_cfg.health_check_port));
		wiredLB_set_opt(g_frag_cfg.rssb_wlb_handle, WLB_OPT_ENABLE_OVERRIDE, (const void*)&g_frag_cfg.enable_override, sizeof(g_frag_cfg.enable_override));
		//wiredLB_set_opt(g_frag_cfg.rssb_wlb_handle, WLB_OPT_HEALTH_CHECK_INTERVAL, (const void*)&g_frag_cfg.health_check_interval, sizeof(g_frag_cfg.health_check_interval));
		wiredLB_set_opt(g_frag_cfg.rssb_wlb_handle, WLB_CONS_OPT_DATA_PORT,(const void*)&g_frag_cfg.bizman_port,sizeof(g_frag_cfg.bizman_port));
		char		pbuf[32] = {0}; 
		int 		buf_len = 32;
		memset(pbuf, 0, sizeof(pbuf));
		inet_ntop(AF_INET, &g_frag_cfg.fwd_ip_nr, pbuf, buf_len); 
		wiredLB_set_opt(g_frag_cfg.rssb_wlb_handle, WLB_CONS_OPT_PRIMARY_ADDR,(const void*)pbuf,strlen(pbuf)+2);
		memset(pbuf, 0, sizeof(pbuf));
		inet_ntop(AF_INET, &g_frag_cfg.local_ip_nr, pbuf, buf_len); 	
		wiredLB_set_opt(g_frag_cfg.rssb_wlb_handle, WLB_CONS_OPT_SECONDARY_ADDR,(const void*)pbuf,strlen(pbuf)+2);
		wiredLB_set_opt(g_frag_cfg.rssb_wlb_handle, WLB_CONS_OPT_CAPACITY, (const void*)&g_frag_cfg.capacity, sizeof(g_frag_cfg.capacity));
		wiredLB_set_opt(g_frag_cfg.rssb_wlb_handle, WLB_CONS_OPT_COST,(const void*)&g_frag_cfg.cost, sizeof(g_frag_cfg.cost));
		wiredLB_set_opt(g_frag_cfg.rssb_wlb_handle, WLB_CONS_OPT_USER_TAG,(const void*)&g_frag_cfg.user_tag, sizeof(g_frag_cfg.user_tag));
		wiredLB_init(g_frag_cfg.rssb_wlb_handle);		
	}	
	
	/*frag_reassembly init*/
	if(-1==frag_reassembly_init("./conf/","./log/frag_rssb/",g_frag_cfg.thread_num))
	{
		printf("[%s] frag_reassembly_init error.\n", FRAG_REASSEMBLY_MODULE_NAME);
		MESA_handle_runtime_log(g_frag_run.logger,RLOG_LV_DEBUG,FRAG_REASSEMBLY_MODULE_NAME,
								(char*)"[%s:%d] frag_reassembly_init error." , __FILE__,__LINE__);
		return -1;
	}
	
	/*trace log*/	
	g_frag_run.mid_trace_hd = MESA_trace_create_numerical_handle(g_frag_run.logger, "./conf/mid_trace.conf");

	/* init ack buf */
	if(g_frag_cfg.ack_switch)
	{
		for(i=0; i<g_frag_cfg.thread_num;i++)
		{			
			msg_header_t *mh_ack = (msg_header_t*)g_frag_run.fb_ack_buf[i];
			g_frag_run.fb_ack_hdr[i] = (msg_data_ack_t*)( (char*)mh_ack + MSG_HEADER_LEN);
			mh_ack->magic_num	= PROTO_MAGICNUM;
			mh_ack->version		= PROTO_VERSION;
			mh_ack->msg_type	= MSG_RESP_CHARACTER;
			mh_ack->cont_len	= sizeof(msg_data_ack_t);			
		}
	}	

	/*frag forward thread*/		
    for(i=0;i<g_frag_cfg.thread_num;i++)
    {       
        if(-1 == create_pthread(frag_forward,(void*)i,g_frag_run.logger))
        {
			MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									(char*)"[%s:%d] Thread frag_forward Create Failed." , __FILE__,__LINE__);
			return -1;
        }
    }   

	/*app queue*/	
	if(g_frag_cfg.app_switch)
    {     		
		g_frag_run.app_lq =  MESA_lqueue_create(1, 0);			
        if(-1 == create_pthread(thread_send_app_data,(void*)i,g_frag_run.logger))
        {
			MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									(char*)"[%s:%d] Thread thread_send_app_data Create Failed." , __FILE__,__LINE__);
			return -1;
        }
    }    

	/*recv response msg from av_analyse*/	
	if(-1 == create_pthread(recv_response_msg,NULL,g_frag_run.logger))
    {
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								(char*)"[%s:%d] Thread proc_response_msg Create Failed." , __FILE__,__LINE__,FRAG_REASSEMBLY_MODULE_NAME);
		return -1;
    }             

	/*dumpfile : monitor_file_lq*/	
	if(g_frag_cfg.monitor_file_switch)
	{
		g_frag_run.monitor_file_lq = (MESA_lqueue_head*)calloc(1, g_frag_cfg.thread_num*sizeof(MESA_lqueue_head));
		for(i=0;i<g_frag_cfg.thread_num;i++)
    	{		
			g_frag_run.monitor_file_lq[i] =  MESA_lqueue_create(1, 0);			
			if(-1 == create_pthread(monitor_service_dump_file_thread,(void*)i,g_frag_run.logger))
			{
				MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
										(char*)"[%s:%d] Thread monitor_service_dump_file_thread Create Failed." , __FILE__,__LINE__,FRAG_REASSEMBLY_MODULE_NAME);
				return -1;
			}
    	}		
	}	
	
	/*av_record_lq and av_digest_record_lq*/	
	if(g_frag_cfg.avrecord_switch)
	{
		g_frag_run.av_record_lq= (MESA_lqueue_head*)calloc(1, g_frag_cfg.thread_num*sizeof(MESA_lqueue_head));
		for(i=0;i<g_frag_cfg.thread_num;i++)
    	{		
			g_frag_run.av_record_lq[i] =  MESA_lqueue_create(1, 0);		
			if(-1 == create_pthread(av_record_thread,(void*)i,g_frag_run.logger))
			{
				MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
										(char*)"[%s:%d] Thread av_record_thread Create Failed." , __FILE__,__LINE__,FRAG_REASSEMBLY_MODULE_NAME);
				return -1;
			}
    	}	
		g_frag_run.av_digest_record_lq= (MESA_lqueue_head*)calloc(1, g_frag_cfg.thread_num*sizeof(MESA_lqueue_head));
		for(i=0;i<g_frag_cfg.thread_num;i++)
    	{		
			g_frag_run.av_digest_record_lq[i] =  MESA_lqueue_create(1, 0);			
			if(-1 == create_pthread(av_digest_record_thread,(void*)i,g_frag_run.logger))
			{
				MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
										(char*)"[%s:%d] Thread av_digest_record_thread Create Failed." , __FILE__,__LINE__,FRAG_REASSEMBLY_MODULE_NAME);
				return -1;
			}
    	}		
	}		
	
	/*stat thread*/
	if(0<g_frag_stat.stat_interval)
	{
		if(-1 == create_pthread(thread_stat_output,NULL,g_frag_run.logger))
    	{
			MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									(char*)"[%s:%d] Thread thread_stat_output Create Failed." ,__FILE__,__LINE__);
			return -1;
    	}
	}		

	/*sysinfo thread*/	
	if(0<g_frag_stat.sysinfo_interval)
	{
		if(-1 == create_pthread(thread_sysinfo_output,NULL,g_frag_run.logger))
    	{
			MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									(char*)"[%s:%d] Thread thread_sysinfo_output Create Failed." ,__FILE__,__LINE__);
			return -1;
    	}
	}	

	/*WLB*/		
	if(0<g_frag_cfg.wlb_on)
	{
		if(-1 == create_pthread(wlb_report,NULL,g_frag_run.logger))
    	{
			MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									(char*)"[%s:%d] Thread WiredLB Create Failed." ,__FILE__,__LINE__);
			return -1;
    	}
	}		
	
	/* bizman recv data from papp*/
	/*bizman接收无队列模式*/
	if(0==g_frag_cfg.bizman_queue_mode)
	{	
		for(i=0;i<g_frag_cfg.thread_num;i++)
		{
			if(-1 == create_pthread(bizman_recv_data,(void*)i,g_frag_run.logger))
			{
				MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
										(char*)"[%s:%d] Thread bizman_recv_data Create Failed." , __FILE__,__LINE__,FRAG_REASSEMBLY_MODULE_NAME);
				return -1;
			}
		}
	}
	/*bizman接收队列模式*/
	else 
	{
		g_frag_run.recv_bizman_lq = (MESA_lqueue_head*)calloc(1, g_frag_cfg.thread_num*sizeof(MESA_lqueue_head));
    	for(i=0;i<g_frag_cfg.thread_num;i++)
    	{		
			g_frag_run.recv_bizman_lq[i] =  MESA_lqueue_create(1, 0);
			if(-1 == create_pthread(bizman_recv_data_to_queue, (void*)i, g_frag_run.logger))
			{
				MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
										(char*)"[%s:%d] Thread bizman_recv_data_to_queue Create Failed." , __FILE__,__LINE__,FRAG_REASSEMBLY_MODULE_NAME);
				return -1;
			} 
    	}
		
		for(i=0;i<g_frag_cfg.thread_num;i++)
    	{    	
			if(-1 == create_pthread(bizman_recv_data_from_queue,(void*)i,g_frag_run.logger))
			{
				MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
										(char*)"[%s:%d] Thread bizman_recv_data_from_queue Create Failed." , __FILE__,__LINE__,FRAG_REASSEMBLY_MODULE_NAME);
				return -1;
			}
		}	
	}	
	
	if(g_frag_cfg.asmis_switch)
	{
		rssb_asmis_init();		
	}		

	/*asmis*/		
	if(g_frag_cfg.asmis_switch)
	{
		if(-1 == create_pthread(rssb_asmis_task,NULL,g_frag_run.logger))
    	{
			MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									(char*)"[%s:%d] Thread rssb_asmis_task Create Failed." ,__FILE__,__LINE__);
			return -1;
    	}
	}

	if(g_frag_cfg.bfd_recv_port>0)
	{
		bfd_port_t bfd_port;
		bfd_port.recv_port = g_frag_cfg.bfd_recv_port;
		hard_keepalive_run(&bfd_port);
	}	
	
	while(1)
	{
		if(g_frag_cfg.asmis_switch)
		{
			sleep(10);			
			asmis_log_HeartBeat(g_frag_run.asmis_log_handle, "rssb_maskey heartbeat");		
		}	
		else
		{
			pause();
		}
	}
	
	return 0;
}


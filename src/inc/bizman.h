/*Businessman Data Exchange System 
 *Based on the short story "The fisherman and the businessman"
 *Author:zhengchao@iie.ac.cn
 *History:2011-08-24 first build;
 		2011-09-20 v0.01 start test;
 		2011-12-31 v0.02 multi bug fixed;
 		2012-01-16 memset 1MB memory can cause obvious performance reduction;
 		2012-01-19 v0.03 1)support multi cast interface
 						2)support smooth by dest ip and port;
 		2012-03-06 v0.04 1)fix crash recovery bug;
 						2)support chunk mode receive;
 						3)support push send,send immediately after smooth without timeout;
 		2012-03-08 v0.05 1)add choke recv mode,recv will blocked if no data in bizman;
 		2012-03-12 v0.06	1)add magic number as packet header check;
 						2)fix bug,when receiver crash restore leave gabarge in to_ack_timout_queue and 
 						ready_index_queue;
 		2012-03-13 v0.07 1)count_last_frag_num_in_packet bug fixed;
 						2)defrozen failed bug caused by no sendtimes reset fixed; 
 						3)fix memleaks at destory sender when receiver crush;
		2012-03-15 v0.07	1)fix copy_recv_data's queue operation before mutex;
						2)defrozen stream's first slice maybe incomplete is normal,because its previous part
						have been abandoned;
		2012-03-19 v0.08 1)add use pag(special network interface)send packets;
		2012-03-20 v0.09 1)reconstruct bizman.h,hide internal information to outside;
		2012-03-21 v0.10 1)use define instead of const in bizman.h
						2)rename functions in hash.cpp,to avoid function name collision in link
		2012-03-22 v0.11 1)add simple statistic,read reliable layer's stream first packet send and resend count;
		2012-03-26 v0.12 add reliable receive stream expire,fix stream expire bug in reliabe send;
		2012-03-29 v0.13 1)fix stream expire bug in reliable receive;
						2) improve send and resend statistic;
		2012-04-01 v0.14 1)fix bug when receive reduandent force establish stream packet reduandent, cause
						app receive duplicate app data.Add force establish stream interval to fix this bug;
						2)On heavy load,may expire send stream which have in use task, will screwed queue,
						fixed by check stream's in_use_task_num before expired;
		2012-04-11 v0.15 1)add sender cached packets limit;
						2)stat dropped pkts;
						3)fix bugs, send smooth stream expire time + max resend interval *2 = reliabe recv stream
						expire time,send smooth stream expire time > reliable send time;
		2012-04-12 v0.16 1)rebuild stat interface;
						2)fix bugs of memory corrupt caused by thread conflict when recv,enlarge mutex proctect
						range;
		2012-04-14 v0.18 1)fix do_actual_send's sendto return value judgemen;
						2)fix add_raw_packet function false abandon dup START_SEQ_NUM,also lose first sequence 
						when receiver unordered sequence  ;
		2012-05-11 v0.19 1)fix sender's smooth default interval zero bug;
						2)fix FORCE_EST_INTERVAL 20min(1000000ms) bug,sho
						3)fix receiver's ack smooth default interval 10s bug;
		2012-05-24 v0.20 1)fix smooth block may copy expired father stream bug;
						2)use mutex_lock instead of try_lock to get smoothed block;
						3)add thread smooth space limit;
		2012-06-11 v0.21	1)add choke recv timeout feature;
		2012-06-18 v0.22 1)businesman_limit.h TIMEOUT_RECV_STREAM_USEC init read array overflow;
						2)smooth send and reliable send stream expire after send data,a busy receiver may drop lazy
						sender's packet,move expire operation in front of send;
		2012-07-24 v1.0 1)check slice_len in is_bizman_packet;
		2013-03-20 v1.01 1)shorten mutex contension area in recv;
                         			2)fix add_raw_packet bugs of filling wrong sequence in ack packet,which caused 250Mbps bottleneck;
		2013-05-14 v1.02 fix unreliable packet receive dead lock bug in add_raw_packet,caused by v1.01 shorten mutex improvement;
		2013-06-19 v1.03 1)fix isolate_recv_record dead lock;
						2)last_send_time type is uint ,cause overflow,change to time_tick_t;
		2013-07-02 v1.04 v1.01 shorten mutex proved wrong,called deadlock,backward.
		2013-10-28 v1.05 Not set chunk end after read uncomplete slice.
 TODO:	1)add select unreachable error;
 		2)add send file interface,save one memory copy time;
 		3)add max memory used limited;
 		
 NOTICE:	1)recv function is NOT thread safe,MUST use your own mutex lock protect the handle;
 		2)send function is thread safe when you send with differenct thread id,but call send function\
 		with the same thread id in different thread is NOT safe;
 		3)unsigned short port is host order,unsigned int ip address is network layer.
 		4)compatible with gcc,link with -lstdc++
*/
#ifndef _BIZMAN_HEADER_INCLUDE_
#define _BIZMAN_HEADER_INCLUDE_

//initial flag
#define BIZMAN_SENDER_ACTIVE	0x01
#define BIZMAN_RECEIVER_ACTIVE	0x02

#define BIZMAN_CHUNK_RECEIVE	0x08
#define BIZMAN_CHOKE_RECEIVE	0x10


//set option
#define BIZMAN_OPT_SOCKET_INTERFACE_NAME	0x00
#define BIZMAN_OPT_PAG_INTERFACE_NAME	0x01

//set parameter
#define BIZMAN_PARAMETER_SEND_SMOOTH_TIME_MSEC		0x01
#define	BIZMAN_PARAMETER_RESEND_BASE_MSEC			0x02
#define	BIZMAN_PARAMETER_RESEND_TIMES				0x03	//5 at most
#define	BIZMAN_PARAMETER_SEND_CACHE_PKTS_LIMIT		0x04
#define	BIZMAN_PARAMETER_ACK_ACCUMULATE_NUM			0x05
#define	BIZMAN_PARAMETER_ACK_ACCUMULATE_MSEC		0x06
#define	BIZMAN_PARAMETER_SEND_WINDOWSIZE			0x07
#define	BIZMAN_PARAMETER_RECV_WINDOWSIZE			0x08
#define BIZMAN_PARAMETER_ACK_SMOOTH_TIME_MSEC		0x09
#define	BIZMAN_PARAMETER_FROZEN_STREAM_MSEC			0x0a
#define	BIZMAN_PARAMETER_SMOOTH_CACHE_PKTS_LIMIT	0x0b

#define BIZMAN_PARAMETER_CHUNK_RECEIVE				0x10
#define	BIZMAN_PARAMETER_CHOKE_RECEIVE				0x11
#define	BIZMAN_PARAMETER_CHOKE_RECV_TIMEOUT_MSEC	0x12

//send flag
#define	BIZMAN_RELIABLE_SEND	0x01
#define	BIZMAN_UNRELIABLE_SEND	0x00
#define	BIZMAN_LAST_PKT	0x02
#define	BIZMAN_PUSH_SEND	0x04
#define	BIZMAN_SMOOTH_DEST	0x08


//recv flag
#define	BIZMAN_READ_CRAM	0x00
#define	BIZMAN_READ_STREAM	0x01
#define	BIZMAN_READ_CHUNK	0x02

//stat
#define BIZMAN_STAT_FIRST_SEND_PKTS		0x01
#define	BIZMAN_STAT_FIRST_RESEND_PKTS	0x02
#define	BIZMAN_STAT_LAST_RESEND			0x03
#define	BIZMAN_STAT_MEM_DROPPED			0x04
#define	BIZMAN_STAT_FROZEN_DROPPED		0x05
#define	BIZMAN_STAT_ACK_PKT_NUM			0x06
#define	BIZMAN_STAT_FIRST_SEND_STREAM	0x07
#define	BIZMAN_STAT_FIRST_RESEND_STREAM	0x08
#define	BIZMAN_STAT_ACTIVE_STREAM		0x09

#ifdef	__cplusplus
extern "C"{
#endif
/*
*return an unintialized handle when success;
*return NULL when failed.
*/
void *bizman_get_handle(void);
void bizman_destroy_handle(void * raw_handle);
/*
BIZMAN_SENDER_ACTIVE:0x01;BIZMAN_RECEIVER_ACTIVE:0x02;BIZMAN_PAG_USE:0x04;
*/
int bizman_set_handle(void* raw_handle,unsigned short flags);
int bizman_set_handle_option(void* raw_handle,int option_type, const char* option_value);
int bizman_set_handle_parameter(void* raw_handle,int type, unsigned int value);
//only used on sender
int bizman_set_pkts_cached_limit(void* raw_handle,unsigned long long cached_pkts_limit);

//unsigned short port is host order,unsigned int ip address is network layer.
int bizman_listen(void* raw_handle,unsigned short port);
int bizman_init_handle(void* raw_handle);

int bizman_recv(void* raw_handle,char* buff,int buf_len,unsigned int *src_ip,unsigned short *src_port,unsigned int *stream_id,unsigned int* is_complete);
int bizman_send(void* raw_handle,unsigned int thread_id,unsigned int dest_ip,unsigned short dest_port,
					const char*data,unsigned int len,unsigned long long app_id,unsigned short flags);
int bizman_multi_send(void* raw_handle,unsigned int thread_id,const unsigned int *dest_ip,const unsigned short *dest_port,unsigned int dest_num,
					const char*data,unsigned int len,unsigned long long app_id,unsigned short flags);
unsigned long long bizman_stat(void* raw_handle,int type);
#ifdef	__cplusplus
}
#endif

#endif

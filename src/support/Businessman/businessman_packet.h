#ifndef _BIZMAN_PACKET_INCLUDE_H_
#define _BIZMAN_PACKET_INCLUDE_H_
#include "list.h"
typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short int uint16;
typedef unsigned char uint8;
const int MTU_SIZE=1472;
const uint32	START_SEQ_NUM=1;
const uint8	FLAG_NEED_ACK=0x01;
const uint8	FLAG_LAST_PKT=0x02;
const uint8 	FLAG_ACK_PKT=0x04;
const uint8	FLAG_FIN_STREAM=0x08;
const uint8	FLAG_EST_STREAM=0x10;

const uint8	SLICE_LAST_FRAG=0x01;
const uint32	PACKET_MAGIC_NUM=0x132B5FC;
typedef struct _businessman_load_header{
	uint32	magic_num;
	uint32	id;				//identify the stream
#ifdef DEPLOY_YSP_SYSTEM
	char		app_id[8];
#endif
	uint32	sequence;		//
	uint16	len;			//including this header
	uint8	slice_num;	//255 at most
	uint8	flags;		//LAST NEEDACK 
}__attribute__((packed))bizman_packet_t;//24B
typedef struct _businessman_slice_header{
	uint16	slice_len;	//including this header
	uint8	flags;
	uint8	reserved;
}bizman_slice_t;//4B
typedef struct _businessman_wrap
{
	unsigned int src_ip;
	unsigned int dest_ip;
	unsigned short src_port;
	unsigned short dest_port;
}bizman_wrap_t;
typedef struct _bizman_reliable_send_task_t
{
	uint32	dest_ip;
	uint16	dest_port;
	bizman_packet_t* data;
	int len;
}bizman_packet_send_task_t;
typedef struct _businessman_send_data_refer_t{
	uint32	data_len;
	uint32	left_quote_count;
	bizman_packet_t*	packet;
}send_data_refer_t;
typedef struct _businessman_send_data_index_t{
	list_index_t hook;
	send_data_refer_t* refer_index;
}send_data_index_t;
const unsigned int LOAD_HEADER_LEN=sizeof(bizman_packet_t);
const unsigned int SLICE_HEADER_LEN=sizeof(bizman_slice_t);
const unsigned int HEADER_RESERVED_LEN=LOAD_HEADER_LEN+SLICE_HEADER_LEN;
const unsigned int LOAD_DAT_LEN=MTU_SIZE-HEADER_RESERVED_LEN;
void init_packet(bizman_packet_t *p_pkt,char is_reliable,uint32 stream_id,int pkt_len);
int is_bizman_packet(bizman_packet_t *p_pkt);
void set_packet_flag_last(bizman_packet_t* p_pkt);
void set_packet_flag_need_ack(bizman_packet_t* p_pkt);
void free_refer_and_data(send_data_refer_t* refer);
send_data_refer_t* create_send_data_refer(char*data,int len,int dest_num);
#endif /*end _BIZMAN_PACKET_INCLUDE_H_*/


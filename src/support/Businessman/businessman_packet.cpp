#include "businessman_limit.h"
#include "businessman_memory.h"
#include "businessman_packet.h"
void init_packet(bizman_packet_t *p_pkt,char is_reliable,uint32 stream_id,int pkt_len)
{
		if(is_reliable==TRUE)
		{
			p_pkt->flags|=FLAG_NEED_ACK;
		}
		p_pkt->id=stream_id;
		p_pkt->len=pkt_len;
		p_pkt->magic_num=PACKET_MAGIC_NUM;
		p_pkt->slice_num=0;
}
int is_bizman_packet(bizman_packet_t *p_pkt)
{
	int i=0,user_data_len=0;
	bizman_slice_t* slice=NULL;
	if(p_pkt->magic_num!=PACKET_MAGIC_NUM)
		return FALSE;
	if(p_pkt->len>MTU_SIZE)
		return FALSE;
	slice=(bizman_slice_t*)((char*)p_pkt+LOAD_HEADER_LEN);
	for(i=0;i<p_pkt->slice_num;i++)
	{
		if(slice->slice_len>p_pkt->len-LOAD_HEADER_LEN-user_data_len-i*SLICE_HEADER_LEN)
		{
			return FALSE;
		}
		user_data_len+=slice->slice_len-SLICE_HEADER_LEN;
		slice=(bizman_slice_t* )((char*)slice+slice->slice_len);
	}
	if(user_data_len+LOAD_HEADER_LEN+p_pkt->slice_num*SLICE_HEADER_LEN!=p_pkt->len)
		return FALSE;
	return TRUE;
}
void set_packet_flag_last(bizman_packet_t* p_pkt)
{
	p_pkt->flags|=FLAG_LAST_PKT;
}
void set_packet_flag_need_ack(bizman_packet_t* p_pkt)
{
	p_pkt->flags|=FLAG_NEED_ACK;
}
send_data_refer_t* create_send_data_refer(char*data,int len,int dest_num)
{
	send_data_refer_t* refer=NULL;
	refer=(send_data_refer_t*)b_malloc(sizeof(send_data_refer_t));
	refer->packet=(bizman_packet_t*)data;
	refer->data_len=len;
	refer->left_quote_count=dest_num;
	return refer;
}
void free_refer_and_data(send_data_refer_t* refer)
{
	refer->left_quote_count--;
	if(refer->left_quote_count==0)
	{
		free(refer->packet);
		free(refer);
	}
	return;
}

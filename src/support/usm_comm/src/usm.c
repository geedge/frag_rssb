#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <assert.h>
#include "usm_api.h"
#include "output.h"
#include "MESA_ring_queue.h"
#include "MESA_handle_logger.h"

#define USM_COMM			"usm_comm.so"

#define MAX_READER_NUM		7
#define	MAX_UND_PATH_SIZE	128

#define MAX_WRAP_NUM		((1500 - 8 - 4)/sizeof(uint32_t))//372
#define SUIT_WRAP_NUM 		300
#define MTU_SIZE			1500

#define MAX_LQ_SIZE_DE		8*1024*1024
#define SM_TM_DE			1000000//nsec

#define atomic_t unsigned long long

void usm_version_0_9_20170509()
{
	//20170117 V0.1 create the new project,define api
	//20170208 V0.2 first whole project.reader side is muti-threading
	//20170210 V0.3 alter reader side to single thread
	//20170214 v0.4 test for 10w write.
	//20170216 v0.5 test online first.alter the use of timespec and mesa_lq
	//20170221 v0.6 tentative joint debugging with ict  
	//20170222 v0.7 attempt memory barrier 
	//20170223 v0.8 muti_users md5 verify succeed,use uint64_t flag in _pkt_node
	//20170309 v0.9 intergrate usm_read
	//20170310 v0.9 debug for single thread
	//20170313 v0.9 support no_block for sending_thread
	//20170314 v0.9 support muti-thread by atomic 
	//20170314 v0.9 alter MESA_ring_queue 
	//20170407 v0.9 DEBUG log ->FATAL log tem 
	//20170509 v0.9 alter to DEBUG log and add lq_count 
}
struct _msg_header
{
	uint16_t magic_num;
	uint8_t version;
	uint8_t msg_type;
	uint32_t cont_len;
};

#define MSG_HEADER_LEN	sizeof(struct _msg_header)
typedef struct msg_metainfo_s
{
	char						prog_id[8];	
	char    					flag;
    uint8_t   					hitservice;
	uint64_t  					prog_len:48;
	uint32_t					cap_IP;
	uint8_t						protocol;		
	uint8_t						media_type;	
	uint8_t						data_flag;	
	uint8_t						opt_num;      
}msg_metainfo_t;
/*
struct _pkt_node
{
	union{
		volatile char flag[2];
		volatile short iflag;
	};
	time_t timestamp;
	uint16_t pkt_len;
	char pkt_data[MTU_SIZE];
};*/
struct _pkt_node
{
	uint64_t flag[MAX_READER_NUM + 1];
	time_t timestamp;
	uint64_t pkt_len;
	char pkt_data[MTU_SIZE+4];
};


struct _usm_stat_t
{
	uint64_t	wt_pktnum;//已完成写入
	uint64_t	wt_datalen;
	uint64_t	err_wting_pktnum;//正写入冲突，记录
	uint64_t	err_wting_datalen;
};
struct _usm_reader_stat_t
{ 
	uint64_t	sd_pktnum;//已发送
	uint64_t	rd_pktnum;//已完成读取
	uint64_t	rd_datalen;
	uint64_t	not_rd_pktnum;//未完成读取
	uint64_t	not_rd_datalen;
	uint64_t	err_rding_pktnum;//正读取冲突，记录
	uint64_t	err_rding_datalen;
	uint64_t	lq_count;
};

struct _usm_dest_t
{	
	int un_sd;
	int max_lqueue_size;
	//int lq_flag;//err:1
	void* log_handle;
	MESA_ring_queue_head msg_queue;//writer:send_msg_queue
	long smooth_time;
	struct _usm_reader_stat_t stat;
	char reader_path[MAX_UND_PATH_SIZE];
};
struct _usm_writer
{	
	int reader_cnt;
	unsigned long long latest_wt_ofst;
	struct _usm_dest_t readers[MAX_READER_NUM];	
	struct _usm_stat_t wt_view_stat;		
};
struct _usm_reader
{
	int reader_id;
	int un_sd;
	//int max_lqueue_size;
	void* log_handle;
	//MESA_lqueue_head pkt_queue;
	char reader_path[MAX_UND_PATH_SIZE];
};

struct _usm_handle_t
{	
	int shm_key;
	int	role;
	struct _pkt_node* shm_addr;
	uint64_t shm_size;
	union
	{
		struct _usm_writer writer_info;
		struct _usm_reader reader_info;
	};
};
struct _usm_outer_msg_t
{
	struct _msg_header header;
	uint32_t msg_cnt;
	uint32_t offset[MAX_WRAP_NUM];
};

USM_t* USM_handle(int shm_key,uint64_t shm_size,int role)
{
	struct _usm_handle_t* _handle = (struct _usm_handle_t*)malloc(sizeof(struct _usm_handle_t));
	memset(_handle,0,sizeof(struct _usm_handle_t));
	
	_handle->shm_key = shm_key;
	_handle->shm_size = shm_size;
	_handle->role = role;
	if(_handle->role == USM_WRITER)
	{
		int i =0;
		for(i =0;i<MAX_READER_NUM;i++)
		{
			_handle->writer_info.readers[i].max_lqueue_size = MAX_LQ_SIZE_DE;
			_handle->writer_info.readers[i].smooth_time = SM_TM_DE;
		}		
	}

	return (USM_t*)_handle;
}

int USM_set_opt(USM_t* handle,enum USM_opt_t type,void* data,int size,int reader_id)
{
	struct _usm_handle_t *_handle = (struct _usm_handle_t*)handle;
	switch(type)
	{
	case READER_CNT:
		_handle->writer_info.reader_cnt = *(const int*)data;
		break;
	case READER_ID:
		_handle->reader_info.reader_id = *(const int*)data;
		break;
	case SMOOTH_TIME:
		if(_handle->role == USM_WRITER)
		{
			_handle->writer_info.readers[reader_id].smooth_time = *(const int*)data;
		}
		break;
	case MAX_LQUEUE_SIZE:
		if(_handle->role == USM_WRITER)
		{
			_handle->writer_info.readers[reader_id].max_lqueue_size = *(const int*)data;
		}
		break;
	case READER_PATH:
		if(_handle->role == USM_READER)
		{
			memcpy(_handle->reader_info.reader_path,(const char*)data,size); 
		}
		else
		{
			memcpy(_handle->writer_info.readers[reader_id].reader_path,(const char*)data,size);
		}
		break;
	case LOG_HANDLE:
		if(_handle->role == USM_READER)
		{
			_handle->reader_info.log_handle = data;
		}
		else
		{
			_handle->writer_info.readers[reader_id].log_handle = data;
		}
		break;
	default:
		break;
	}
	return USM_SUCC;
}

void* sending_thread(void * reader)
{
	int	rst = 0;
	struct _usm_dest_t *_reader=(struct _usm_dest_t *)reader;
	struct _usm_outer_msg_t to_send;
	memset(&to_send,0,sizeof(struct _usm_outer_msg_t));
	to_send.header.magic_num = 0x5641;
	to_send.header.version = 3;
	to_send.header.msg_type = 0x50;
	struct timespec last_send_time,now;
	struct sockaddr_un dest_un;
	memset(&dest_un,0,sizeof(struct sockaddr_un));
	dest_un.sun_family=AF_UNIX;
	snprintf(dest_un.sun_path,sizeof(dest_un.sun_path),"%s",_reader->reader_path);
	uint32_t offset = 0;
	long offset_len = sizeof(offset);
	long lq_count = 0;
	clock_gettime(CLOCK_MONOTONIC,&last_send_time);//init
	int sent_cnt = 0;
	int send_len = 0;
	while(1)
	{		
		clock_gettime(CLOCK_MONOTONIC,&now);
		lq_count = MESA_lqueue_get_count(_reader->msg_queue);
		lq_count = MESA_ring_queue_get_count(_reader->msg_queue);
		_reader->stat.lq_count = lq_count;
		
		if(to_send.msg_cnt < MAX_WRAP_NUM)//boundary protection
		{
			rst = MESA_ring_queue_get(_reader->msg_queue,&offset,&offset_len);

			if(rst < 0)
			{
				MESA_handle_runtime_log(_reader->log_handle, RLOG_LV_FATAL, USM_COMM,
                                        "MESA_rqueue_get failed. error num is %d.\n",rst);
			}
			else
			{
				to_send.offset[to_send.msg_cnt] = offset;
                to_send.msg_cnt++;
			}

		}
		else
		{
			//empty
		}
		
		if((to_send.msg_cnt >= SUIT_WRAP_NUM)||
			((now.tv_sec - last_send_time.tv_sec)*1000000000 + (now.tv_nsec - last_send_time.tv_nsec) > _reader->smooth_time))
		{
			to_send.header.cont_len = to_send.msg_cnt*sizeof(uint32_t) + sizeof(uint32_t);
			send_len = sizeof(struct _msg_header) + to_send.header.cont_len;
			rst = named_unix_domain_socket_send(_reader->un_sd,&dest_un,(const char*)&to_send,send_len);

			if(-1 == rst)
			{
				MESA_handle_runtime_log(_reader->log_handle, RLOG_LV_FATAL, USM_COMM,"Unix-domain send data: loss data %s.",strerror(errno));
			}
			else
			{
				//assert(send_len == (to_send.msg_cnt*sizeof(uint32_t) + sizeof(uint32_t)+ sizeof(struct _msg_header)));
				MESA_handle_runtime_log(_reader->log_handle, RLOG_LV_DEBUG, USM_COMM,
					"Send msg%d to %s. un_sd:%d. msg_cnt:%llu.last offset is %llu.SEND LEN is %llu.",
					sent_cnt,_reader->reader_path,_reader->un_sd,to_send.msg_cnt,to_send.offset[to_send.msg_cnt -1],send_len);
				
				_reader->stat.sd_pktnum += to_send.msg_cnt;
				sent_cnt ++;

				last_send_time = now;
			}
			to_send.msg_cnt = 0;//reset
								
		}		
	}
	
}

int USM_init(USM_t* handle)
{
	struct _usm_handle_t *_handle = (struct _usm_handle_t*)handle;
    pthread_t	 	thread_id[_handle->writer_info.reader_cnt];
	//pthread_attr_t 	attr;
	uint16_t i = 0;
	uint32_t rq_buflen = sizeof(uint32_t);
	uint32_t rq_num = 0;
	int shmid;	
	shmid = shmget((key_t)_handle->shm_key, sizeof(struct _pkt_node)*_handle->shm_size, 0666 | IPC_CREAT);
	if(shmid == -1)
	{
		if(_handle->role == USM_READER)
		{
			return USM_RD_SHMGET_ERR;
		}
		else
		{
			//get err with writer
			if (shmctl(shmid, IPC_RMID, 0) == -1) 
			{
				return USM_SHMCTL_ERR;
			}
			shmid = shmget((key_t)_handle->shm_key, sizeof(struct _pkt_node)*_handle->shm_size, 0666 | IPC_CREAT);
			if(shmid == -1)
			{
				return USM_WT_SHMGET_ERR;
			}
		}
	}
	printf("Apply the shared memory SUCCEED. The shmid is %d.\n",shmid);
	//shmat to local variable, namely shm_addr
	_handle->shm_addr = (struct _pkt_node*)shmat(shmid, (void *)0, 0);
	if(_handle->shm_addr == (void*)-1)
	{
		return USM_SHMAT_ERR;
	}
	printf("Link the shared memory SUCCEED.\n");
	
	if(_handle->role == USM_WRITER)
	{
	    memset(_handle->shm_addr,0,sizeof(struct _pkt_node)*_handle->shm_size);
		printf("Clean the shm finished.\n");
	}
	/*if(0 > mprotect(_handle->shm_addr,sizeof(struct _pkt_node)*_handle->shm_size,PROT_NONE))
	{
		printf("mprotect PROT_NONE error: %s.",strerror(errno));
	}*/	
	char file_name[128]={0};
	if(_handle->role == USM_READER)
	{
		_handle->reader_info.un_sd = init_output_un_socket(_handle->reader_info.reader_path);
		
		if(_handle->reader_info.un_sd == -1)
		{
			return USM_SOCKET_INIT_ERR;
		}
		/*
		struct timeval ti;
		ti.tv_sec = 10;
		ti.tv_usec = 0;
		setsockopt(_handle->reader_info.un_sd,SOL_SOCKET,SO_RCVTIMEO,&ti,sizeof(ti));*/
	}
	else
	{
		for(i = 0; i < _handle->writer_info.reader_cnt; i++)
		{
			sprintf(file_name,"/home/usm_send%d",i);
			_handle->writer_info.readers[i].msg_queue = MESA_ring_queue_born();
			rq_num = _handle->writer_info.readers[i].max_lqueue_size;
			MESA_ring_queue_set_opt(_handle->writer_info.readers[i].msg_queue,RQO_RING_ELEMENT_NUM,&(rq_num),sizeof(int));
			MESA_ring_queue_set_opt(_handle->writer_info.readers[i].msg_queue,RQO_PRE_ALLOC_BUF_LEN,&rq_buflen,sizeof(uint32_t));
			if(MESA_ring_queue_mature(_handle->writer_info.readers[i].msg_queue))
			{
				return USM_QUEUE_INIT_ERR;
			}

			_handle->writer_info.readers[i].un_sd = init_output_un_socket(file_name);

			//_handle->writer_info.readers[i].un_sd = init_output_un_socket(_handle->writer_info.readers[i].reader_path);
			if(_handle->writer_info.readers[i].un_sd == -1)
			{
				return USM_SOCKET_INIT_ERR;
			}
			pthread_create(&thread_id[i],NULL,sending_thread,&(_handle->writer_info.readers[i]));
			//pthread_create(&thread_id[i],&attr,sending_thread,&(_handle->readers[i]));
		}		
	}

	return USM_SUCC;
}

int USM_write(USM_t * handle, const char * data, int datalen)
{
	struct _usm_handle_t *_handle=(struct _usm_handle_t*)handle;
	int rst = 0;
	//struct _usm_inner_msg_t *q=NULL;
	//struct _usm_inner_msg_t p;
	//memset(&p,0,sizeof(struct _usm_inner_msg_t));
	uint32_t cur_offset = 0;
	struct _pkt_node *curnode=NULL;
	uint16_t i = 0;
	unsigned long long to_use_offset=0;
	assert( datalen <= MTU_SIZE );

	//debug
	/*struct _msg_header* msgheader = (struct _msg_header*)data;
	msg_metainfo_t* msginfo = (msg_metainfo_t*)(data + MSG_HEADER_LEN);
	assert(msgheader->magic_num == 22081);
	
	if(msgheader->msg_type == 49)
	{
		assert(msginfo->opt_num == 0);
	}*/
	to_use_offset = __sync_fetch_and_add(&(_handle->writer_info.latest_wt_ofst),1);
	to_use_offset = to_use_offset % _handle->shm_size;
	curnode = _handle->shm_addr + to_use_offset;
	
	//MESA_handle_runtime_log(_handle->writer_info.readers[0].log_handle, RLOG_LV_DEBUG, USM_COMM,
		//	"wt_ofst is %d.",_handle->writer_info.latest_wt_ofst);
			
	/*if(0 > mprotect(_handle->shm_addr,sizeof(struct _pkt_node)*_handle->shm_size,PROT_WRITE))
	{
		printf("mprotect  PROT_WRITE error: %s.",strerror(errno));
	}*/
	/*for(i = 0;i< _handle->writer_info.reader_cnt; i++)
	{
		MESA_handle_runtime_log(_handle->writer_info.readers[i].log_handle, RLOG_LV_DEBUG, USM_COMM,
			"Cur_offset is %d. Before write flag is %o,%o. The buff length is %d. The fgets buff is %s. ",
			_handle->writer_info.latest_wt_ofst,curnode->flag[0],curnode->flag[1],datalen,data);
	}*/
	//int flag = (int)curnode->flag[0] + (int)(curnode->flag[1] << 8);
	//mb();
	//writing err
	//if(curnode->iflag&0x01)
	if(curnode->flag[0]&0x01)
	{
		_handle->writer_info.wt_view_stat.err_wting_pktnum ++;
		_handle->writer_info.wt_view_stat.err_wting_datalen += datalen;
	}
	for(i = 0;i < _handle->writer_info.reader_cnt; i++)
	{
		//reading err
		//if(curnode->iflag&(0x01<<(i*2+2)))
		if(curnode->flag[i+1]&0x01)
		{
			_handle->writer_info.readers[i].stat.err_rding_pktnum++;
			_handle->writer_info.readers[i].stat.err_rding_datalen += datalen;
		}
		
		//read already
		//if(curnode->iflag&(0x01<<(i*2+3)))
		if(curnode->flag[i+1]&0x02)
		{
			_handle->writer_info.readers[i].stat.rd_pktnum++;
			_handle->writer_info.readers[i].stat.rd_datalen += datalen;
		}
		else//not read
		{
			_handle->writer_info.readers[i].stat.not_rd_pktnum++;
			_handle->writer_info.readers[i].stat.not_rd_datalen += datalen;
		}
	}

	curnode->flag[0] |= 0x01;//set writing
	
	curnode->pkt_len = datalen;
	
	memcpy(curnode->pkt_data,data,curnode->pkt_len);

	curnode->flag[0] &= 0x00;//set back to not writing
	curnode->flag[0] |= 0x02;//set writen already
	
	//set back to not read
	for(i = 0;i < _handle->writer_info.reader_cnt; i++)
	{
		/*if(i<3)
		{
			curnode->flag[0] &= (~(0x01<<(i*2+3)));
		}
		else
		{
			curnode->flag[1] &= (~(0x01<<(i*2-5)));
		}*/
		curnode->flag[i+1]&=0x00;
		
	}
	/*if(0 > mprotect(_handle->shm_addr,sizeof(struct _pkt_node)*_handle->shm_size,PROT_NONE))
	{
		printf("mprotect PROT_NONE error: %s.",strerror(errno));
	}*/
	
	cur_offset = to_use_offset;	
	for(i=0;i<_handle->writer_info.reader_cnt;i++)
	{
		//make a duplication of p
		//q=(struct _usm_inner_msg_t*)malloc(sizeof(struct _usm_inner_msg_t));
		//memcpy(q,&p,sizeof(p));
		//rst =  MESA_lqueue_join_tail(_handle->writer_info.readers[i].msg_queue,&cur_offset,sizeof(cur_offset));
		rst = MESA_ring_queue_join(_handle->writer_info.readers[i].msg_queue,&cur_offset,sizeof(cur_offset));
//debug
		//MESA_handle_runtime_log(_handle->writer_info.readers[i].log_handle, RLOG_LV_FATAL, USM_COMM,
			//	"JOIN offset is %d.",cur_offset);		
		if(rst < 0)
		{
			MESA_handle_runtime_log(_handle->writer_info.readers[i].log_handle, RLOG_LV_FATAL, USM_COMM,
				"MESA_rqueue_join failed. error num is %d.\n",rst);
		}
		//int lq_count = MESA_lqueue_get_count(_handle->writer_info.readers[i].msg_queue);
		//printf("MESA_lqueue cur_count %llu.\n",lq_count);
		//MESA_handle_runtime_log(_handle->writer_info.readers[i].log_handle, RLOG_LV_DEBUG, USM_COMM,
        //"MESA_lqueue cur_count %llu.\n",lq_count);	
	}
	_handle->writer_info.wt_view_stat.wt_pktnum ++;
	_handle->writer_info.wt_view_stat.wt_datalen += datalen;
	
	return USM_SUCC;
}

int USM_read(USM_t* handle,char** data,int* datalen,int* data_cnt)
{
	struct _usm_handle_t *_handle = (struct _usm_handle_t*)handle;
	//MESA_lqueue_head pkt_queue = _handle->reader_info.pkt_queue;
	int un_sd = _handle->reader_info.un_sd;
	struct _pkt_node* shm_addr = _handle->shm_addr;
	struct _pkt_node *curnode = NULL;
	struct sockaddr sa;	
	socklen_t sa_len = sizeof(struct sockaddr);
	struct _usm_outer_msg_t to_recv;
	memset(&to_recv,0,sizeof(struct _usm_outer_msg_t));
	int recv_len = 0;
	int i = 0;
	//uint64_t cur_offset = 0;
	//long offset_len = sizeof(uint64_t);
	//int rst = 0;
	recv_len = recvfrom(un_sd,&to_recv,sizeof(struct _usm_outer_msg_t),0,(struct sockaddr*)&sa,&sa_len);
	if(recv_len < 0)
	{
		MESA_handle_runtime_log(_handle->reader_info.log_handle, RLOG_LV_FATAL, USM_COMM,
			"Unix-domain recv data: %s\n",strerror(errno));
		
		return -1;
	}
	else
	{
		/*if(0 > mprotect(_handle->shm_addr,sizeof(struct _pkt_node)*_handle->shm_size,PROT_READ))
		{
			printf("mprotect PROT_READ error: %s.",strerror(errno));
		}*/
		
		*data_cnt = to_recv.msg_cnt;
		for(i =0; i < to_recv.msg_cnt; i++)
		{
			curnode = shm_addr + to_recv.offset[i];
			//MESA_handle_runtime_log(_handle->reader_info.log_handle, RLOG_LV_DEBUG, USM_COMM,
				//"Recv flag is %d,%d.",curnode->flag[0],curnode->flag[1]);
			
			//mb();
			
			//set reading
			/*if(_handle->reader_info.reader_id <3)
			{
				curnode->flag[0] |= (0x01 << (_handle->reader_info.reader_id*2 + 2));
			}
			else
			{
				curnode->flag[1] |= (0x01 << (_handle->reader_info.reader_id*2 - 6));
			}*/
			
			curnode->flag[_handle->reader_info.reader_id+1] |= 0x01;

			//if(datalen[i] > curnode->pkt_len)
			//{
				datalen[i] = curnode->pkt_len;
			//}		
			data[i] = curnode->pkt_data;//only copy pointer
			
			//set not reading and set read already
			/*if(_handle->reader_info.reader_id <3)
			{
				curnode->flag[0] &= (~(0x01 << (_handle->reader_info.reader_id*2 + 2)));
				curnode->flag[0] |= (0x01 << (_handle->reader_info.reader_id*2 + 3));
			}
			else
			{
				curnode->flag[1] &= (~(0x01 << (_handle->reader_info.reader_id*2 - 6)));
				curnode->flag[1] |= (0x01 << (_handle->reader_info.reader_id*2 - 5));
			}*/
			curnode->flag[_handle->reader_info.reader_id+1] &= 0x00;
			curnode->flag[_handle->reader_info.reader_id+1] |= 0x02;
			
			/*MESA_handle_runtime_log(_handle->reader_info.log_handle, RLOG_LV_DEBUG, USM_COMM,
				"After read flag is %d,%d.  The length is %d. The read out buff is %s.",
				curnode->flag[0],curnode->flag[1],*datalen,data);	*/		
		}
	}
	return 0;
}

uint64_t USM_stat(USM_t* handle,enum USM_stat_t type,int reader_id)
{
	uint64_t i=0;
	struct _usm_handle_t *_handle=(struct _usm_handle_t*)handle;
	
	switch(type)
	{
	case WRITED_SIZE:
		i = _handle->writer_info.wt_view_stat.wt_datalen; 
		break;
	case WRITED_CNT:
		i = _handle->writer_info.wt_view_stat.wt_pktnum;
		break;
	case WRITING_CLASH_SIZE:
		i = _handle->writer_info.wt_view_stat.err_wting_datalen;
		break;
	case WRITING_CLASH_CNT:	
		i = _handle->writer_info.wt_view_stat.err_wting_pktnum;
		break;
	case SENDED_CNT:
		i = _handle->writer_info.readers[reader_id].stat.sd_pktnum;
		break;	
	case READED_SIZE:
		i = _handle->writer_info.readers[reader_id].stat.rd_datalen;
		break;
	case READED_CNT:
		i = _handle->writer_info.readers[reader_id].stat.rd_pktnum;
		break;
	case READER_DROP_SIZE:
		i = _handle->writer_info.readers[reader_id].stat.not_rd_datalen;
		break;
	case READER_DROP_CNT:
		i = _handle->writer_info.readers[reader_id].stat.not_rd_pktnum;
		break;
	case READING_CLASH_SIZE:
		i = _handle->writer_info.readers[reader_id].stat.err_rding_datalen;
		break;
	case READING_CLASH_CNT:
		i = _handle->writer_info.readers[reader_id].stat.err_rding_pktnum;
		break;
	case LQ_COUNT:
		i = _handle->writer_info.readers[reader_id].stat.lq_count;
		break;
	default:
		break;
	}
	return i; 
}

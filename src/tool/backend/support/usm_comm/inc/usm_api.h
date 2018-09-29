/*
 * usm_api.h
 *
 *  Created on: 2017-1-17
 *      Author: dmj
 */
#ifndef	__USM_COMM_H_INCLUDE_
#define __USM_COMM_H_INCLUDE_
#ifdef	__cplusplus
extern "C" {
#endif

/*
#ifndef uint64_t
typedef unsigned long long  uint64_t;
#endif

#ifndef uint16_t
typedef unsigned int  uint16_t;
#endif
*/
typedef	struct
{
}USM_t;

#define	USM_READER	0x01
#define	USM_WRITER	0x02

enum USM_rst_t
{
	USM_SUCC = 0,		
	USM_RD_SHMGET_ERR = -1,
	USM_SHMCTL_ERR = -2, 
	USM_WT_SHMGET_ERR = -3,
	USM_SHMAT_ERR = -4,
	USM_SOCKET_INIT_ERR = -5,
	USM_QUEUE_INIT_ERR = -6
};

enum USM_opt_t
{
	SHM_BUF_SIZE,//WRITER and READER VIEW.VALUE is interger,SIZE=sizeof(int). DEFAULT:65535.
	READER_CNT,//WRITER VIEW.
	SMOOTH_TIME,//WRITER VIEW.but every reader need to set
	READER_ID,//READER VIEW.VALUE is interger,SIZE=sizeof(int). 0-7.Reader needs set read_id first.
	MAX_LQUEUE_SIZE,//READER VIEW.VALUE is interger,SIZE=sizeof(int). DEFAULT:1500.
	READER_PATH,//READER VIEW.data is a const char* und_path, define the reader's Unix named domain socket file path. return reader id if success.
	LOG_HANDLE
};

enum USM_stat_t
{
	WRITED_SIZE,
	WRITED_CNT,
	WRITING_CLASH_SIZE,
	WRITING_CLASH_CNT,	
	SENDED_CNT,		//READERS
	READED_SIZE,
	READED_CNT,
	READER_DROP_SIZE,//number of reader missed due to overwrite
	READER_DROP_CNT,	
	READING_CLASH_SIZE,
	READING_CLASH_CNT,
	LQ_COUNT
};

USM_t* USM_handle(int shm_key,uint64_t shm_size,int role);
int USM_init(USM_t* handle);
int USM_write(USM_t* handle,const char* data,int datalen);
int USM_read(USM_t* handle,char** data,int* datalen,int* data_cnt);
int USM_set_opt(USM_t* handle,enum USM_opt_t type,void* data,int size,int reader_id);
uint64_t USM_stat(USM_t* handle,enum USM_stat_t type,int reader_id);


#ifdef	__cplusplus
}
#endif
#endif

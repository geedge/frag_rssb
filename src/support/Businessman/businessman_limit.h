#ifndef	_BUSINESSMAN_LIMIT_INCLUDE_H_
#define	_BUSINESSMAN_LIMIT_INCLUDE_H_

#include <stdio.h>
#include "businessman_time.h"
#ifndef	TRUE
#define TRUE	1
#endif //end TURE

#ifndef	FALSE
#define FALSE 0
#endif //end false

#ifndef MAX
#define MAX(a, b)  	(((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b)  	(((a) < (b)) ? (a) : (b))
#endif

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })
#endif

const int MAX_THREAD_SUPPORT=64;



const int MAX_DAT_SEND_PER_LOOP=300;
const int MAX_ACK_RECV_PER_LOOP=6;
const int MAX_DAT_RECV_PER_LOOP=MAX_DAT_SEND_PER_LOOP;
const int MAX_ACK_SEND_PER_LOOP=MAX_ACK_RECV_PER_LOOP;

const unsigned int MAX_RESEND_TIMES=5;
const unsigned int DEFAULT_TIMEOUT_RELIABLE_SEND_BASE_USEC=500*1000; 
const time_tick_t DEFAULT_TIMEOUT_RELIABLE_SEND_BASE_TICK=DEFAULT_TIMEOUT_RELIABLE_SEND_BASE_USEC/TIME_TICK_STEP_USEC;
const unsigned int TIMEOUT_RELIABLE_SEND_MULTIPLE[MAX_RESEND_TIMES]={1,1,1,1,1};

const time_tick_t RESEND_PERSIST_INTERVAL=DEFAULT_TIMEOUT_RELIABLE_SEND_BASE_USEC*TIMEOUT_RELIABLE_SEND_MULTIPLE[MAX_RESEND_TIMES-1];
const time_tick_t FORCE_EST_INTERVAL=RESEND_PERSIST_INTERVAL*2*MAX_RESEND_TIMES;

const time_tick_t DEFAULT_STREAM_FROZEN_TICK_INTERVAL=30*1000*1000/TIME_TICK_STEP_USEC;

const unsigned int TIMEOUT_SMOOTH_STREAM_USEC=10*60*1000*1000; // 10min
const unsigned int TIMEOUT_SEND_STREAM_USEC=9*60*1000*1000; // 9min,a little less than TIMEOUT_SMOOTH_STREAM_USEC
const unsigned int TIMEOUT_RECV_STREAM_USEC=TIMEOUT_SMOOTH_STREAM_USEC+TIMEOUT_RELIABLE_SEND_MULTIPLE[MAX_RESEND_TIMES-1]*DEFAULT_TIMEOUT_RELIABLE_SEND_BASE_USEC*MAX_RESEND_TIMES;

//TIMEOUT_SMOOTH_BLOCK_USEC+ACK_COMULATE_USEC<TIMEOUT_RELIABLE_SEND_BASE_USEC
const unsigned int DEFAULT_TIMEOUT_SMOOTH_BLOCK_USEC=10*1000;
const unsigned int DEFAULT_COMULATE_ACK_NUM=5;
const unsigned int DEFAULT_ACK_COMULATE_USEC=100*1000;
const unsigned int DEFALUT_SEND_WINDOW_SIZE=30;
const unsigned int DEFALUT_RECV_WINDOW_SIZE=DEFALUT_SEND_WINDOW_SIZE;

#endif /*_BUSINESSMAN_LIMIT_INCLUDE_H_*/


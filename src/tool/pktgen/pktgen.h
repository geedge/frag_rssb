#ifndef _PKTGEN_H_
#define _PKTGEN_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "loadprof.h"

#define MAX_ACK 10240

#define DEFAULT_CONFIG          "pktgen.ini"

#define AVS_FLAG_ACK        0x1

#define CONN_FLAG_FIRST          0x01
#define CONN_FLAG_LAST           0x02
#define CONN_FLAG_CLOSE          0x04
#define CONN_FLAG_ACK            0x08
#define SETFLAG(x, flag) x |= flag
#define UNSETFLAG(x, flag) x &= ~flag
#define TESTFLAG(x, flag) (0 != (x & flag)) 

typedef struct conn_info {
    uint64_t    prog_id;    // program id
    uint64_t    prog_off:48;// program offset
    uint16_t    win_size;   // send window size
    uint64_t    data_len:48;// data to be transmitted in this conn
    uint16_t    flag;       // new | last 
    uint64_t    data_off;   // data that has been sent out
} conn_info_t;

typedef struct runtime_para {
    // Parameters in config file
    uint32_t stop_cond;
    uint32_t stop_pkt_num;               // pkt max
    uint32_t stop_prog_num;               // pkt max
    uint32_t conn_num_max;              // connection num
    uint32_t send_rate_pps;
    uint32_t multi_thread;
	uint32_t set_ack_flag;

	uint32_t log_prog;

    uint32_t loss_ratio;
    uint32_t rep_ratio;

    uint32_t prog_len_max;
    uint32_t version_cur;           // current version
    uint32_t info_interv;
    addr4_t dstaddr;
} runtime_para_t;


typedef struct pktgen {
    conn_info_t *conn;
    uint32_t conn_num_cur;
    uint64_t count_pkt;
    uint64_t count_byte;
    uint32_t count_prog;
    uint32_t count_lost;
    int sockfd;
} pktgen_t;

typedef struct conn_distributor {
    uint64_t    prog_id;
    uint64_t    prog_len:48;
    uint64_t    thread_len;
    uint8_t     thread_num; //max 255
    uint8_t     thread_cur;
} conn_distributor_t;

#endif // _PKTGEN_H_

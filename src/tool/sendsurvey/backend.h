#ifndef _BACKEND_H_
#define _BACKEND_H_

#include "loadprof.h"

#define BUF_SIZE 1500

#define LOG_SURVEY 		"survey.log"
#define LOG_ERROR 		"error.log"
#define DEFAULT_CONFIG 	"backend.conf"

typedef struct parameter {
	uint32_t log_switch;
	uint32_t timeout;
	uint32_t queue_size;
	uint32_t level;
    uint16_t data_port;
    uint16_t survey_port;
	uint8_t service;
} parameter_t;

typedef struct delay_elem {
	char prog_id[8];
	long long expire_time;
	uint32_t src_ip;
} delay_elem_t;

typedef struct delay_queue {
    delay_elem_t *buf;
    int head;
    int tail;
    int max_len;
} delay_queue_t; 

#endif /*_BACKEND_H_*/

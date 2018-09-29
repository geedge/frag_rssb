
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "loadprof.h"
#include "input.h"
#include "output.h"
#include "Com_SendToBack.h"
#include "log.h"
#include "backend.h"

delay_queue_t *g_delay_queue;
parameter_t *g_para = NULL;

delay_queue_t *delay_queue_init(int max_len)
{
	delay_queue_t *dq = (delay_queue_t *)calloc(1, sizeof(delay_queue_t));
	if (NULL == dq) {
		fprintf(stderr, "No enough Memory for queue!");
		return NULL;
	}

	dq->buf = (delay_elem_t *)calloc(max_len, sizeof(delay_elem_t));
	if (NULL == dq->buf) {
		fprintf(stderr, "No enough Memory for q->buf!");
		free(dq); 
		return NULL;
	}

	dq->head = 0;
	dq->tail = 1;
	dq->max_len = max_len;
	return dq;
}


void *send_routine(void *arg) 
{
	uint16_t port_survey = g_para->survey_port;
	FILE *fd_log;

	char buf_survey[BUF_SIZE];
	struct msg_header_t *mh_survey = (struct msg_header_t*)buf_survey;
	struct survey_ind_t *msi_survey = (struct survey_ind_t *)(buf_survey + MSG_HEADER_LEN);

	memset(buf_survey, 0, BUF_SIZE);
	mh_survey->magic_num = PROTO_MAGICNUM;
	mh_survey->version = 3;
	mh_survey->msg_type = MSG_RESP_CHECKRESULT;
	mh_survey->cont_len = SURVEY_MSG_FIX_LEN;
	msi_survey->service = g_para->service;
	msi_survey->level = g_para->level; 
	msi_survey->cfg_id = 28; 

	int fd_survey = output_udp_init();
	if (fd_survey < 0) {
		printf("socket error!");
		exit(1);
	}

	if (g_para->log_switch) {
		fd_log = fopen(LOG_SURVEY,"ab+");
		if (NULL == fd_log) {
			printf("log init error!");
			exit(1);
		}
	}

	int head_new;
	int url_len;
	delay_queue_t *q = g_delay_queue;
	while (1) {
		head_new = (q->head + 1) % q->max_len;
		while (head_new == q->tail) 
			if (g_para->timeout > 0)
				sleep(g_para->timeout);
			else
				sleep(1);
		delay_elem_t *qe = q->buf + head_new;
		while (qe->expire_time > time(NULL))
			sleep(1);

		memcpy(msi_survey->prog_id, qe->prog_id, 8);
		url_len = 1 + sprintf(buf_survey + MSG_HEADER_LEN + SURVEY_MSG_FIX_LEN, "/home/log/%lu.log", *((uint64_t *)qe->prog_id));	
		mh_survey->cont_len = SURVEY_MSG_FIX_LEN + url_len;
		output_udp_send(fd_survey, qe->src_ip, port_survey, 
				(unsigned char *)buf_survey, MSG_HEADER_LEN + SURVEY_MSG_FIX_LEN + url_len);  

		if (g_para->log_switch) {
			char ntop_buf[20];
			write_local_log(fd_log, "dst %s pid %-11llu service 0x%2x level %d", 
					inet_ntop(AF_INET, &qe->src_ip, ntop_buf, sizeof ntop_buf),
					*((unsigned long long*)msi_survey->prog_id), 
					msi_survey->service,
					msi_survey->level);
			fflush(fd_log);
		}

		q->head = head_new;
	}
}

int read_profile(char *fpath)
{
	if (NULL != g_para) {
		fprintf(stderr, "g_para has been initialized!");
		return -1;
	}

	static parameter_t para;
	g_para = &para;

	int rc;
	/* [SYSTEM] */
	rc = profile_read_nport(fpath, "SYSTEM", "SURVEY_PORT", &g_para->survey_port, 1);
	if (rc < 1)
		return -1;
	rc = profile_read_nport(fpath, "SYSTEM", "DATA_PORT", &g_para->data_port, 1);
	if (rc < 1)
		return -1;
	rc = profile_read_nbyte_hex(fpath, "SYSTEM", "SERVICE", &g_para->service, 1);
	if (rc < 1)
		return -1;
	profile_read_uint(fpath, "SYSTEM", "TIMEOUT", &g_para->timeout, 0);
	profile_read_uint(fpath, "SYSTEM", "LOG_SWITCH", &g_para->log_switch, 0);
	profile_read_uint(fpath, "SYSTEM", "LEVEL", &g_para->level, 0);
	profile_read_uint(fpath, "SYSTEM", "QUEUE_SIZE", &g_para->queue_size, 10);
	if (g_para->queue_size < 1)
		g_para->queue_size = 10;

	return 0;
}

int main (int argc, char *argv[])
{
	char *fprof;
	if (argc > 2) {
		fprintf(stderr, "\nUsage: %s [<config_file>]\n", argv[0]);
		exit(0);
	}
	// read in configs
	fprof = DEFAULT_CONFIG;
	if (argc > 1)
		fprof = argv[1];

	if (read_profile(fprof) < 0) {
		fprintf(stderr, "[Error]: Read config file error!\n");
		exit(1);
	}

	g_delay_queue = delay_queue_init(g_para->queue_size);
	if (NULL == g_delay_queue) {
		printf("g_delay_queue init error!");
		exit(1);
	}

	int fd_data = input_udp_init(g_para->data_port);
	if (fd_data < 0) {
		printf("socket error!");
		exit(1);
	}

	FILE *fd_error;
	if (g_para->log_switch) {
		fd_error = fopen(LOG_ERROR,"ab+");
		if (NULL == fd_error) {
			printf("log init error!");
			exit(1);
		}
	}

	pthread_t send_tid;
	pthread_create(&send_tid, NULL, send_routine, NULL);

	uint32_t src_ip;
	uint32_t size;
	char buf_data[BUF_SIZE];
	struct msg_header_t *mh_data = (struct msg_header_t *)buf_data;
	struct metainfo_t *mi_data = (struct metainfo_t *)(buf_data + MSG_HEADER_LEN);
	delay_queue_t *q = g_delay_queue;
	delay_elem_t *qe;
	while(1) {
		if (input_udp_recv(fd_data, &src_ip, (unsigned char *)buf_data, &size) < 0)
			continue;
		if (MSG_DATA_METAINFO != mh_data->msg_type)
			continue;
		if ((q->tail + 1) % q->max_len == q->head) {
			if (g_para->log_switch) {
				char ntop_buf[20];
				write_local_log(fd_error, "drop - src %s pid %-11llu", 
						inet_ntop(AF_INET, &src_ip, ntop_buf, sizeof ntop_buf),
						*((unsigned long long*)mi_data->prog_id));
				fflush(fd_error);
			}
			sleep (1);
			continue;
		}
		qe = q->buf + q->tail;
		memcpy(qe->prog_id, mi_data->prog_id, 8);
		qe->expire_time = time(NULL) + g_para->timeout;
		qe->src_ip = src_ip;
		q->tail = (q->tail + 1) % q->max_len;
	}
	return EXIT_SUCCESS;
}	// end of function main


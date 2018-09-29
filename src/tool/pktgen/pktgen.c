// pktgen.c
//
// generatio avs pkt  
// 
// MODIFIED by
// CREATED by Guo Le, 08/17/2010

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "Com_SendToBack.h"
#include "loadprof.h"
#include "debug.h"
#include "error.h"
#include "output.h"
#include "log.h"

#include "pktgen.h"

pktgen_t *g_pktgen = NULL;
runtime_para_t *g_para = NULL;
FILE *g_log_prog_fd = NULL;

int thread_num_choice[10] = {1, 1, 1, 1, 1, 10, 20, 30, 40, 50};

int pktgen_readprof (char *fpath)
{
	if (NULL == fpath) {
		DEBUG_MESSAGE("null pointer");
		return -1;
	}
	static runtime_para_t para;
	g_para = &para; 

	profile_read_uint(fpath, "SYSTEM", "STOP_CONDITION", &g_para->stop_cond, 0);
	if (g_para->stop_cond) 
		profile_read_uint(fpath, "SYSTEM", "STOP_PROG_NUM", &g_para->stop_prog_num, 0);
	else 
		profile_read_uint(fpath, "SYSTEM", "STOP_PKT_NUM", &g_para->stop_pkt_num, 0);
	profile_read_uint(fpath, "SYSTEM", "CONN_NUM_MAX", &g_para->conn_num_max, 1);
	profile_read_uint(fpath, "SYSTEM", "SEND_RATE_PPS", &g_para->send_rate_pps, 0); 
	profile_read_uint(fpath, "SYSTEM", "MULTI_THREAD", &g_para->multi_thread, 0); 

	profile_read_uint(fpath, "SYSTEM", "LOSS_RATIO", &g_para->loss_ratio, 10);
	profile_read_uint(fpath, "SYSTEM", "REPITITION_RATIO", &g_para->rep_ratio, 10);
	profile_read_uint(fpath, "SYSTEM", "SET_ACK_FLAG", &g_para->set_ack_flag, 0); 

	profile_read_uint(fpath, "SYSTEM", "REPORT_INTERVAL", &g_para->info_interv, 0);
	profile_read_uint(fpath, "SYSTEM", "LOG_PROG", &g_para->log_prog, 0); 

	profile_read_uint(fpath, "PROTOCOL", "CURRENT_VERSION", &g_para->version_cur, 0x1);
	profile_read_uint(fpath, "PROTOCOL", "MAX_PROGRAM_LEN", &g_para->prog_len_max, 1024);

	profile_read_naddr(fpath, "OUTPUT", "DST_ADDRESS", &g_para->dstaddr, 1);

	return 0;
}

int pktgen_init()
{
	if (NULL == g_pktgen || NULL ==  g_para) {
		DEBUG_MESSAGE("null pointer");
		return -1;
	}

	g_pktgen->conn = (conn_info_t *)calloc(g_para->conn_num_max, 
			sizeof(conn_info_t));
	if(NULL == g_pktgen->conn) {
		DEBUG_MESSAGE("calloc error");
		return -1;
	}

	g_pktgen->sockfd = output_udp_init();
	if (g_pktgen->sockfd < 0) {
		printf("Read profile failed!");
		exit(1);
	}

	g_pktgen->conn_num_cur = 0;
	g_pktgen->count_pkt = 0;
	g_pktgen->count_byte = 0;
	g_pktgen->count_prog = 0;
	g_pktgen->count_lost = 0;

	if (g_para->log_prog) {
		g_log_prog_fd = fopen("prog.log","ab+");
		if (NULL == g_log_prog_fd)
			return -1;
	}
	return 0;
}

// Clearing before exit
// 
// Return:
//  void
void pktgen_clear()
{
	if (g_pktgen == NULL)
		return;
	fprintf(stdout, "-----------------------------------------------------------------------\n");
	fprintf(stdout, "\nDone generating packets, wheew! ");
	fprintf(stdout, " Statistics \n");
	fprintf(stdout, " - #pkt: %lu\n", g_pktgen->count_pkt); 
	fprintf(stdout, " - #byte: %lu\n", g_pktgen->count_byte); 
	fprintf(stdout, " - #Prog: %u\n", g_pktgen->count_prog); 
	fprintf(stdout, " - #lost: %u\n", g_pktgen->count_lost); 
	fprintf(stdout, "=======================================================================\n");

	output_udp_clear(g_pktgen->sockfd);
	free(g_pktgen);
}


// Signal callback function
//
// Return:
//  void
void sig_hand_int(
		int signo)  // [IN] signal number
{
	pktgen_clear();
	exit(1);
}

// Push info to stdout
//
// Return
//  void
void print_runtime_para() 
{
	fprintf(stdout, "=======================================================================\n");
	fprintf(stdout, " Parameters:\n");
	fprintf(stdout, "-----------------------------------------------------------------------\n");
	fprintf(stdout, " [SYSTEM]\n");
	fprintf(stdout, " - Stop condition : %u\n", g_para->stop_cond);
	if (!g_para->stop_cond)
		fprintf(stdout, " - Stop pkt num : %u\n", g_para->stop_pkt_num);
	else
		fprintf(stdout, " - Stop prog num : %u\n", g_para->stop_prog_num);
	fprintf(stdout, " - Max concurent connection : %u\n", g_para->conn_num_max);
	fprintf(stdout, " - Loss ratio : %u/1,000,000\n", g_para->loss_ratio);
	fprintf(stdout, " - Repitition ratio : %u/1,000,000\n", g_para->rep_ratio);
	fprintf(stdout, " - Report interval(s) = %d\n", g_para->info_interv);
	fprintf(stdout, " - Send rate control(pps) : %d\n", g_para->send_rate_pps);
	fprintf(stdout, " - Use Multi-Thread : %s\n", g_para->multi_thread ? "On" : "Off");
	fprintf(stdout, " - Set ACK flag : %s\n", g_para->set_ack_flag ? "On" : "Off");
	fprintf(stdout, " - Log prog switch : %s\n", g_para->log_prog ? "On" : "Off");

	fprintf(stdout, " [PROTOCOL]\n");
	fprintf(stdout, " - Max program length : %d Byte\n",
			g_para->prog_len_max);
	fprintf(stdout, " - Current protocol version : %x\n",
			g_para->version_cur);

	fprintf(stdout, " [OUTPUT]\n");
	char buf[20];
	fprintf(stdout, " - Destination address: %s:%d\n",
			inet_ntop(AF_INET, &g_para->dstaddr.s_addr, buf, sizeof buf), 
			ntohs(g_para->dstaddr.sin_port));

	fprintf(stdout, "=======================================================================\n");
	fprintf(stdout, "  | %20s | %10s | %10s | %10s | %10s | %10s | %10s | %10s |\n",
			"time", "#pkt", "#byte", "pps", "bps", "#conn", "#prog", "#lost");

	fflush(stdout);
}

void *report(void *arg)
{
	time_t currTime;
	struct tm *now;
	char strTime[32];

	struct timeval interv_start, interv_end; 
	uint64_t proc_rate_pps;
	uint64_t proc_rate_bps;
	uint64_t interv_time;
	uint64_t pkt_count_last = 0;      // interval packet counter
	uint64_t pkt_count_cur = 0;
	uint64_t byte_count_last = 0;
	uint64_t byte_count_cur = 0;

	while (1) {
		gettimeofday(&interv_start, NULL); 
		sleep(g_para->info_interv);

		time(&currTime);
		now = localtime(&currTime);
		memset(strTime, 0, sizeof(strTime));
		strftime(strTime, sizeof(strTime), "%Y-%m-%d %H:%M:%S", now);

		pkt_count_cur = g_pktgen->count_pkt;
		byte_count_cur = g_pktgen->count_byte;
		gettimeofday(&interv_end, NULL); 
		interv_time = 1000000 * (interv_end.tv_sec - interv_start.tv_sec) + 
			interv_end.tv_usec - interv_start.tv_usec;
		proc_rate_pps = 1000000 * 
			(pkt_count_cur - pkt_count_last) / interv_time;
		proc_rate_bps = 8 * 1000000 * 
			(byte_count_cur - byte_count_last) / interv_time;

		fprintf(stdout, "  | %20s | %10lu | %10lu | %10lu | %10lu | %10u | %10u | %10u |\n",
				strTime,
				pkt_count_cur - pkt_count_last,
				byte_count_cur - byte_count_last,
				proc_rate_pps,
				proc_rate_bps,
				g_pktgen->conn_num_cur,
				g_pktgen->count_prog,
				g_pktgen->count_lost
			   );
		pkt_count_last = pkt_count_cur;
		byte_count_last = byte_count_cur;
		fflush(stdout);
	}
	return 0;
}


inline uint64_t rand64()
{
	uint64_t ret = rand();
	ret <<= 32;
	ret |= rand();
	return ret;
}


void create_conn (int idx)
{
	static conn_distributor_t conn_dis = {0};

	g_pktgen->count_prog ++;

	if (!conn_dis.prog_id) {
		conn_dis.prog_id = rand64();
		//conn_dis.prog_id = 555 + 1403641;
		conn_dis.prog_len = (conn_dis.prog_id % g_para->prog_len_max) + 1;
		conn_dis.thread_num = g_para->multi_thread ? thread_num_choice[rand() % 10] : 1;
		conn_dis.thread_len = conn_dis.prog_len / conn_dis.thread_num;
		conn_dis.thread_cur = 0;
	}

	conn_info_t *conn_cur = g_pktgen->conn + idx;
	conn_cur->prog_id = conn_dis.prog_id; 
	conn_cur->prog_off = conn_dis.thread_len * conn_dis.thread_cur;
	conn_cur->win_size = rand() % 900 + 500; // 500 - 1400
	conn_cur->data_len = conn_dis.thread_len; 
	conn_cur->data_off = 0;
	conn_cur->flag = 0;
	if (0 == conn_dis.thread_cur) {
		SETFLAG(conn_cur->flag, CONN_FLAG_FIRST);
		conn_cur->prog_off = conn_dis.prog_len; // store prog_len in prog_off 
	}
	if (conn_dis.thread_num - 1 == conn_dis.thread_cur) {
		conn_cur->data_len += conn_dis.prog_len % conn_dis.thread_num;
		conn_dis.prog_id = 0;
	}
	conn_dis.thread_cur ++;
}

void pktgen_run(pktgen_t *g_pktgen)
{
	uint8_t buf[2000];
	struct msg_header_t *mh = (struct msg_header_t*) buf;
	struct metainfo_t *mi = (struct metainfo_t*) (buf + MSG_HEADER_LEN);
	struct avs_data_t *ad = (struct avs_data_t*) (buf + MSG_HEADER_LEN);

	mh->magic_num = PROTO_MAGICNUM;
	mh->version = g_para->version_cur;
	mh->msg_type = MSG_DATA_METAINFO;
	mh->cont_len = 0;

	uint64_t sendone_time, send_time;
	if (g_para->send_rate_pps > 0) 
		sendone_time = 1000000 / g_para->send_rate_pps;
	send_time = sendone_time;

	int run = 1;
	while (run) {
		struct timeval sendone_end; 

		uint32_t idx;
		if (g_para->stop_prog_num > 0 && g_pktgen->count_prog >= g_para->stop_prog_num) {
			idx = rand() % g_pktgen->conn_num_cur;
		} else {
			idx = rand() % g_para->conn_num_max;
			if (idx >= g_pktgen->conn_num_cur) {
				create_conn(g_pktgen->conn_num_cur);
				idx = g_pktgen->conn_num_cur;
				g_pktgen->conn_num_cur ++;
			}
		}

		conn_info_t *conn_cur = g_pktgen->conn + idx;

		if (TESTFLAG(conn_cur->flag, CONN_FLAG_FIRST)) { // metainfo 
			mh->msg_type = MSG_DATA_METAINFO;
			//mh->msg_type = MSG_DATA_BODY;
			mh->cont_len = METAINFO_HEAD_LEN;
			memcpy(mi->prog_id, &conn_cur->prog_id, 8);
            mi->cap_IP = inet_addr("10.53.2.15");
			mi->prog_len = conn_cur->prog_off; // [ATTENTION]
			mi->media_type = 0x68; //TODO
            mi->hitservice = 0x80;
			conn_cur->prog_off = 0;
			UNSETFLAG(conn_cur->flag, CONN_FLAG_FIRST);

			if (g_para->log_prog) {
				write_local_log(g_log_prog_fd, "pid %-20llu byte %-15lu", 
						conn_cur->prog_id, 
						mi->prog_len);
			}

		} else {
			mh->msg_type = MSG_DATA_BODY;
			memcpy(ad->prog_id, &conn_cur->prog_id, 8);
			//ad->pad = 0;
			ad->offset = conn_cur->prog_off + conn_cur->data_off;
			if (g_para->set_ack_flag && ad->offset < 10240)
				SETFLAG(ad->flags, AVS_FLAG_ACK);
			if (conn_cur->win_size + conn_cur->data_off >= conn_cur->data_len) {
				// last pkt
				mh->cont_len = AVS_DATA_HEAD_LEN + 
					conn_cur->data_len - conn_cur->data_off;
				*conn_cur = g_pktgen->conn[g_pktgen->conn_num_cur-1];	
				g_pktgen->conn_num_cur --;
				if (!g_pktgen->conn_num_cur)
					run = 0;
			} else {
				mh->cont_len = AVS_DATA_HEAD_LEN + conn_cur->win_size;
				conn_cur->data_off += conn_cur->win_size;
			}
		}

		g_pktgen->count_pkt ++;
		g_pktgen->count_byte += MSG_HEADER_LEN + mh->cont_len;

		// drop 
		if (g_para->loss_ratio > 0 &&
				(uint32_t)rand() < g_para->loss_ratio * RAND_MAX / 1000000) {
			g_pktgen->count_lost ++;
			continue;
		}

		// send one
		if (output_udp_send(g_pktgen->sockfd, g_para->dstaddr.s_addr, g_para->dstaddr.sin_port, buf, MSG_HEADER_LEN + mh->cont_len) < 0) {
			printf("send error");
			continue;
		}

		if (g_para->stop_pkt_num &&	g_pktgen->count_pkt >= g_para->stop_pkt_num) 
			run = 0;

		if (g_para->send_rate_pps > 0) {
			uint64_t now;
			gettimeofday(&sendone_end, NULL);
			now = sendone_end.tv_sec * 1000000 + sendone_end.tv_usec;
			while (now < send_time) {
				//usleep(10);
				gettimeofday(&sendone_end, NULL);
				now = sendone_end.tv_sec * 1000000 + sendone_end.tv_usec;
			}
			send_time = now + sendone_time;
		}

	}
	return;
}


int main(int argc, char *argv[])
{
	char *fprof;

	if (argc > 3) {
		fprintf(stdout, "\nUsage: %s [<config_file>]\n\n", argv[0]);
		exit(0);
	}
	fprof = DEFAULT_CONFIG;
	if (argc > 1)
		fprof = argv[1];



	fprintf(stdout, "Profile: %s\n", fprof);
	if (pktgen_readprof(fprof) < 0) {
		printf("Read profile failed!");
		exit(1);
	}

	print_runtime_para();

	srand(time(NULL));

	g_pktgen = (pktgen_t *)calloc(1, sizeof(pktgen_t));
	if (NULL == g_pktgen) {
		DEBUG_MESSAGE("calloc error!");
		pktgen_clear();
		exit(1);
	}


	if (pktgen_init() < 0) {
		printf("pktgen init failed!!");
		pktgen_clear();
		exit(1);
	}


	signal(SIGINT, sig_hand_int);

	pthread_t report_tid;
	if (g_para->info_interv > 0) {
		pthread_create(&report_tid, NULL, report, NULL);
	}

	pktgen_run(g_pktgen);

	pktgen_clear();

	return EXIT_SUCCESS;
}
	


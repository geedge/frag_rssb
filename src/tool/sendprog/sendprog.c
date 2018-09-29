// sendprog.c 
//
//   
// 
// MODIFIED by
// CREATED by Guo Le, 10/16/2011

#include "sendprog.h"
void print_usage(char *pname)
{
    static const char *USAGE = 
        "Usage: %s -i <input file> -o <output addr> [OPTION]\n"
        "    -i <input file>,  input file name.\n"
        "    -o <output addr>,  output socket. eg. 127.0.0.1:22082\n"
        "    -t <timeout sec>, default 1 sec.\n"
        "    -p <prog_id>, in hex or dec. eg.0x624353641237 \n"
        "    -h,  print this help.\n"
        "\n"
        "    eg. %s -i ./if.mp3 -o 127.0.0.1:22082 -t 310 -p 0x624353641237\n";

    fprintf(stderr, USAGE, pname, pname);

}

int send_prog(FILE *fp, int sockfd, uint64_t flen, uint64_t prog_id, int timeout)
{
    char pkt_mi[1500];
    char pkt_ad[1500];

    struct msg_header_t *mh_mi = (struct msg_header_t*)pkt_mi; 
    struct msg_header_t *mh_ad = (struct msg_header_t*)pkt_ad; 
    struct metainfo_t *mi = (struct metainfo_t*)(pkt_mi + MSG_HEADER_LEN);
    struct avs_data_t *ad = (struct avs_data_t*)(pkt_ad + MSG_HEADER_LEN); 
    uint8_t *data = (uint8_t*)ad + AVS_DATA_HEAD_LEN;
    uint64_t max_data_len = 1400 - MSG_HEADER_LEN - AVS_DATA_HEAD_LEN;


    // init metainfo pkt
	*((uint64_t *)(mi->prog_id)) = prog_id;
	//mi->flag = META_FLAG_DUMP;
	mi->hitservice = 0x83;
	mi->prog_len = flen;
	mi->cap_IP = inet_addr("10.53.2.15");
	mi->protocol = 0;
	mi->media_type = 0x68;
	mi->data_flag = 0x02;
	mi->opt_num = 0; 
    mh_mi->magic_num = PROTO_MAGICNUM; 
    mh_mi->version = PROTO_VERSION;  
    mh_mi->msg_type = MSG_DATA_METAINFO;
    mh_mi->cont_len = METAINFO_HEAD_LEN;

    // init avs_data pkt
	*((uint64_t *)(ad->prog_id)) = prog_id;
    ad->pad = 0;
    ad->flags = 0;
    ad->offset = 0;
    mh_ad->magic_num = PROTO_MAGICNUM; 
    mh_ad->version = PROTO_VERSION;  
    mh_ad->msg_type = MSG_DATA_BODY;
    mh_ad->cont_len = 0;

    int ret;
    int i;

    printf("Sending metainfo * 3 ... \n");

    // send metainfo 3 times
    for (i=0; i<1; i++) {
        ret = output_udp_send_connect(sockfd, pkt_mi, mh_mi->cont_len + MSG_HEADER_LEN); 
        if (ret != mh_mi->cont_len + MSG_HEADER_LEN) {
            fprintf(stderr, "output_udp_send_connect error!\n");
            return -1;
        }
    }

    // send avs_data
    fprintf(stdout, "Sending avs_data ... \n");
    while (flen > 0) {
        int payload;
        if (flen > max_data_len) {
            payload = max_data_len;
        } else {
            payload = flen;
        }
        mh_ad->cont_len = payload + AVS_DATA_HEAD_LEN;
        flen -= payload;

        if (1 != fread(data, payload, 1, fp)) {
            fprintf(stderr, "fread error!\n");
            break;
        }

        //printf("%lu \n", mh_ad->cont_len + MSG_HEADER_LEN);
        ret = output_udp_send_connect(sockfd, pkt_ad, mh_ad->cont_len + MSG_HEADER_LEN); 
        if (ret != mh_ad->cont_len + MSG_HEADER_LEN) {
            fprintf(stderr, "output_udp_send_connect error!\n");
            perror("");
            break;
        }
        ad->offset += payload;
    }
	return 0;

    printf("Waiting until timesout ...    ");
    for (i=0; i<timeout; i++) {
        fprintf(stdout, "\b\b\b\b\b\b\b%3d sec", i);
        fflush(stdout);
        sleep(1);
    }

    fprintf(stdout, "\nSending metainfo * 3 ... \n");
    // send metainfo 3 times again
    for (i=0; i<3; i++) {
        ret = output_udp_send_connect(sockfd, pkt_mi, mh_mi->cont_len + MSG_HEADER_LEN); 
        if (ret != mh_mi->cont_len + MSG_HEADER_LEN) {
            fprintf(stderr, "output_udp_send_connect error!\n");
            perror("");
            return -1;
        }
    }

    return 0;
}

void *rcv(void *args)
{
	fprintf(stdout, "[Thread] Message handling thread run...\n");
    int sockfd;
    sockfd = input_udp_init(22080);
    if (sockfd < 0) {
        fprintf(stderr, "input_udp_init error");
        fflush(stderr);
        return NULL;
    }
    uint32_t src_ip;
    uint32_t size;
    char buf[1500];
	struct msg_header_t *mh = (struct msg_header_t*)buf;

	while (1) {
        fflush(stderr);
		// recv 
		while (input_udp_recv(sockfd, &src_ip, buf, &size) < 0);
		// Ignore invalid packet
        if (size < MSG_HEADER_LEN + SURVEY_MSG_FIX_LEN) {
            fprintf(stderr, "size error");
			continue;
        }
		if (PROTO_MAGICNUM != mh->magic_num) {
            fprintf(stderr, "magic num error");
			continue;
        }
		if (0x11 != mh->msg_type) {
            fprintf(stderr, "msg_type error");
            continue;
        }
        struct survey_ind_t *si = (struct survey_ind_t*)(buf + MSG_HEADER_LEN);
        printf("%s\n", (char *)si + SURVEY_MSG_FIX_LEN);
    } 
}

int main(int argc, char *argv[])
{

    char *opt_output = NULL;
    char *opt_input = NULL;

    char optchr;
    char *s; 
    char buf[20];
    int len1, len2;
    uint32_t ip;
    uint16_t port;
    int sockfd;
    FILE *fp;
    uint64_t flen;
    int timeout = -1;
    uint64_t prog_id = 0x6000000000000000;

    while ((optchr = getopt(argc, argv, "i:o:p:t:v")) != -1) {
        switch(optchr) {
			case 'i': 
				opt_input = optarg;
				break;
            case 'o': 
				opt_output = optarg;
				break;
            case 't': 
				timeout = atoi(optarg);
				break;
            case 'p':
                if (strstr(optarg, "0x")) {
                    prog_id = strtoll(optarg, (char **)NULL, 16);
                } else {
                    prog_id = strtoll(optarg, (char **)NULL, 10);
                }
                break;
			case 'v':
				fprintf(stderr, "%s version : "SENDPROG_VERSION"\n", argv[0]);
				return 0;
			default:
                print_usage(argv[0]);
				return 0;
		}
	}


    if (NULL == opt_input || NULL == opt_output) {
        print_usage(argv[0]);
        return -1;
    }

    if (timeout < 0) {
        timeout = 1;
    }

    s = strchr (opt_output, ':');
    if (NULL == s) {
        fprintf(stderr, "output format error: %s\n", opt_output);
        return -1;
    }
    len1 = s - opt_output;
    len2 = strlen(opt_output) - len1;
    if (0 == len1 || len2 < 2) {
        fprintf(stderr, "output format error: %s\n", opt_output);
        return -1;
    }
    strncpy(buf, opt_output, len1); 
    buf[len1] = 0;
    ip = inet_addr(buf);
    s += 1;
    strncpy(buf, s, len2); 
    port = htons(atoi(buf));

    sockfd = output_udp_init_connect(ip, port, NULL);
    if (-1 == sockfd) {
        fprintf(stderr, "output_udp_init_connect error");
        return -1; 
    }


    fp = fopen(opt_input, "r");
    if (fp == NULL) {
        fprintf(stderr, "fopen %s\n", opt_input);
        exit(-1);
    }
    fseek(fp, 0L, SEEK_END);
    flen = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    printf("========= <parameter> =========\n"); 
    printf(" prog_id : 0x%lx (hex)\n", prog_id);
    printf("           %lu (dec)\n", prog_id);
    printf(" timeout : %d s\n", timeout);
    printf("    file : %s (%lu Byte)\n", opt_input, flen);
    printf("     dst : %s %d\n", inet_ntop(AF_INET, &ip, buf, sizeof buf), ntohs(port)); 
    printf("========= <parameter> =========\n"); 

    pthread_t rcv_tid;
    int ret = pthread_create(&rcv_tid, NULL, rcv, NULL);
    if (ret != 0) {
        printf("pthread_create error");
        return -1;
    }

    sleep(3);

    send_prog(fp, sockfd, flen, prog_id, timeout);

    fclose(fp); 
    output_udp_clear_connect(sockfd);
    return 0;
}

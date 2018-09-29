
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

int main (int argc, char *argv[])
{
    char buf[BUF_SIZE];
    struct msg_header_t *mh = (struct msg_header_t*)buf;
	struct live_data_t *ld = (struct live_data_t *)(buf + MSG_HEADER_LEN); 

    memset(buf, 0, BUF_SIZE);
    mh->magic_num = PROTO_MAGICNUM;
    mh->version = 3;
    mh->msg_type = 0x41;
    mh->cont_len = 8;

    int fd_out = output_udp_init();
    if (fd_out < 0) {
        printf("socket error!");
        exit(1);
    }

    uint32_t ip = inet_addr(argv[1]);
    uint32_t port = htons(22087);

    //*((uint64_t *)msi_survey->prog_id) = 1811047534659531684;
    *((uint64_t *)ld->prog_id) = strtoull(argv[2], NULL, 0);

    printf("%lu\n", *((uint64_t *)ld->prog_id));
    output_udp_send(fd_out, ip, port, 
            (unsigned char *)buf, MSG_HEADER_LEN + mh->cont_len);  


	int fd_data = input_udp_init(port);
	if (fd_data < 0) {
		printf("socket error!");
		exit(1);
	}
	uint32_t src_ip;
	uint32_t size;
	char buf_data[BUF_SIZE];
	struct msg_header_t *mh_data = (struct msg_header_t *)buf_data;
	struct live_data_t *ld_data = (struct live_data_t *)(buf_data + MSG_HEADER_LEN); 

	if (input_udp_recv(fd_data, &src_ip, (unsigned char *)buf_data, &size) < 0)
		exit(0);
	if (0x42 != mh_data->msg_type)
		exit(0);
	printf("got a heart beat reply! %lu\n", *((uint64_t *)ld_data->prog_id));

	return 0;

}


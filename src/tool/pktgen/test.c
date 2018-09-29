#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "output.h"
int main() {
	uint32_t s_addr = inet_addr("127.0.0.1");
	uint16_t sin_port = htons(22082);

	//int sockfd = output_udp_init();
	int sockfd = output_udp_init_connect(s_addr, sin_port);
	uint8_t buf[100];
	int i;
	int send = 0, error = 0;

	while(1) {
		//if (output_udp_send(sockfd, s_addr, sin_port, buf, 50) < 0) {
		if (output_udp_send_connect(sockfd, buf, 50) < 0) {
			error ++;
			perror("error");
			continue;
		}
		printf("ok\n");
		send++;
		sleep(1);
	}
	output_udp_clear_connect(sockfd);
	//output_udp_clear(sockfd);
	printf("%d %d\n", send, error);
	return 1;
}

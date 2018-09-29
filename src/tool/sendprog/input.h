#ifndef _INPUT_H_
#define _INPUT_H_

#include <stdint.h>

int input_udp_init(uint16_t port);

void input_udp_clear(int sockfd);

int input_udp_recv(int sockfd, uint32_t *src_ip, uint8_t *buf, uint32_t *size);
#endif /* _INPUT_H_ */

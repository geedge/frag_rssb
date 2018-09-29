#ifndef _INPUT_H_
#define _INPUT_H_

#include <stdint.h>

int input_udp_init(uint16_t port);

void input_udp_clear(int sockfd);

int input_udp_recv(int sockfd, uint32_t *src_ip, uint8_t *buf, uint32_t buf_size);
int init_input_un_socket(const char* file_name);
int named_unix_domain_socket_recv(int un_sd,char*buf,int buf_len);
#endif /* _INPUT_H_ */

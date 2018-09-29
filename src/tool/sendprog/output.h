#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include <stdint.h>

// send udp packet

int output_udp_init();
void output_udp_clear(int sockfd);
int output_udp_send(int sockfd, uint32_t addr, uint16_t port, uint8_t *data, int datalen);


int output_udp_init_connect(uint32_t addr, uint16_t port, char *dev);
void output_udp_clear_connect(int sockfd);
int output_udp_send_connect(int sockfd, uint8_t *data, int datalen);
#endif /* _OUTPUT_H_ */

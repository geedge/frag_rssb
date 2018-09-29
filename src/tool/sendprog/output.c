// output.c
//
//
//
// MODIFIED by
// CREATED BY Guo Le, 08/24/2010
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "output.h"

int output_udp_init()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (-1 == sockfd) {
        printf("udpout_init: socket %s\n", strerror(errno));
        return -1;
    }
    return sockfd;
}


void output_udp_clear(int sockfd)
{
    close(sockfd);
}

// send udp packet
int output_udp_send(int sockfd, uint32_t addr, uint16_t port, uint8_t *data, int datalen)
{
    struct sockaddr_in dst_addr; /* connector's address information */
    dst_addr.sin_family = AF_INET; /* host byte order */
    dst_addr.sin_port = port; /* short, network byte order */
    dst_addr.sin_addr.s_addr = addr;
    bzero(&(dst_addr.sin_zero), 8); /* zero the rest of the struct */

    int numbytes = sendto(sockfd, data, datalen, 0, (struct sockaddr *)&dst_addr, sizeof(struct sockaddr));
    if (-1 == numbytes) {
        printf("udpout_send: sendto: %s", strerror(errno));
		return -1;
    }
    //printf("sent %d bytes to %s\n",numbytes,inet_ntoa(dst_addr.sin_addr));
    
    if (numbytes != datalen)
        printf("udpout_send: loss data");

    return numbytes;
}


int output_udp_init_connect(uint32_t addr, uint16_t port, char *dev)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (-1 == sockfd) {
        printf("udpout_init_connect: socket %s\n", strerror(errno));
        return -1;
    }
	struct sockaddr_in dst_addr; /* connector's address information */
	dst_addr.sin_family = AF_INET; /* host byte order */
	dst_addr.sin_port = port; /* short, network byte order */
	dst_addr.sin_addr.s_addr = addr;
    bzero(&(dst_addr.sin_zero), 8); /* zero the rest of the struct */

	if (-1 == connect(sockfd, (struct sockaddr*)&dst_addr, sizeof(dst_addr)))
	{
        printf("udpout_init_connect: error in connect %s\n", strerror(errno));
		return -1;
	}

    if (dev) {
        struct ifreq interface;
        strncpy(interface.ifr_ifrn.ifrn_name, dev, IFNAMSIZ);
        if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&interface, sizeof(interface))  < 0) {
            printf("SO_BINDTODEVICE failed %s\n", strerror(errno));
            return -1;
        }
    }

    return sockfd;
}

void output_udp_clear_connect(int sockfd)
{
    close(sockfd);
}

// send udp packet
int output_udp_send_connect(int sockfd, uint8_t *data, int datalen)
{
    int numbytes = send(sockfd, data, datalen, 0);
    if (-1 == numbytes) {
        printf("udpout_send_connect: send: %d %s\n", errno, strerror(errno));
        return -1;
    }

    if (numbytes != datalen) {
        printf("udpout_send_connect: loss data");
    }
    return numbytes;
}
